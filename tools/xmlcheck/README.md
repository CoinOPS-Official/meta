# menu.xml verifier (RapidXML)

## What it checks
- XML is well-formed (RapidXML parse). On failure, prints line + column and shows the offending line with a caret.
- Duplicate `<game name="...">` values. Duplicates are treated as a failing condition.

Exit codes:
- `0` = OK
- `1` = XML parse/IO/structural failure
- `2` = duplicates found

## Dependency: RapidXML
Place RapidXML header here:

`tools/xmlcheck/rapidxml/rapidxml.hpp`

(or change include paths in the workflow / build command).

## Local build (g++)
From repo root:

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic \
  -I tools/xmlcheck/rapidxml \
  tools/xmlcheck/XmlGameDuplicateFinder.cpp \
  -o xmlcheck
./xmlcheck data/menu.xml
```

## Local build (CMake)
From `tools/xmlcheck`:

```bash
cmake -S . -B build
cmake --build build --config Release
./build/xmlcheck ../../data/menu.xml
```

## GitHub Actions
Workflow file:

`.github/workflows/verify-menu-xml.yml`

It runs on pushes to `main` and on PRs when `data/menu.xml` or the verifier changes.
Adjust `data/menu.xml` if your path differs.
