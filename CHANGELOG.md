# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.4.0] - 2026-09-13

### Added

- CLI contract: documented exit codes (`0` ok, `1` error, `2` usage/input, `3` network,
  `4` partial failure) and `iggen::ExitCode`.
- Standard flags: `--version`, `-q/--quiet`, `--debug`, `--no-color`, `--no-input`,
  `-y/--yes`, `--json`, and `-n/--dry-run`.
- `--json` machine-readable result on stdout (`include/report.hpp`).
- `--ai-api-key-file` and `--ai-api-key-stdin` so API keys never appear in the process
  list or shell history.
- `IGGEN_API_BASE_URL` endpoint override (mirrors and offline mock servers) and
  `IGGEN_API_RETRIES` retry control.
- CLI contract tests (`tests/test_cli.cpp`) covering exit codes, stdout/stderr
  separation, JSON output, and the cache-update paths against a loopback mock server.
- `CHANGELOG.md`, generated `IGGEN_VERSION` header, and a single source of truth for the
  project version.

### Changed

- Status/progress logs now go to **stderr** (via wcppcli 0.2.0) so `iggen --dry-run > file`
  captures only `.gitignore` content. This is required for the wcppcli 0.2.0 dependency.
- A non-interactive overwrite without `-y/--yes` now fails with exit code `2` and guidance
  instead of silently succeeding (previously exit `0` without writing anything).
- Invalid template names are rejected before any HTTP request is made.
- Template directory exclusions are shared between the language detector and the AI project
  scanner (`include/ignore_rules.hpp`); `vcpkg_installed`, `cmake-build-*`, `.venv`, and
  similar directories are now skipped consistently.
- The template cache is written atomically (temp file + rename) and carries a
  `schema_version`.

### Fixed

- `iggen update` no longer skips ~25% of templates: the gitignore.io `/list` response wraps
  lines, and the parser only split on commas, so wrapped names were sent as invalid URLs.
- Unreachable gitignore.io with no usable local template now returns exit code `3` instead
  of a generic failure.

### Security

- LLM API keys are no longer accepted only as a command-line flag; `--ai-api-key` is
  deprecated in favour of file/stdin input.
- Template names are validated (`[A-Za-z0-9+#._-]{1,64}`) to prevent path injection into
  API URLs.
- TLS certificate verification remains enabled for `https://` endpoints; the `http://`
  override exists only for local testing.

## [0.2.0]

### Added

- Offline template store with compiled-in defaults, user cache, and API → cache → builtin
  fallback.
- LLM-based refinement (`--ai`) with graceful fallback.
- `--dry-run`, `--no-defaults`, `--output`, `--lang`.

## [0.1.0]

### Added

- Initial release: scan the current directory, detect languages, and generate `.gitignore`
  through the gitignore.io API.

[Unreleased]: https://github.com/wkqco33/iggen/compare/v0.4.0...HEAD
[0.4.0]: https://github.com/wkqco33/iggen/compare/v0.3.0...v0.4.0
[0.2.0]: https://github.com/wkqco33/iggen/compare/v0.1.1...v0.2.0
[0.1.0]: https://github.com/wkqco33/iggen/releases/tag/v0.1.0
