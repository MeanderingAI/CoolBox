#!/usr/bin/env python3
from __future__ import annotations

import argparse
import html
import re
import shutil
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable


REMOTE_URL_RE = re.compile(r"^(?:[a-zA-Z][a-zA-Z0-9+.-]*://|git@|mailto:)")
HEADING_RE = re.compile(r"^(#{1,6})\s+(.*)$")


@dataclass
class TutorialBlock:
    kind: str
    value: str = ""
    meta: str = ""
    extra: str = ""


@dataclass
class TutorialPage:
    source_path: Path
    title: str
    tags: list[str] = field(default_factory=list)
    libraries: list[str] = field(default_factory=list)
    repo: str = ""
    publication_date: str = ""
    description: str = ""
    blocks: list[TutorialBlock] = field(default_factory=list)

    @property
    def slug(self) -> str:
        relative = self.source_path.with_suffix("").as_posix()
        return relative


def is_remote_url(reference: str) -> bool:
    return bool(REMOTE_URL_RE.match(reference.strip()))


def normalize_library_reference(reference: str) -> str:
    return "/".join(part for part in reference.strip().replace("\\", "/").split("/") if part)


def _split_csv(value: str) -> list[str]:
    return [item.strip() for item in value.split(",") if item.strip()]


def _html_escape_text(text: str) -> str:
    return html.escape(text, quote=False)


def _copy_local_asset(source_root: Path, output_root: Path, tutorial_dir: Path, reference: str) -> str:
    reference = reference.strip()
    if not reference or is_remote_url(reference):
        return reference

    asset_source = (tutorial_dir / reference).resolve() if not Path(reference).is_absolute() else Path(reference).resolve()
    if not asset_source.exists() or not asset_source.is_file():
        return reference

    try:
        relative_asset = asset_source.relative_to(source_root.resolve())
    except ValueError:
        relative_asset = Path(asset_source.name)

    asset_target = output_root / relative_asset
    asset_target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(asset_source, asset_target)
    return relative_asset.as_posix()


def parse_tutorial(path: Path, source_root: Path | None = None) -> TutorialPage:
    source_text = path.read_text(encoding="utf-8")
    lines = source_text.splitlines()

    title = path.stem.replace("-", " ").replace("_", " ").title()
    tags: list[str] = []
    libraries: list[str] = []
    repo = ""
    publication_date = ""
    description = ""
    blocks: list[TutorialBlock] = []

    paragraph_lines: list[str] = []
    code_lines: list[str] = []
    in_code = False
    code_meta = ""
    code_lang = "text"

    def flush_paragraph() -> None:
        nonlocal paragraph_lines
        if paragraph_lines:
            paragraph = " ".join(line.strip() for line in paragraph_lines if line.strip())
            if paragraph:
                blocks.append(TutorialBlock(kind="paragraph", value=paragraph))
            paragraph_lines = []

    def flush_code() -> None:
        nonlocal code_lines, code_meta, code_lang
        if code_lines:
            blocks.append(TutorialBlock(kind="code", value="\n".join(code_lines), meta=code_lang, extra=code_meta))
            code_lines = []
            code_meta = ""
            code_lang = "text"

    for raw_line in lines:
        line = raw_line.rstrip()
        stripped = line.strip()

        if in_code:
            if stripped.lower() == "@endcode":
                in_code = False
                flush_code()
                continue
            code_lines.append(raw_line)
            continue

        if not stripped:
            flush_paragraph()
            continue

        if stripped.startswith("@"):
            flush_paragraph()
            lower = stripped.lower()
            if lower.startswith("@title1:"):
                title = stripped.split(":", 1)[1].strip()
            elif lower.startswith("@tags:"):
                tags = _split_csv(stripped.split(":", 1)[1])
            elif lower.startswith("@libs:"):
                libraries = [normalize_library_reference(item) for item in _split_csv(stripped.split(":", 1)[1])]
            elif lower.startswith("@repo:"):
                repo = stripped.split(":", 1)[1].strip()
            elif lower.startswith("@publication_date:"):
                publication_date = stripped.split(":", 1)[1].strip()
            elif lower.startswith("@desc:"):
                description = stripped.split(":", 1)[1].strip()
            elif lower.startswith("@title"):
                level_text, value = stripped[1:].split(":", 1)
                match = re.match(r"title([2-6])", level_text.lower())
                level = int(match.group(1)) if match else 2
                blocks.append(TutorialBlock(kind="heading", value=value.strip(), meta=str(level)))
            elif lower.startswith("@link:"):
                payload = stripped.split(":", 1)[1].strip()
                if "|" in payload:
                    href, label = [part.strip() for part in payload.split("|", 1)]
                else:
                    href, label = payload, payload
                blocks.append(TutorialBlock(kind="link", value=href, meta=label))
            elif lower.startswith("@image:"):
                payload = stripped.split(":", 1)[1].strip()
                if "|" in payload:
                    ref, alt = [part.strip() for part in payload.split("|", 1)]
                else:
                    ref, alt = payload, ""
                blocks.append(TutorialBlock(kind="image", value=ref, meta=alt))
            elif lower.startswith("@video:"):
                payload = stripped.split(":", 1)[1].strip()
                if "|" in payload:
                    ref, caption = [part.strip() for part in payload.split("|", 1)]
                else:
                    ref, caption = payload, ""
                blocks.append(TutorialBlock(kind="video", value=ref, meta=caption))
            elif lower.startswith("@code:"):
                payload = stripped.split(":", 1)[1].strip()
                if "|" in payload:
                    code_lang, code_meta = [part.strip() for part in payload.split("|", 1)]
                else:
                    code_lang = payload.strip() or "text"
                    code_meta = ""
                in_code = True
                code_lines = []
            else:
                paragraph_lines.append(stripped[1:].strip())
            continue

        heading_match = HEADING_RE.match(stripped)
        if heading_match:
            blocks.append(TutorialBlock(kind="heading", value=heading_match.group(2).strip(), meta=str(len(heading_match.group(1)))))
            continue

        paragraph_lines.append(stripped)

    flush_paragraph()
    if in_code:
        flush_code()

    source_path = path
    if source_root is not None:
        try:
            source_path = path.resolve().relative_to(source_root.resolve())
        except ValueError:
            source_path = path.name

    return TutorialPage(
        source_path=source_path,
        title=title,
        tags=tags,
        libraries=libraries,
        repo=repo,
        publication_date=publication_date,
        description=description,
        blocks=blocks,
    )


def render_block(page: TutorialPage, block: TutorialBlock, source_root: Path, output_root: Path) -> str:
    if block.kind == "paragraph":
        return f"<p>{_html_escape_text(block.value)}</p>"

    if block.kind == "heading":
        level = max(1, min(6, int(block.meta or "2")))
        return f"<h{level}>{_html_escape_text(block.value)}</h{level}>"

    if block.kind == "link":
        href = _html_escape_text(block.value)
        label = _html_escape_text(block.meta or block.value)
        return f'<p><a href="{href}">{label}</a></p>'

    if block.kind == "image":
        ref = _copy_local_asset(source_root, output_root, page.source_path.parent, block.value)
        alt = _html_escape_text(block.meta or block.value)
        return f'<figure><img src="{html.escape(ref, quote=True)}" alt="{alt}"><figcaption>{alt}</figcaption></figure>'

    if block.kind == "video":
        ref = _copy_local_asset(source_root, output_root, page.source_path.parent, block.value)
        caption = _html_escape_text(block.meta or block.value)
        return f'<figure><video controls src="{html.escape(ref, quote=True)}"></video><figcaption>{caption}</figcaption></figure>'

    if block.kind == "code":
        code = _html_escape_text(block.value)
        lang = _html_escape_text(block.meta or "text")
        extra = f'<p class="code-meta">{_html_escape_text(block.extra)}</p>' if block.extra else ""
        return f'<div class="code-block"><pre><code class="language-{lang}">{code}</code></pre>{extra}</div>'

    return ""


def render_page(page: TutorialPage, output_root: Path, source_root: Path | None = None) -> Path:
    source_root = source_root or page.source_path.parent.parent
    destination = output_root / page.slug
    destination = destination.with_suffix(".html")
    destination.parent.mkdir(parents=True, exist_ok=True)

    blocks_html = "\n".join(render_block(page, block, source_root, output_root) for block in page.blocks)
    tags_html = ", ".join(f'<span>{_html_escape_text(tag)}</span>' for tag in page.tags) or '<span class="muted">None</span>'
    libs_html = ", ".join(f'<code>{_html_escape_text(lib)}</code>' for lib in page.libraries) or '<span class="muted">None</span>'
    publication_html = f'<p class="meta"><strong>Published:</strong> {_html_escape_text(page.publication_date)}</p>' if page.publication_date else ""
    repo_html = f'<p class="meta"><strong>Repo:</strong> <a href="{html.escape(page.repo, quote=True)}">{html.escape(page.repo)}</a></p>' if page.repo else ""
    desc_html = f'<p class="description">{_html_escape_text(page.description)}</p>' if page.description else ""

    html_text = f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>{_html_escape_text(page.title)} | CoolBox Tutorials</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 0 auto; max-width: 920px; padding: 2rem 1rem 4rem; line-height: 1.6; color: #0f172a; background: #f8fafc; }}
    header, .meta-box, article {{ background: white; border: 1px solid #dbeafe; border-radius: 14px; padding: 1.25rem; box-shadow: 0 12px 36px rgba(15, 23, 42, 0.07); }}
    header {{ margin-bottom: 1rem; }}
    h1, h2, h3, h4, h5, h6 {{ color: #111827; line-height: 1.2; }}
    .meta-box {{ margin: 1rem 0 1.5rem; }}
    .meta {{ margin: 0.25rem 0; color: #334155; }}
    .description {{ color: #475569; font-size: 1.05rem; }}
    pre {{ overflow-x: auto; background: #0f172a; color: #e2e8f0; padding: 1rem; border-radius: 12px; }}
    code {{ font-family: Menlo, Monaco, Consolas, monospace; }}
    .code-block {{ margin: 1rem 0; }}
    figure {{ margin: 1.25rem 0; }}
    img, video {{ max-width: 100%; border-radius: 12px; }}
    .muted {{ color: #64748b; }}
  </style>
</head>
<body>
  <header>
    <h1>{_html_escape_text(page.title)}</h1>
    {desc_html}
  </header>
  <section class="meta-box">
    {publication_html}
    {repo_html}
    <p class="meta"><strong>Tags:</strong> {tags_html}</p>
    <p class="meta"><strong>Libraries:</strong> {libs_html}</p>
  </section>
  <article>
    {blocks_html}
  </article>
</body>
</html>
"""
    destination.write_text(html_text, encoding="utf-8")
    return destination


def _write_tags_index(output_root: Path, pages: list[TutorialPage]) -> None:
    tags_dir = output_root / "tags"
    tags_dir.mkdir(parents=True, exist_ok=True)

    tag_map: dict[str, list[TutorialPage]] = {}
    for page in pages:
        for tag in page.tags:
            tag_map.setdefault(tag, []).append(page)

    index_items = []
    for tag in sorted(tag_map):
        slug = re.sub(r"[^a-z0-9]+", "-", tag.lower()).strip("-") or "tag"
        tag_pages = tag_map[tag]
        tag_path = tags_dir / f"{slug}.html"
        tag_links = "".join(
            f'<li><a href="../{page.slug}.html">{_html_escape_text(page.title)}</a></li>'
            for page in tag_pages
        )
        tag_path.write_text(
            f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>{_html_escape_text(tag)} | CoolBox Tags</title>
</head>
<body>
  <h1>Tag: {_html_escape_text(tag)}</h1>
  <ul>{tag_links}</ul>
  <p><a href="index.html">Back to tags index</a></p>
</body>
</html>
""",
            encoding="utf-8",
        )
        index_items.append(f'<li><a href="{slug}.html">{_html_escape_text(tag)}</a> ({len(tag_pages)})</li>')

    (tags_dir / "index.html").write_text(
        f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Tutorial Tags</title>
</head>
<body>
  <h1>CoolBox Tutorial Tags</h1>
  <ul>{''.join(index_items)}</ul>
  <p><a href="../index.html">Back to tutorials index</a></p>
</body>
</html>
""",
        encoding="utf-8",
    )


def build_tutorial_site(source_dir: Path, output_dir: Path) -> list[TutorialPage]:
    source_dir = source_dir.resolve()
    output_dir = output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    pages = [parse_tutorial(path, source_dir) for path in sorted(source_dir.rglob("*.tut"))]

    for page in pages:
        if page.source_path.is_absolute():
            try:
                page.source_path = page.source_path.relative_to(source_dir.resolve())
            except ValueError:
                page.source_path = Path(page.source_path.name)
        render_page(page, output_dir, source_dir)

    if pages:
        _write_tags_index(output_dir, pages)

    tutorial_items = []
    for page in pages:
        tags = ", ".join(_html_escape_text(tag) for tag in page.tags) or "No tags"
        tutorial_items.append(
            f'<li><a href="{page.slug}.html">{_html_escape_text(page.title)}</a><span> - {tags}</span></li>'
        )

    (output_dir / "index.html").write_text(
        f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Tutorials</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 0 auto; max-width: 900px; padding: 2rem 1rem 4rem; background: #f8fafc; color: #0f172a; }}
    h1 {{ margin-bottom: 0.25rem; }}
    p {{ color: #475569; }}
    ul {{ list-style: none; padding: 0; }}
    li {{ margin: 0.8rem 0; background: white; border: 1px solid #dbeafe; border-radius: 12px; padding: 0.85rem 1rem; }}
    a {{ color: #2563eb; text-decoration: none; font-weight: 600; }}
    a:hover {{ text-decoration: underline; }}
    span {{ color: #64748b; }}
  </style>
</head>
<body>
  <h1>CoolBox Tutorials</h1>
  <p>Static tutorials generated from <code>.tut</code> files.</p>
  <p><a href="tags/index.html">Browse tags</a></p>
  <ul>{''.join(tutorial_items) if tutorial_items else '<li>No tutorials found.</li>'}</ul>
</body>
</html>
""",
        encoding="utf-8",
    )
    return pages


def main() -> int:
    parser = argparse.ArgumentParser(description="Build static HTML pages from CoolBox .tut tutorial files.")
    parser.add_argument("source", nargs="?", default="tutorials", help="Directory containing .tut files")
    parser.add_argument("output", nargs="?", default="build/tutorials-site", help="Directory for generated HTML")
    args = parser.parse_args()

    source = Path(args.source)
    output = Path(args.output)
    pages = build_tutorial_site(source, output)
    print(f"Built tutorials site at {output.resolve()} ({len(pages)} tutorial(s))")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
