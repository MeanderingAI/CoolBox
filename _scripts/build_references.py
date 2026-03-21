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
    pages = []
    for bib_file in sorted(bib_dir.glob('*.bib')):
        page_name = bib_to_html(bib_file, output_dir)
        pages.append((bib_file.stem, page_name))

    # Generate references index page
    if pages:
        items = '\n'.join(f'<li><a href="{name}">{html.escape(title)}</a></li>' for title, name in pages)
        subtitle = ''
    else:
        items = '<li>No references are available yet.</li>'
        subtitle = '<p class="muted">No references are available yet.</p>'

    index_html = f'''<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>References Index</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 900px; padding: 0 1rem; background: #f8fafc; color: #0f172a; }}
    main {{ background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }}
    h1 {{ margin-bottom: 1rem; }}
    .muted {{ color: #64748b; font-size: 0.95rem; }}
    ul {{ margin: 1.5rem 0; }}
    li {{ margin: 0.75rem 0; }}
    a {{ color: #2563eb; text-decoration: none; }}
    a:hover {{ text-decoration: underline; }}
  </style>
</head>
<body>
  <main>
    <a href="../publications/index.html" class="back-arrow" title="Back to publications">← Back to publications</a>
    <h1>𖥂 References</h1>
    {subtitle}
    <ul>
      {items}
    </ul>
  </main>
</body>
</html>'''

    (output_dir / 'index.html').write_text(index_html, encoding='utf-8')

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Generate reference HTML pages from .bib files.")
    parser.add_argument('--bib', default='bib', help='Path to bib directory')
    parser.add_argument('--out', default='build/documentation_site/references', help='Output directory for HTML files (should be references/)')
    args = parser.parse_args()
    # Always ensure output is in references/ (not tutorials/references/)
    output_dir = Path(args.out)
    if output_dir.name == "references" and output_dir.parent.name == "tutorials":
      # Move up one directory if path is tutorials/references
      output_dir = output_dir.parent.parent / "references"
    build_references(Path(args.bib), output_dir)
