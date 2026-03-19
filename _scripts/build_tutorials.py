#!/usr/bin/env python3
from __future__ import annotations

import html
import json
import re
import shutil
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable
from urllib.parse import urlparse

TITLE_DIRECTIVE = re.compile(r"^@title([1-6]):\s*(.+?)\s*$")
HEADING_DIRECTIVE = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
TAGS_DIRECTIVE = re.compile(r"^@tags:\s*(.+?)\s*$")
LIBS_DIRECTIVE = re.compile(r"^@libs:\s*(.+?)\s*$")
REPO_DIRECTIVE = re.compile(r"^@repo:\s*(.+?)\s*$")
IMAGE_DIRECTIVE = re.compile(r"^@image:\s*(.+?)\s*$")
VIDEO_DIRECTIVE = re.compile(r"^@video:\s*(.+?)\s*$")
LINK_DIRECTIVE = re.compile(r"^@link:\s*(.+?)\s*$")
CODE_DIRECTIVE = re.compile(r"^@code:\s*(.+?)\s*$")
ENDCODE_DIRECTIVE = re.compile(r"^@endcode\s*$")


@dataclass
class Block:
    kind: str
    data: dict[str, object]


@dataclass
class TutorialPage:
    source: Path
    title: str
    slug: str
    tags: list[str] = field(default_factory=list)
    libs: list[str] = field(default_factory=list)
    repo: str = ""
    blocks: list[Block] = field(default_factory=list)
    excerpt: str = ""
    output_path: Path | None = None


@dataclass
class PublicationEntry:
    metadata_path: Path
    title: str = ""
    authors: list[str] = field(default_factory=list)
    abstract: str = ""
    year: str = ""
    tags: list[str] = field(default_factory=list)
    pdf: str = ""


def slugify(value: str) -> str:
    slug = re.sub(r"[^a-z0-9]+", "-", value.lower()).strip("-")
    return slug or "tutorial"


def split_payload(payload: str) -> tuple[str, str]:
    parts = [part.strip() for part in payload.split("|", 1)]
    if len(parts) == 1:
        return parts[0], ""
    return parts[0], parts[1]


def normalize_library_reference(reference: str) -> str:
    value = reference.strip().replace("\\", "/")
    for prefix in ("_libraries/backages/", "/_libraries/backages/", "backages/", "/backages/"):
        if value.startswith(prefix):
            return value[len(prefix):].strip("/")
    return value.strip("/")


def parse_code_payload(payload: str) -> tuple[str, list[str]]:
    language, libraries = split_payload(payload)
    language = language or "text"
    libs = [normalize_library_reference(item) for item in libraries.split(",") if item.strip()]
    return language, libs


def is_remote_url(value: str) -> bool:
    parsed = urlparse(value)
    return parsed.scheme in {"http", "https"}


def copy_asset(reference: str, source_file: Path, output_dir: Path, asset_dir: Path) -> str:
    if is_remote_url(reference):
        return reference

    source_path = (source_file.parent / reference).resolve()
    if not source_path.exists():
        raise FileNotFoundError(f"Missing tutorial asset: {reference} referenced by {source_file}")

    asset_dir.mkdir(parents=True, exist_ok=True)
    target_path = asset_dir / source_path.name
    shutil.copy2(source_path, target_path)
    return str(target_path.relative_to(output_dir)).replace("\\", "/")


def parse_tutorial(source_file: Path) -> TutorialPage:
    blocks: list[Block] = []
    tags: list[str] = []
    libs: list[str] = []
    repo = ""
    paragraph_lines: list[str] = []
    title = source_file.stem.replace("-", " ").title()
    excerpt = ""
    in_code_block = False
    code_language = "text"
    code_libraries: list[str] = []
    code_lines: list[str] = []

    def flush_paragraph() -> None:
        nonlocal excerpt
        if not paragraph_lines:
            return
        text = " ".join(line.strip() for line in paragraph_lines if line.strip())
        if text:
            blocks.append(Block("paragraph", {"text": text}))
            if not excerpt:
                excerpt = text
        paragraph_lines.clear()

    def flush_code_block() -> None:
        nonlocal in_code_block, code_language, code_libraries, code_lines
        if not in_code_block:
            return
        blocks.append(
            Block(
                "code",
                {
                    "language": code_language,
                    "libraries": code_libraries.copy(),
                    "code": "\n".join(code_lines).rstrip("\n"),
                },
            )
        )
        in_code_block = False
        code_language = "text"
        code_libraries = []
        code_lines = []

    for raw_line in source_file.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()

        if in_code_block:
            if ENDCODE_DIRECTIVE.match(line):
                flush_code_block()
            else:
                code_lines.append(raw_line)
            continue

        if not line:
            flush_paragraph()
            continue
        if line.startswith("//"):
            continue

        title_match = TITLE_DIRECTIVE.match(line)
        if title_match:
            flush_paragraph()
            level = int(title_match.group(1))
            text = title_match.group(2).strip()
            if level == 1 and title == source_file.stem.replace("-", " ").title():
                title = text
            blocks.append(Block("title", {"level": level, "text": text}))
            continue

        heading_match = HEADING_DIRECTIVE.match(line)
        if heading_match:
            flush_paragraph()
            level = len(heading_match.group(1))
            text = heading_match.group(2).strip()
            if level == 1 and title == source_file.stem.replace("-", " ").title():
                title = text
            blocks.append(Block("title", {"level": level, "text": text}))
            continue

        tags_match = TAGS_DIRECTIVE.match(line)
        if tags_match:
            flush_paragraph()
            tags = [tag.strip() for tag in tags_match.group(1).split(",") if tag.strip()]
            continue

        libs_match = LIBS_DIRECTIVE.match(line)
        if libs_match:
            flush_paragraph()
            libs = [normalize_library_reference(item) for item in libs_match.group(1).split(",") if item.strip()]
            continue

        repo_match = REPO_DIRECTIVE.match(line)
        if repo_match:
            flush_paragraph()
            repo = repo_match.group(1).strip()
            continue

        image_match = IMAGE_DIRECTIVE.match(line)
        if image_match:
            flush_paragraph()
            src, alt = split_payload(image_match.group(1))
            blocks.append(Block("image", {"src": src, "alt": alt}))
            continue

        video_match = VIDEO_DIRECTIVE.match(line)
        if video_match:
            flush_paragraph()
            src, caption = split_payload(video_match.group(1))
            blocks.append(Block("video", {"src": src, "caption": caption}))
            continue

        link_match = LINK_DIRECTIVE.match(line)
        if link_match:
            flush_paragraph()
            href, label = split_payload(link_match.group(1))
            blocks.append(Block("link", {"href": href, "label": label or href}))
            continue

        code_match = CODE_DIRECTIVE.match(line)
        if code_match:
            flush_paragraph()
            code_language, code_libraries = parse_code_payload(code_match.group(1))
            code_lines = []
            in_code_block = True
            continue

        paragraph_lines.append(line)

    flush_paragraph()
    flush_code_block()
    return TutorialPage(source=source_file, title=title, slug=slugify(source_file.stem), tags=tags, libs=libs, repo=repo, blocks=blocks, excerpt=excerpt)


def youtube_embed(url: str) -> str | None:
    parsed = urlparse(url)
    host = parsed.netloc.lower()
    if "youtube.com" in host:
        query = parsed.query
        match = re.search(r"(?:^|&)v=([^&]+)", query)
        if match:
            return f"https://www.youtube.com/embed/{match.group(1)}"
    if "youtu.be" in host:
        video_id = parsed.path.strip("/")
        if video_id:
            return f"https://www.youtube.com/embed/{video_id}"
    if "vimeo.com" in host:
        video_id = parsed.path.strip("/")
        if video_id:
            return f"https://player.vimeo.com/video/{video_id}"
    return None


def render_block(block: Block, page: TutorialPage, output_dir: Path, asset_dir: Path) -> str:
    if block.kind == "title":
        level = int(block.data["level"])
        text = html.escape(str(block.data["text"]))
        return f"<h{level}>{text}</h{level}>"

    if block.kind == "paragraph":
        text = html.escape(str(block.data["text"]))
        return f"<p>{text}</p>"

    if block.kind == "image":
        src = copy_asset(str(block.data["src"]), page.source, output_dir, asset_dir)
        alt = html.escape(str(block.data.get("alt", "")))
        return (
            '<figure class="media-card">'
            f'<img src="{html.escape(src)}" alt="{alt}">'
            + (f"<figcaption>{alt}</figcaption>" if alt else "")
            + "</figure>"
        )

    if block.kind == "video":
        src = str(block.data["src"])
        caption = html.escape(str(block.data.get("caption", "")))
        embed_url = youtube_embed(src)
        if embed_url:
            body = (
                '<div class="video-frame">'
                f'<iframe src="{html.escape(embed_url)}" title="Tutorial video" loading="lazy" allowfullscreen></iframe>'
                '</div>'
            )
        else:
            resolved = copy_asset(src, page.source, output_dir, asset_dir)
            body = f'<video controls preload="metadata" src="{html.escape(resolved)}"></video>'
        caption_html = f"<figcaption>{caption}</figcaption>" if caption else ""
        return f'<figure class="media-card">{body}{caption_html}</figure>'

    if block.kind == "link":
        href = html.escape(str(block.data["href"]))
        label = html.escape(str(block.data["label"]))
        return f'<p><a class="inline-link" href="{href}">{label}</a></p>'

    if block.kind == "code":
        language = html.escape(str(block.data.get("language", "text")))
        libraries = [html.escape(str(item)) for item in block.data.get("libraries", [])]
        code = html.escape(str(block.data.get("code", "")))
        libraries_html = "".join(f'<span class="code-lib">{item}</span>' for item in libraries)
        meta_html = (
            '<div class="code-meta">'
            f'<span class="code-language">{language}</span>'
            + (f'<div class="code-libraries">{libraries_html}</div>' if libraries_html else "")
            + '</div>'
        )
        return f'<section class="code-card">{meta_html}<pre><code class="language-{language}">{code}</code></pre></section>'

    return ""


def render_page(page: TutorialPage, output_dir: Path) -> None:
    asset_dir = output_dir / "assets" / page.slug
    blocks_html = "\n      ".join(render_block(block, page, output_dir, asset_dir) for block in page.blocks)
    tags_html = "".join(f'<span class="tag">{html.escape(tag)}</span>' for tag in page.tags)
    libs_html = "".join(f'<span class="tag lib-tag">{html.escape(lib)}</span>' for lib in page.libs)
    repo_html = f'<p><a class="inline-link repo-link" href="{html.escape(page.repo)}">Repository</a></p>' if page.repo else ""
    output_file = output_dir / f"{page.slug}.html"
    page.output_path = output_file
    output_file.write_text(
        f"""<!DOCTYPE html>
<html lang=\"en\">
<head>
  <meta charset=\"UTF-8\">
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
  <title>{html.escape(page.title)} | CoolBox Tutorials</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 900px; padding: 0 1rem; background: #f8fafc; color: #0f172a; }}
    main {{ background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }}
    h1, h2, h3, h4, h5, h6 {{ color: #111827; }}
    p {{ line-height: 1.65; color: #334155; }}
    a {{ color: #2563eb; text-decoration: none; }}
    a:hover {{ text-decoration: underline; }}
    .back-link {{ display: inline-block; margin-bottom: 1rem; }}
    .repo-link {{ font-weight: 700; }}
    .tag-row {{ display: flex; flex-wrap: wrap; gap: 0.5rem; margin: 0 0 1.25rem; }}
    .tag {{ background: #dbeafe; color: #1d4ed8; border-radius: 999px; padding: 0.25rem 0.7rem; font-size: 0.9rem; font-weight: 600; }}
    .lib-tag {{ background: #dcfce7; color: #166534; }}
    .media-card {{ margin: 1.5rem 0; }}
    .code-card {{ margin: 1.5rem 0; border: 1px solid #1e293b; border-radius: 14px; overflow: hidden; background: #0f172a; }}
    .code-meta {{ display: flex; flex-wrap: wrap; gap: 0.75rem; align-items: center; justify-content: space-between; padding: 0.8rem 1rem; background: #111827; border-bottom: 1px solid #334155; }}
    .code-language {{ color: #93c5fd; font-size: 0.9rem; font-weight: 700; text-transform: uppercase; letter-spacing: 0.08em; }}
    .code-libraries {{ display: flex; flex-wrap: wrap; gap: 0.45rem; }}
    .code-lib {{ background: #1e3a8a; color: #dbeafe; border-radius: 999px; padding: 0.2rem 0.6rem; font-size: 0.82rem; font-weight: 600; }}
    pre {{ margin: 0; padding: 1rem; overflow-x: auto; }}
    code {{ font-family: Menlo, Monaco, Consolas, monospace; font-size: 0.95rem; color: #e2e8f0; white-space: pre; }}
    img, video, iframe {{ width: 100%; border-radius: 14px; border: 1px solid #e2e8f0; background: #0f172a; }}
    img {{ max-height: 420px; object-fit: contain; background: white; }}
    video {{ max-height: 420px; }}
    .video-frame {{ position: relative; padding-top: 56.25%; }}
    .video-frame iframe {{ position: absolute; inset: 0; height: 100%; }}
    figcaption {{ margin-top: 0.65rem; color: #64748b; font-size: 0.95rem; }}
    footer {{ margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; }}
  </style>
</head>
<body>
  <main>
    <a class=\"back-link\" href=\"index.html\">← Back to tutorials</a>
    <h1>{html.escape(page.title)}</h1>
    <div class=\"tag-row\">{tags_html}</div>
        <div class=\"tag-row\">{libs_html}</div>
        {repo_html}
      {blocks_html}
    <footer>
      <p>Generated from {html.escape(page.source.name)}.</p>
    </footer>
  </main>
</body>
</html>
""",
        encoding="utf-8",
    )
def load_publications(source_dir: Path, output_dir: Path) -> list[PublicationEntry]:
    publications_source = source_dir / "publications"
    if not publications_source.exists():
        return []

    publications_output = output_dir / "publications"
    publications_output.mkdir(parents=True, exist_ok=True)

    entries: list[PublicationEntry] = []
    for metadata_path in sorted(publications_source.glob("*.json")):
        try:
            payload = json.loads(metadata_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            payload = {}
        entry = PublicationEntry(
            metadata_path=metadata_path,
            title=str(payload.get("title", "") or ""),
            authors=[str(item) for item in payload.get("authors", [])],
            abstract=str(payload.get("abstract", "") or ""),
            year=str(payload.get("year", "") or ""),
            tags=[str(item) for item in (payload.get("tags") or payload.get("keywords") or [])],
            pdf=str(payload.get("pdf", "") or ""),
        )
        entries.append(entry)
        shutil.copy2(metadata_path, publications_output / metadata_path.name)

    for pdf_path in sorted(publications_source.glob("*.pdf")):
        shutil.copy2(pdf_path, publications_output / pdf_path.name)

    return entries


def render_publications(entries: list[PublicationEntry], output_dir: Path) -> None:
    publications_dir = output_dir / "publications"
    publications_dir.mkdir(parents=True, exist_ok=True)

    cards: list[str] = []
    for entry in entries:
        title = html.escape(entry.title or "Untitled publication")
        authors = ", ".join(html.escape(author) for author in entry.authors) or ""
        abstract = html.escape(entry.abstract or "")
        year = html.escape(entry.year or "")
        tags_html = "".join(f'<span class="tag">{html.escape(tag)}</span>' for tag in entry.tags)
        metadata_name = html.escape(entry.metadata_path.name)
        links: list[str] = [f'<a href="{metadata_name}">Metadata</a>']
        if entry.pdf:
            links.append(f'<a href="{html.escape(entry.pdf)}">PDF</a>')
        elif any(publications_dir.glob("*.pdf")):
            links.append('<a href="blank.pdf">PDF</a>')

        details = " · ".join(links)
        meta_line = " · ".join(item for item in [authors, year] if item)
        cards.append(
            '<article class="card">'
            f'<h2>{title}</h2>'
            + (f'<p class="muted">{meta_line}</p>' if meta_line else '<p class="muted">Publication details will be added later.</p>')
            + (f'<div class="tag-row">{tags_html}</div>' if tags_html else '')
            + (f'<p>{abstract}</p>' if abstract else '<p>Abstract coming soon.</p>')
            + f'<p>{details}</p>'
            + '</article>'
        )

    if not cards:
        cards.append('<article class="card"><h2>Publications</h2><p>Publication metadata and PDFs will appear here.</p></article>')

    (publications_dir / "index.html").write_text(
        f'''<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Publications</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; background: #f8fafc; color: #0f172a; }}
    main {{ background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }}
    .card {{ border: 1px solid #e2e8f0; border-radius: 14px; padding: 1rem 1.25rem; margin: 1rem 0; background: #fff; }}
    .tag-row {{ display: flex; flex-wrap: wrap; gap: 0.5rem; margin: 0.75rem 0; }}
    .tag {{ background: #dbeafe; color: #1d4ed8; border-radius: 999px; padding: 0.25rem 0.7rem; font-size: 0.9rem; font-weight: 600; }}
    a {{ color: #2563eb; text-decoration: none; }}
    a:hover {{ text-decoration: underline; }}
    .muted {{ color: #64748b; font-size: 0.95rem; }}
  </style>
</head>
<body>
  <main>
    <a href="../index.html">← Back to tutorials</a>
    <h1>இ Publications</h1>
    <p class="muted">Metadata records and downloadable PDFs for CoolBox publications.</p>
    {''.join(cards)}
  </main>
</body>
</html>
''',
        encoding="utf-8",
    )


def render_index(pages: Iterable[TutorialPage], output_dir: Path, publications: list[PublicationEntry]) -> None:
    page_cards = []
    tag_map: dict[str, list[TutorialPage]] = {}

    for page in pages:
        rel_path = page.output_path.name if page.output_path else f"{page.slug}.html"
        tags_html = "".join(f'<span class="tag">{html.escape(tag)}</span>' for tag in page.tags)
        excerpt = html.escape(page.excerpt or "Open the tutorial to learn more.")
        page_cards.append(
            f'<article class="card"><h2><a href="{rel_path}">{html.escape(page.title)}</a></h2><p>{excerpt}</p><div class="tag-row">{tags_html}</div></article>'
        )
        for tag in page.tags:
            tag_map.setdefault(tag, []).append(page)

    tag_sections = []
    for tag in sorted(tag_map):
        links = " · ".join(
            f'<a href="{page.output_path.name if page.output_path else page.slug + ".html"}">{html.escape(page.title)}</a>'
            for page in sorted(tag_map[tag], key=lambda item: item.title.lower())
        )
        tag_sections.append(f'<li><strong>{html.escape(tag)}</strong>: {links}</li>')

    publications_section = (
        '<section>'
        '<h2>இ Publications</h2>'
        + ('<p class="muted">Browse metadata and blank starter PDFs for publications.</p>' if publications else '<p class="muted">No publications are available yet.</p>')
        + '<p><a href="publications/index.html">Open publications</a></p>'
        '</section>'
    )

    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "index.html").write_text(
        f"""<!DOCTYPE html>
<html lang=\"en\">
<head>
  <meta charset=\"UTF-8\">
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
  <title>CoolBox Tutorials</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; background: #f8fafc; color: #0f172a; }}
    main {{ background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }}
    .hero {{ margin-bottom: 2rem; }}
    .card {{ border: 1px solid #e2e8f0; border-radius: 14px; padding: 1rem 1.25rem; margin: 1rem 0; background: #fff; }}
    h1, h2 {{ color: #111827; }}
    a {{ color: #2563eb; text-decoration: none; }}
    a:hover {{ text-decoration: underline; }}
    p {{ color: #475569; line-height: 1.65; }}
    .tag-row {{ display: flex; flex-wrap: wrap; gap: 0.5rem; margin-top: 0.75rem; }}
    .tag {{ background: #dbeafe; color: #1d4ed8; border-radius: 999px; padding: 0.25rem 0.7rem; font-size: 0.9rem; font-weight: 600; }}
    .muted {{ color: #64748b; font-size: 0.95rem; }}
    footer {{ margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; }}
  </style>
</head>
<body>
  <main>
    <section class=\"hero\">
            <h1>☉ 𓂀 CoolBox Tutorials</h1>
      <p class=\"muted\">Static tutorials generated from `.tut` files with support for headings, media, links, and tags.</p>
    </section>
    {''.join(page_cards)}
        {publications_section}
    <section>
      <h2>Tags</h2>
      <ul>
        {''.join(tag_sections) or '<li>No tags defined yet.</li>'}
      </ul>
    </section>
        <footer>
            <p>𓁿 Meandering LLC © 2026</p>
        </footer>
  </main>
</body>
</html>
""",
        encoding="utf-8",
    )


def build_tutorial_site(source_dir: Path, output_dir: Path) -> None:
    if not source_dir.exists():
        raise FileNotFoundError(f"Tutorial source directory not found: {source_dir}")

    if output_dir.exists():
        shutil.rmtree(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    pages: list[TutorialPage] = []
    for tut_file in sorted(source_dir.rglob("*.tut")):
        page = parse_tutorial(tut_file)
        render_page(page, output_dir)
        pages.append(page)

    publications = load_publications(source_dir, output_dir)
    render_publications(publications, output_dir)
    render_index(pages, output_dir, publications)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Build static HTML pages from CoolBox .tut tutorial files.")
    parser.add_argument("source", nargs="?", default="tutorials", help="Directory containing .tut files")
    parser.add_argument("output", nargs="?", default="build/tutorials-site", help="Directory for generated HTML")
    args = parser.parse_args()

    build_tutorial_site(Path(args.source).resolve(), Path(args.output).resolve())
    print(f"Built tutorials site at {Path(args.output).resolve()}")
