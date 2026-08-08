#!/usr/bin/env python3
from __future__ import annotations

import argparse
import html
import re
from pathlib import Path


ENTRY_RE = re.compile(r"@(?P<kind>\w+)\s*\{(?P<key>[^,]+),(?P<body>.*?)\n\}", re.S)
FIELD_RE = re.compile(r"(?P<field>\w+)\s*=\s*[{\"](?P<value>.*?)[}\"]\s*,?\s*$", re.M)


def parse_bib_file(path: Path) -> list[dict[str, str]]:
    text = path.read_text(encoding="utf-8", errors="ignore")
    entries: list[dict[str, str]] = []
    for match in ENTRY_RE.finditer(text):
        body = match.group("body")
        fields: dict[str, str] = {"key": match.group("key").strip(), "kind": match.group("kind").strip()}
        for field_match in FIELD_RE.finditer(body):
            fields[field_match.group("field").lower()] = field_match.group("value").strip()
        entries.append(fields)
    return entries


def build_publications_site(bib_dir: Path, output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    publications_dir = output_dir / "publications"
    references_dir = output_dir / "references"
    publications_dir.mkdir(parents=True, exist_ok=True)
    references_dir.mkdir(parents=True, exist_ok=True)

    entries: list[dict[str, str]] = []
    if bib_dir.exists():
        for bib_file in sorted(bib_dir.rglob("*.bib")):
            entries.extend(parse_bib_file(bib_file))

    list_items = []
    for entry in entries:
        title = entry.get("title") or entry.get("key", "Untitled")
        author = entry.get("author") or entry.get("authors", "Unknown author")
        year = entry.get("year", "n.d.")
        list_items.append(
            f"<li><strong>{html.escape(title)}</strong><br><span>{html.escape(author)} · {html.escape(year)}</span></li>"
        )

    publications_dir.joinpath("index.html").write_text(
        f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Publications</title>
</head>
<body>
  <h1>CoolBox Publications</h1>
  <ul>{''.join(list_items) if list_items else '<li>No publication entries found.</li>'}</ul>
</body>
</html>
""",
        encoding="utf-8",
    )

    references_dir.joinpath("index.html").write_text(
        f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox References</title>
</head>
<body>
  <h1>CoolBox References</h1>
  <p>{len(entries)} reference entry(ies) discovered.</p>
</body>
</html>
""",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Build publications and reference pages from .bib files.")
    parser.add_argument("--bib", default="bib", help="Directory containing .bib files")
    parser.add_argument("--out", required=True, help="Output directory")
    args = parser.parse_args()

    build_publications_site(Path(args.bib), Path(args.out))
    print(f"Built publication pages at {Path(args.out).resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())