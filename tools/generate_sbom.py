#!/usr/bin/env python3
"""릴리스 산출물용 SPDX SBOM 생성기.

vcpkg가 설치한 패키지 목록(vcpkg_installed/vcpkg/status)에서 정확한 의존성 버전을 읽어
SPDX 2.3 JSON을 만든다. 바이너리만 스캔하면 C++ 의존성이 드러나지 않아 SBOM이 사실상
빈 문서가 되므로, vcpkg의 설치 상태를 단일 소스로 사용한다.

사용:
    generate_sbom.py --asset iggen_linux_amd64.tar.gz \
                     --status build/release/vcpkg_installed/vcpkg/status \
                     --out iggen_linux_amd64.tar.gz.spdx.json
    generate_sbom.py --selftest
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path

SCHEMA_VERSION = "SPDX-2.3"
DATA_LICENSE = "CC0-1.0"
SBOM_TOOL_NAME = "iggen-generate_sbom.py"

# SPDX ID에 쓸 수 없는 문자를 '-'로 바꾼다 (SPDXID는 [A-Za-z0-9.-]+ 만 허용).
_INVALID_ID_CHARS = re.compile(r"[^A-Za-z0-9.-]")


def sanitize_id(value: str) -> str:
    cleaned = _INVALID_ID_CHARS.sub("-", value.strip())
    return cleaned or "unknown"


def read_text(path: str) -> str | None:
    """파일을 읽는다. 실패하면 원인을 출력하고 None을 돌려준다."""
    try:
        return Path(path).read_text(encoding="utf-8", errors="replace")
    except OSError as error:
        print(f"error: cannot read {path}: {error}", file=sys.stderr)
        return None


def write_text(path: str, text: str) -> bool:
    try:
        Path(path).write_text(text, encoding="utf-8")
        return True
    except OSError as error:
        print(f"error: cannot write {path}: {error}", file=sys.stderr)
        return False


def write_bytes(path: str, data: bytes) -> bool:
    try:
        Path(path).write_bytes(data)
        return True
    except OSError as error:
        print(f"error: cannot write {path}: {error}", file=sys.stderr)
        return False


def sha256_of(path: str) -> str | None:
    digest = hashlib.sha256()
    try:
        with open(path, "rb") as handle:
            for chunk in iter(lambda: handle.read(1024 * 1024), b""):
                digest.update(chunk)
    except OSError as error:
        print(f"error: cannot hash {path}: {error}", file=sys.stderr)
        return None
    return digest.hexdigest()


def parse_json(text: str) -> dict | None:
    """JSON을 파싱한다. 실패하면 원인을 출력하고 None을 돌려준다."""
    try:
        return json.loads(text)
    except json.JSONDecodeError as error:
        print(f"error: invalid JSON: {error}", file=sys.stderr)
        return None


def parse_status(text: str) -> list[dict[str, str]]:
    """vcpkg status 파일을 패키지 스탠자 목록으로 파싱한다.

    형식은 Debian control과 같은 'Key: value' 블록이며 빈 줄로 구분된다.
    'install ok installed' 상태인 패키지만 대상으로 한다.
    """
    stanzas: list[dict[str, str]] = []
    current: dict[str, str] = {}
    for line in text.splitlines():
        if not line.strip():
            if current:
                stanzas.append(current)
                current = {}
            continue
        if line[0].isspace() or ":" not in line:
            continue  # 연속 줄/알 수 없는 형식은 무시
        key, _, value = line.partition(":")
        current[key.strip()] = value.strip()
    if current:
        stanzas.append(current)

    packages: list[dict[str, str]] = []
    for stanza in stanzas:
        name = stanza.get("Package", "")
        if not name or not stanza.get("Status", "").startswith("install ok"):
            continue
        packages.append(
            {
                "name": name,
                "version": stanza.get("Version", "unknown"),
                "architecture": stanza.get("Architecture", ""),
                "abi": stanza.get("Abi", ""),
                "depends": stanza.get("Depends", ""),
            }
        )
    packages.sort(key=lambda package: package["name"])
    return packages


def build_document(asset_name: str, asset_sha256: str, packages: list[dict[str, str]]) -> dict:
    created = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    namespace = f"https://github.com/wkqco33/iggen/sbom/{sanitize_id(asset_name)}-{asset_sha256}"

    asset_spdx_id = f"SPDXRef-Package-{sanitize_id(asset_name)}"
    spdx_packages: list[dict] = [
        {
            "name": asset_name,
            "SPDXID": asset_spdx_id,
            "versionInfo": "NOASSERTION",
            "downloadLocation": "NOASSERTION",
            "filesAnalyzed": False,
            "licenseConcluded": "MIT",
            "licenseDeclared": "MIT",
            "copyrightText": "NOASSERTION",
            "checksums": [{"algorithm": "SHA256", "checksumValue": asset_sha256}],
        }
    ]

    relationships: list[dict] = [
        {
            "spdxElementId": "SPDXRef-DOCUMENT",
            "relationshipType": "DESCRIBES",
            "relatedSpdxElement": asset_spdx_id,
        }
    ]

    id_by_name: dict[str, str] = {}
    for package in packages:
        spdx_id = f"SPDXRef-Package-{sanitize_id(package['name'])}"
        id_by_name[package["name"]] = spdx_id
        spdx_packages.append(
            {
                "name": package["name"],
                "SPDXID": spdx_id,
                "versionInfo": package["version"],
                "downloadLocation": "NOASSERTION",
                "filesAnalyzed": False,
                "licenseConcluded": "NOASSERTION",
                "licenseDeclared": "NOASSERTION",
                "copyrightText": "NOASSERTION",
                "externalRefs": [
                    {
                        "referenceCategory": "PACKAGE-MANAGER",
                        "referenceType": "purl",
                        "referenceLocator": f"pkg:vcpkg/{package['name']}@{package['version']}",
                    }
                ],
                "comment": (
                    f"vcpkg ABI {package['abi']}" if package["abi"] else "installed via vcpkg"
                ),
            }
        )
        relationships.append(
            {
                "spdxElementId": asset_spdx_id,
                "relationshipType": "DEPENDS_ON",
                "relatedSpdxElement": spdx_id,
            }
        )

    # 패키지 간 의존 관계도 기록한다(가능한 범위에서).
    for package in packages:
        source = id_by_name[package["name"]]
        for dependency in re.split(r",\s*", package["depends"]):
            dependency_name = dependency.split("(")[0].strip()
            target = id_by_name.get(dependency_name)
            if target:
                relationships.append(
                    {
                        "spdxElementId": source,
                        "relationshipType": "DEPENDS_ON",
                        "relatedSpdxElement": target,
                    }
                )

    return {
        "spdxVersion": SCHEMA_VERSION,
        "dataLicense": DATA_LICENSE,
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"{asset_name} SBOM",
        "documentNamespace": namespace,
        "creationInfo": {
            "created": created,
            "creators": [f"Tool: {SBOM_TOOL_NAME}"],
        },
        "packages": spdx_packages,
        "relationships": relationships,
    }


def generate(asset: str, status_paths: list[str], out_path: str) -> int:
    packages: dict[str, dict[str, str]] = {}
    for status_path in status_paths:
        text = read_text(status_path)
        if text is None:
            return 1
        for package in parse_status(text):
            packages.setdefault(package["name"], package)
    if not packages:
        print("error: no installed vcpkg packages found in the status file(s)", file=sys.stderr)
        return 1

    digest = sha256_of(asset)
    if digest is None:
        return 1

    asset_name = Path(asset).name
    document = build_document(asset_name, digest, sorted(packages.values(), key=lambda p: p["name"]))
    if not write_text(out_path, json.dumps(document, indent=2) + "\n"):
        return 1

    print(f"SBOM written: {out_path} ({len(document['packages'])} packages)")
    return 0


SELFTEST_STATUS = """Package: openssl
Version: 3.6.4
Architecture: x64-linux
Abi: abc123
Status: install ok installed

Package: cpr
Version: 1.11.2
Depends: curl, openssl
Architecture: x64-linux
Abi: def456
Status: install ok installed

Package: curl
Version: 8.15.0
Architecture: x64-linux
Abi: ghi789
Status: install ok installed

Package: not-installed-port
Version: 1.0.0
Status: purge ok not-installed
"""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(f"selftest failed: {message}")


def selftest() -> int:
    packages = parse_status(SELFTEST_STATUS)
    require(
        [package["name"] for package in packages] == ["cpr", "curl", "openssl"],
        "status 파싱/정렬/필터가 잘못되었습니다",
    )

    with tempfile.TemporaryDirectory() as tmp:
        asset = str(Path(tmp) / "iggen_test.tar.gz")
        status_path = str(Path(tmp) / "status")
        out_path = str(Path(tmp) / "out.spdx.json")

        require(write_bytes(asset, b"payload"), "asset 생성이 실패했습니다")
        require(generate(asset, [status_path], out_path) == 1, "status 없이 성공하면 안 됩니다")
        require(write_text(status_path, SELFTEST_STATUS), "status 생성이 실패했습니다")
        require(generate(asset, [status_path], out_path) == 0, "SBOM 생성이 실패했습니다")

        raw = read_text(out_path)
        require(raw is not None, "SBOM을 읽을 수 없습니다")
        parsed = parse_json(raw or "{}")
        if parsed is None:
            raise SystemExit("selftest failed: SBOM JSON을 파싱할 수 없습니다")
        document = parsed

    require(document.get("spdxVersion") == SCHEMA_VERSION, "spdxVersion이 잘못되었습니다")
    require(document.get("dataLicense") == DATA_LICENSE, "dataLicense가 잘못되었습니다")
    require(document.get("SPDXID") == "SPDXRef-DOCUMENT", "SPDXID가 잘못되었습니다")

    ids = {package["SPDXID"] for package in document["packages"]}
    require("SPDXRef-Package-iggen-test.tar.gz" in ids, f"asset 패키지 누락: {ids}")
    require("SPDXRef-Package-openssl" in ids, f"openssl 패키지 누락: {ids}")
    require("SPDXRef-Package-not-installed-port" not in ids, "미설치 포트가 포함되었습니다")

    asset_package = next(p for p in document["packages"] if p["name"] == "iggen_test.tar.gz")
    expected_digest = hashlib.sha256(b"payload").hexdigest()
    require(
        asset_package["checksums"][0]["checksumValue"] == expected_digest,
        "asset 체크섬이 잘못되었습니다",
    )

    openssl = next(p for p in document["packages"] if p["name"] == "openssl")
    require(openssl["versionInfo"] == "3.6.4", "openssl 버전이 잘못되었습니다")
    require(
        openssl["externalRefs"][0]["referenceLocator"] == "pkg:vcpkg/openssl@3.6.4",
        "purl이 잘못되었습니다",
    )

    relations = [
        (entry["spdxElementId"], entry["relationshipType"]) for entry in document["relationships"]
    ]
    require(("SPDXRef-DOCUMENT", "DESCRIBES") in relations, "DESCRIBES 관계가 없습니다")
    require(
        any(element.endswith("iggen-test.tar.gz") and kind == "DEPENDS_ON"
            for element, kind in relations),
        "asset -> 패키지 DEPENDS_ON 관계가 없습니다",
    )
    require(("SPDXRef-Package-cpr", "DEPENDS_ON") in relations, "cpr 의존 관계가 없습니다")

    print("selftest OK")
    return 0


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Generate an SPDX SBOM for a release asset")
    parser.add_argument("--asset", help="release artifact file")
    parser.add_argument(
        "--status", action="append", default=[], help="vcpkg status file (repeatable)"
    )
    parser.add_argument("--out", help="output SPDX JSON path")
    parser.add_argument("--selftest", action="store_true", help="run the built-in self test")
    args = parser.parse_args(argv)

    if args.selftest:
        return selftest()
    if not args.asset or not args.status or not args.out:
        parser.error("--asset, --status and --out are required")
    return generate(args.asset, args.status, args.out)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
