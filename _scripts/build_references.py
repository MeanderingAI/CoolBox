#!/usr/bin/env python3

"""
Script to generate reference pages from .bib files in the bib/ folder.
Each .bib file will become a single HTML page with a bullet list of references.
"""

import re
import html
from pathlib import Path
from build_libraries_bib import parse_bib_entries

def bib_to_html(bib_path, output_dir):
    bib_content = bib_path.read_text(encoding='utf-8')
    entries = parse_bib_entries(bib_content)
    items = '\n'.join(f'<li><strong>{html.escape(e["title"])}</strong> <span class="muted">{html.escape(e["author"])}</span></li>' for e in entries)
    html_content = f'''<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>References: {html.escape(bib_path.stem)}</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 900px; padding: 0 1rem; background: #f8fafc; color: #0f172a; }}
    main {{ background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }}
    h1 {{ display: flex; align-items: center; gap: 0.5rem; margin: 0 0 1.5rem 0; }}
    .muted {{ color: #64748b; font-size: 0.95rem; }}
    ul {{ margin: 1.5rem 0; }}
    li {{ margin: 1rem 0; }}
    a {{ color: #2563eb; text-decoration: none; }}
    a:hover {{ text-decoration: underline; }}
  </style>
</head>
<body>
  <main>
    <a href="../publications/index.html" class="back-arrow" title="Back to publications">&#8592;</a>
    <h1>𖥂 References</h1>
    <ul>
      {items}
    </ul>
  </main>
</body>
</html>
'''
    out_path = output_dir / f'{bib_path.stem}.html'
    out_path.write_text(html_content, encoding='utf-8')
    return out_path.name

def build_references(bib_dir, output_dir):
    output_dir.mkdir(parents=True, exist_ok=True)
    for bib_file in bib_dir.glob('*.bib'):
        bib_to_html(bib_file, output_dir)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Generate reference HTML pages from .bib files.")
    parser.add_argument('--bib', default='bib', help='Path to bib directory')
    parser.add_argument('--out', default='build/tutorials-site/references', help='Output directory for HTML files')
    args = parser.parse_args()
    build_references(Path(args.bib), Path(args.out))
