#!/usr/bin/env python3
from __future__ import annotations

import argparse
import html
from pathlib import Path

from build_tutorials import parse_tutorial


def build_tags_site(source_dir: Path, output_dir: Path) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)

    pages = [parse_tutorial(path, source_dir) for path in sorted(source_dir.rglob("*.tut"))]
    tag_map: dict[str, list] = {}
    for page in pages:
        for tag in page.tags:
            tag_map.setdefault(tag, []).append(page)

    index_items: list[str] = []
    for tag in sorted(tag_map):
        slug = "-".join(part for part in tag.lower().split() if part)
        slug = "".join(ch if ch.isalnum() or ch == "-" else "-" for ch in slug).strip("-") or "tag"
        pages_for_tag = tag_map[tag]
        tag_file = output_dir / f"{slug}.html"
        tag_links = "".join(f'<li><a href="../{page.slug}.html">{html.escape(page.title)}</a></li>' for page in pages_for_tag)
        tag_file.write_text(
            f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>{html.escape(tag)} | CoolBox Tutorial Tags</title>
</head>
<body>
  <h1>{html.escape(tag)}</h1>
  <ul>{tag_links}</ul>
  <p><a href="index.html">Back to tags index</a></p>
</body>
</html>
""",
            encoding="utf-8",
        )
        index_items.append(f'<li><a href="{slug}.html">{html.escape(tag)}</a> ({len(pages_for_tag)})</li>')

    (output_dir / "index.html").write_text(
        f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Tutorial Tags</title>
</head>
<body>
  <h1>CoolBox Tutorial Tags</h1>
  <ul>{''.join(index_items) if index_items else '<li>No tags found.</li>'}</ul>
  <p><a href="../index.html">Back to tutorials index</a></p>
</body>
</html>
""",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Build tutorial tag pages from CoolBox .tut files.")
    parser.add_argument("--source", default="__init__/tutorials", help="Tutorial source directory")
    parser.add_argument("--out", required=True, help="Output directory for tag pages")
    args = parser.parse_args()

    build_tags_site(Path(args.source), Path(args.out))
    print(f"Built tutorial tag pages at {Path(args.out).resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())