---
description: "C++ conventions for sokketter library, CLI, and UI sources. Use when editing or creating C++ headers/sources (.h/.hpp/.cpp/.mm)."
applyTo: "**/*.{h,hpp,cpp,mm}"
---

# C++ conventions (sokketter)

Applies to first-party C++ under `libsokketter/`, `sokketter-cli/`, and `sokketter-ui/`. Do **not** apply these when editing vendored code in `third-party/`.

## Style

- Formatting is enforced by [.clang-format](../../.clang-format): 100-column limit, 4-space indent, Allman-style braces, pointers bind right (`Type *ptr`). Run clang-format rather than hand-formatting.
- Prefer trailing return types: `auto function_name(...) -> ReturnType`.
- Use `snake_case` for functions, variables, classes, and file names.
- Every header starts with an include guard **and** `#pragma once`, matching the existing pattern.
- Document public API and non-obvious members with Doxygen `@brief` comments; keep inline comments to a single line that adds information the code cannot.

## Library API

- Public symbols declared in [libsokketter.h](../../libsokketter/include/libsokketter.h) must be annotated with the `EXPORTED` macro from [export_definitions.h](../../libsokketter/include/export_definitions.h).
- Keep internal implementation headers in `sources/`; they are not part of the shipped public interface.

## Logging

- Use spdlog via the project macros: `SOKKETTER_LOGGER` in the library, `APP_LOGGER` in the UI, together with the `SPDLOG_LOGGER_TRACE/DEBUG/INFO/WARN/ERROR/CRITICAL` macros.
- Never log secrets such as authentication passwords.

## Platform code

- Guard OS-specific code with `_WIN32` / `__linux__` / `__APPLE__`.
- macOS-only Objective-C++ belongs in `.mm` files (e.g. [macos_theme_change_detection.mm](../../sokketter-ui/sources/macos_theme_change_detection.mm)).
