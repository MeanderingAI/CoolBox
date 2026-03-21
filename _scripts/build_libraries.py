from build_libraries_bib import parse_bib_entries
import re
import html
import json
import shutil
import datetime
import warnings
from collections import defaultdict
from urllib.parse import urlparse
from dataclasses import dataclass, field
from typing import List, Optional
from pathlib import Path

# --- Data classes and utility functions (must be defined before use) ---
@dataclass
class Block:
    kind: str
    data: dict

@dataclass
class TutorialPage:
    source: Path
    title: str
    slug: str
    tags: List[str] = field(default_factory=list)
    libs: List[str] = field(default_factory=list)
    repo: str = ""
    blocks: List[Block] = field(default_factory=list)
    excerpt: str = ""
    output_path: Optional[Path] = None

@dataclass
class PublicationEntry:
    title: str
    authors: List[str]
    abstract: str
    year: str
    tags: List[str]
    pdf: str
    metadata_path: Path

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

def is_remote_url(src: str) -> bool:
    """Return True if src looks like an absolute URL (http/https)."""
    return src.startswith("http://") or src.startswith("https://")

def copy_asset(src: str, source_file: Path, output_dir: Path, asset_dir: Path) -> str:
    """Copy an asset file to the output directory and return its relative path."""
    if is_remote_url(src):
        return src
    src_path = Path(src)
    if not src_path.is_absolute():
        src_path = source_file.parent / src_path
    if not src_path.exists():
        raise FileNotFoundError(f"Asset not found: {src}")

    dest_path = asset_dir / src_path.name
    dest_path.parent.mkdir(parents=True, exist_ok=True)
    dest_path.write_bytes(src_path.read_bytes())

    return str(dest_path.relative_to(output_dir))

def parse_tutorial(source_file: Path) -> TutorialPage:
    blocks = []
    tags = []
    libs = []
    repo = ""
    paragraph_lines = []
    title = source_file.stem.replace("-", " ").title()
    excerpt = ""
    in_code_block = False
    code_language = "text"
    code_libraries = []
    code_lines = []

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

    def flush_paragraph():
        nonlocal excerpt
        if not paragraph_lines:
            return
        text = " ".join(line.strip() for line in paragraph_lines if line.strip())
        if text:
            blocks.append(Block("paragraph", {"text": text}))
            if not excerpt:
                excerpt = text
        paragraph_lines.clear()

    def flush_code_block():
        nonlocal in_code_block, code_language, code_libraries, code_lines
        if not in_code_block:
            return
        blocks.append(Block("code", {
            "language": code_language,
            "libraries": code_libraries.copy(),
            "code": "\n".join(code_lines).rstrip("\n"),
        }))
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
    return TutorialPage(source=source_file, title=title, slug=slugify(source_file.stem), tags=tags, libs=libs, repo=repo, blocks=blocks)

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

def render_page(page: TutorialPage, output_dir: Path, publications: list[PublicationEntry] | None = None) -> None:
    asset_dir = output_dir / "assets" / page.slug
    blocks_html = "\n      ".join(render_block(block, page, output_dir, asset_dir) for block in page.blocks)
    tags_html = "".join(
        f'<a href="../tags/{slugify(tag)}.html" class="tag">{html.escape(tag)}</a>'
        for tag in page.tags
    )
    libs_html = "".join(f'<span class="tag lib-tag">{html.escape(lib)}</span>' for lib in page.libs)
    repo_html = f'<p><a class="inline-link repo-link" href="{html.escape(page.repo)}">Repository</a></p>' if page.repo else ""
    output_file = output_dir / f"{page.slug}.html"
    references_dir = output_dir / "references"
    references_dir.mkdir(parents=True, exist_ok=True)
    bib_dir = Path(__file__).parent.parent / "bib"
    bib_items = []
    if bib_dir.exists():
        for bib_file in sorted(bib_dir.glob("*.bib")):
            content = bib_file.read_text(encoding="utf-8")
            for entry in parse_bib_entries(content):
                bib_items.append({"file": bib_file.name, **entry})

    # Build a map of normalized publication titles to their HTML filenames
    pub_title_to_html = {}
    for pub in (publications or []):
        norm_title = (pub.title or "").strip().lower().lstrip('{').rstrip('}')
        pub_title_to_html[norm_title] = pub.metadata_path.stem if pub.metadata_path else None

    def bib_entry_link(item):
        # Try to link to a publication if the title matches
        norm_title = (item["title"] or "").strip().lower().lstrip('{').rstrip('}')
        pub_html = pub_title_to_html.get(norm_title)
        title_html = html.escape(item["title"] or item["key"])
        if pub_html:
            return f'<a href="../publications/{pub_html}">{title_html}</a>'
        return title_html

    bib_list_html = "\n".join(
        f'<li><strong>{bib_entry_link(item)}</strong> <span class="muted">{html.escape(item["author"] or "")}</span> <span class="muted">({html.escape(item["file"])})</span></li>'
        for item in bib_items
    )
    (references_dir / "index.html").write_text(
        f"""<!DOCTYPE html>
<html lang=\"en\">
<head>
    <meta charset=\"UTF-8\">
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
    <title>BibTeX References</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; background: #f8fafc; color: #0f172a; }}
        main {{ background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }}
        h1 {{ color: #111827; }}
        a {{ color: #2563eb; text-decoration: none; }}
        a:hover {{ text-decoration: underline; }}
        .muted {{ color: #64748b; font-size: 0.95rem; }}
        ul {{ margin: 1.5rem 0; }}
        li {{ margin: 1rem 0; }}
        footer {{ margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; }}
    </style>
</head>
<body>
    <main>
        <a href=\"../index.html\" class=\"back-arrow\" title=\"Back to home\">&#8592; Back to home</a>
        <h1>BibTeX References</h1>
        <ul>
        {bib_list_html or '<li>No BibTeX entries found.</li>'}
        </ul>
        <footer>
            <p>𓁿 Meandering LLC © 2026</p>
        </footer>
    </main>
</body>
</html>""",
        encoding="utf-8"
    )

def load_publications(bib_dir: Path) -> list[PublicationEntry]:
    """Load publications from .bib files in the given bib directory."""
    publications = []
    if not bib_dir.exists():
        print(f"No bibliography directory found at {bib_dir}.")
        return publications

    for bib_file in sorted(bib_dir.glob("*.bib")):
        try:
            content = bib_file.read_text(encoding="utf-8")
            for entry in parse_bib_entries(content):
                publications.append(PublicationEntry(
                    title=entry.get("title"),
                    authors=entry.get("author", "").split(","),
                    abstract=entry.get("abstract"),
                    year=entry.get("year"),
                    tags=entry.get("keywords", "").split(","),
                    metadata_path=bib_file,
                    pdf=entry.get("pdf"),
                ))
        except Exception as e:
            print(f"Error parsing {bib_file}: {e}")

    return publications

def render_publications(entries: list[PublicationEntry], output_dir: Path) -> None:
    publications_dir = output_dir / "publications"
    publications_dir.mkdir(parents=True, exist_ok=True)
    cards = []
    for entry in entries:
        title = html.escape(entry.title or "Untitled publication")
        authors = ", ".join(html.escape(author) for author in entry.authors) or ""
        abstract = html.escape(entry.abstract or "")
        year = html.escape(entry.year or "")
        tags_html = "".join(f'<span class="tag">{html.escape(tag)}</span>' for tag in entry.tags)
        metadata_name = html.escape(entry.metadata_path.name)
        links = [f'<a href="{metadata_name}">Metadata</a>']
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
    # Add references section at the bottom
    bib_dir = Path(__file__).parent.parent / "bib"
    references_dir = output_dir / "references"
    references_dir.mkdir(parents=True, exist_ok=True)
    reference_links = []
    if bib_dir.exists():
        for bib_file in sorted(bib_dir.glob("*.bib")):
            # Only link to the per-bib HTML page, no mention of .bib or BibTeX
            display_name = bib_file.stem.replace('_', ' ').capitalize()
            reference_links.append(f'<li><a href="../references/{bib_file.stem}.html">{display_name}</a></li>')
    references_section = ""
    if reference_links:
        references_section = f'<section><h2>References</h2><ul>{"".join(reference_links)}</ul><p><a href="../references/index.html">Browse all references</a></p></section>'
    if not cards:
        cards.append('<article class="card"><h2>Publications</h2><p>Publication metadata and PDFs will appear here.</p></article>')
    (publications_dir / "index.html").write_text(
        f"""<!DOCTYPE html>
<html lang=\"en\">
<head>
    <meta charset=\"UTF-8\">
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
    <title>CoolBox Publications</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; background: #f8fafc; color: #0f172a; }}
        main {{ background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }}
        .hero {{ margin-bottom: 2rem; display: flex; align-items: center; }}
        .back-arrow {{ font-size: 2rem; margin-right: 1.2rem; color: #2563eb; text-decoration: none; font-weight: bold; line-height: 1; }}
        .card {{ border: 1px solid #e2e8f0; border-radius: 14px; padding: 1rem 1.25rem; margin: 1rem 0; background: #fff; display: flex; justify-content: space-between; align-items: flex-start; }}
        h1, h2 {{ color: #111827; }}
        a {{ color: #2563eb; text-decoration: none; }}
        a:hover {{ text-decoration: underline; }}
        p {{ color: #475569; line-height: 1.65; }}
        .tag-row {{ display: flex; flex-wrap: wrap; gap: 0.5rem; margin-top: 0.75rem; }}
        .tag {{ background: #dbeafe; color: #1d4ed8; border-radius: 999px; padding: 0.25rem 0.7rem; font-size: 0.9rem; font-weight: 600; }}
        .muted {{ color: #64748b; font-size: 0.95rem; }}
        .info-icon {{
            font-size: 0.9rem;
            margin-left: 0.5rem;
            color: #2563eb;
            text-decoration: none;
        }}
        .info-icon:hover {{
            text-decoration: underline;
        }}
        footer {{ margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; }}
    </style>
</head>
<body>
    <main>
        <section class=\"hero\">
            <h1>☉ Publications
                <a href=\"../info.html\" class=\"info-icon\" title=\"More information\">ⓘ</a>
            </h1>
        </section>
        {''.join(cards)}
        {references_section}
        <footer>
            <p>𓁿 Meandering LLC © 2026</p>
        </footer>
    </main>
</body>
</html>""",
        encoding="utf-8"
    )

def render_index(pages, output_dir, publications):
    tutorials_with_dates = []
    for page in pages:
        if page.output_path and (output_dir / page.output_path.name).exists():
            mtime = (output_dir / page.output_path.name).stat().st_mtime
        else:
            mtime = page.source.stat().st_mtime
        tutorials_with_dates.append((page, mtime))
    tutorials_with_dates.sort(key=lambda x: -x[1])
    def format_date(ts):
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=DeprecationWarning)
            return datetime.datetime.fromtimestamp(ts, tz=datetime.UTC).strftime('%Y-%m-%d')
    page_cards = []
    for page, mtime in tutorials_with_dates:
        tags_html = "".join(f'<span class="tag">{html.escape(tag)}</span>' for tag in page.tags)
        excerpt = html.escape(page.excerpt or "Open the tutorial to learn more.")
        date_str = format_date(mtime)
        page_cards.append(
            f'<article class="card" style="display: flex; justify-content: space-between; align-items: flex-start;">'
            f'<div style="flex: 1 1 auto; min-width: 0;">'
            f'<h2><a href="{page.output_path.name if page.output_path else page.slug + ".html"}">{html.escape(page.title)}</a></h2>'
            f'<p>{excerpt}</p>'
            f'<div class="tag-row">{tags_html}</div>'
            f'</div>'
            f'<div style="flex: 0 0 auto; align-self: flex-start; color: #64748b; font-size: 0.95rem; font-weight: 600; margin-left: 1.5rem;">{date_str}</div>'
            f'</article>'
        )
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
        .hero {{ margin-bottom: 2rem; display: flex; align-items: center; }}
        .back-arrow {{ font-size: 2rem; margin-right: 1.2rem; color: #2563eb; text-decoration: none; font-weight: bold; line-height: 1; }}
        .card {{ border: 1px solid #e2e8f0; border-radius: 14px; padding: 1rem 1.25rem; margin: 1rem 0; background: #fff; display: flex; justify-content: space-between; align-items: flex-start; }}
        h1, h2 {{ color: #111827; }}
        a {{ color: #2563eb; text-decoration: none; }}
        a:hover {{ text-decoration: underline; }}
        p {{ color: #475569; line-height: 1.65; }}
        .tag-row {{ display: flex; flex-wrap: wrap; gap: 0.5rem; margin-top: 0.75rem; }}
        .tag {{ background: #dbeafe; color: #1d4ed8; border-radius: 999px; padding: 0.25rem 0.7rem; font-size: 0.9rem; font-weight: 600; }}
        .muted {{ color: #64748b; font-size: 0.95rem; }}
        .info-icon {{
            font-size: 0.9rem;
            margin-left: 0.5rem;
            color: #2563eb;
            text-decoration: none;
        }}
        .info-icon:hover {{
            text-decoration: underline;
        }}
        footer {{ margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; }}
    </style>
</head>
<body>
    <main>
        <section class=\"hero\">\n        <a href=\"../index.html\" class=\"back-arrow\" title=\"Back to documentation\">&#8592;</a>\n        <h1 style=\"display: flex; align-items: center; gap: 0.5rem; margin: 0;\">𓂀 Tutorials</h1>\n    </section>\n    {''.join(page_cards)}\n    <footer>\n        <p>𓁿 Meandering LLC © 2026</p>\n    </footer>\n    </main>\n</body>\n</html>\n""",
        encoding="utf-8"
    )

def _generate_tag_pages(pages: list[TutorialPage], output_dir: Path) -> None:
    """Generate per-tag HTML pages and a tags index from tutorial pages."""
    tag_to_pages = defaultdict(list)
    for page in pages:
        for tag in page.tags:
            tag_to_pages[tag].append(page)

    tags_dir = output_dir / "tags"
    tags_dir.mkdir(parents=True, exist_ok=True)

    for tag, tag_pages in tag_to_pages.items():
        tag_slug = slugify(tag)
        tag_file = tags_dir / f"{tag_slug}.html"
        tag_cards = []
        for page in tag_pages:
            tag_cards.append(
                f'<article class="card">'
                f'<h2><a href="../tutorials/{page.output_path.name if page.output_path else page.slug + ".html"}">{html.escape(page.title)}</a></h2>'
                f'<p>{html.escape(page.excerpt or "Open the tutorial to learn more.")}</p>'
            )
        tag_file.write_text("\n".join(tag_cards), encoding="utf-8")

    # Generate a tags index page
    tags_index = output_dir / "tags.html"
    tag_links = [f'<li><a href="tags/{slugify(tag)}.html">{html.escape(tag)} <span class="muted">({len(tag_pages)})</span></a></li>' for tag, tag_pages in sorted(tag_to_pages.items())]
    tags_index.write_text(
        f'''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Tags Index</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; background: #f8fafc; color: #0f172a; }}
        main {{ background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }}
        h1 {{ color: #111827; }}
        a {{ color: #2563eb; text-decoration: none; }}
        a:hover {{ text-decoration: underline; }}
        .muted {{ color: #64748b; font-size: 0.95rem; }}
        ul {{ margin: 1.5rem 0; }}
        li {{ margin: 1rem 0; }}
        footer {{ margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; }}
    </style>
</head>
<body>
    <main>
        <a href="index.html" class="back-arrow" title="Back to home">&#8592; Back to home</a>
        <h1>Tags Index</h1>
        <ul>
        {''.join(tag_links)}
        </ul>
        <footer>
            <p>𓁿 Meandering LLC © 2026</p>
        </footer>
    </main>
</body>
</html>''',
        encoding="utf-8"
    )

def build_tutorial_site(source_dir: Path, output_dir: Path) -> None:
    """Builds the static HTML site for tutorials and publications."""
    tutorials_dir = source_dir
    pages = []
    for tut_file in sorted(tutorials_dir.glob("*.tut")):
        try:
            print(f"Processing tutorial: {tut_file}")
            page = parse_tutorial(tut_file)
            page.output_path = Path(f"{page.slug}.html")
            pages.append(page)
        except Exception as e:
            print(f"Error parsing {tut_file}: {e}")

    # Ensure tutorials are checked in the .site folder
    site_tutorials_dir = output_dir / "tutorials"
    site_tutorials_dir.mkdir(parents=True, exist_ok=True)

    # Load publications from the project-root bib/ directory
    project_root = Path(__file__).parent.parent
    bib_dir = project_root / "bib"
    publications = load_publications(bib_dir)
    print(f"Loaded {len(publications)} publication(s) from {bib_dir}")

    # Render each tutorial page (pass publications for cross-linking)
    for page in pages:
        render_page(page, site_tutorials_dir, publications)

    # Render publications
    render_publications(publications, site_tutorials_dir)
    print(f"Rendered publications index at {site_tutorials_dir / 'publications' / 'index.html'}")

    # Generate tag pages
    _generate_tag_pages(pages, output_dir)
    print(f"Generated tag pages at {output_dir / 'tags'}")

    # Generate tutorials index (original, feature-rich version)
    tutorials_with_dates = []
    for page in pages:
        if page.output_path and (site_tutorials_dir / page.output_path.name).exists():
            mtime = (site_tutorials_dir / page.output_path.name).stat().st_mtime
        else:
            mtime = page.source.stat().st_mtime
        tutorials_with_dates.append((page, mtime))
    tutorials_with_dates.sort(key=lambda x: -x[1])
    def format_date(ts):
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=DeprecationWarning)
            return datetime.datetime.fromtimestamp(ts).strftime('%Y-%m-%d')
    page_cards = []
    for page, mtime in tutorials_with_dates:
        tags_html = "".join(f'<span class="tag">{html.escape(tag)}</span>' for tag in page.tags)
        excerpt = html.escape(page.excerpt or "Open the tutorial to learn more.")
        date_str = format_date(mtime)
        page_cards.append(
            f'<article class="card" style="display: flex; justify-content: space-between; align-items: flex-start;">'
            f'<div style="flex: 1 1 auto; min-width: 0;">'
            f'<h2><a href="{page.output_path.name if page.output_path else page.slug + ".html"}">{html.escape(page.title)}</a></h2>'
            f'<p>{excerpt}</p>'
            f'<div class="tag-row">{tags_html}</div>'
            f'</div>'
            f'<div style="flex: 0 0 auto; align-self: flex-start; color: #64748b; font-size: 0.95rem; font-weight: 600; margin-left: 1.5rem;">{date_str}</div>'
            f'</article>'
        )
    (site_tutorials_dir / "index.html").write_text(
        f"""<!DOCTYPE html>
<html lang=\"en\">
<head>
    <meta charset=\"UTF-8\">
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
    <title>CoolBox Tutorials</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; background: #f8fafc; color: #0f172a; }}
        main {{ background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }}
        .hero {{ margin-bottom: 2rem; display: flex; align-items: center; }}
        .back-arrow {{ font-size: 2rem; margin-right: 1.2rem; color: #2563eb; text-decoration: none; font-weight: bold; line-height: 1; }}
        .card {{ border: 1px solid #e2e8f0; border-radius: 14px; padding: 1rem 1.25rem; margin: 1rem 0; background: #fff; display: flex; justify-content: space-between; align-items: flex-start; }}
        h1, h2 {{ color: #111827; }}
        a {{ color: #2563eb; text-decoration: none; }}
        a:hover {{ text-decoration: underline; }}
        p {{ color: #475569; line-height: 1.65; }}
        .tag-row {{ display: flex; flex-wrap: wrap; gap: 0.5rem; margin-top: 0.75rem; }}
        .tag {{ background: #dbeafe; color: #1d4ed8; border-radius: 999px; padding: 0.25rem 0.7rem; font-size: 0.9rem; font-weight: 600; }}
        .muted {{ color: #64748b; font-size: 0.95rem; }}
        .info-icon {{
            font-size: 0.9rem;
            margin-left: 0.5rem;
            color: #2563eb;
            text-decoration: none;
        }}
        .info-icon:hover {{
            text-decoration: underline;
        }}
        footer {{ margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; }}
    </style>
</head>
<body>
    <main>
        <section class=\"hero\">\n        <a href=\"../index.html\" class=\"back-arrow\" title=\"Back to documentation\">&#8592;</a>\n        <h1 style=\"display: flex; align-items: center; gap: 0.5rem; margin: 0;\">𓂀 Tutorials</h1>\n    </section>\n    {''.join(page_cards)}\n    <footer>\n        <p>𓁿 Meandering LLC © 2026</p>\n    </footer>\n    </main>\n</body>\n</html>\n""",
        encoding="utf-8"
    )
