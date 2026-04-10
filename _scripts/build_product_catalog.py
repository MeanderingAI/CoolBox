#!/usr/bin/env python3

from __future__ import annotations

import html
import json
import re
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable

from build_product_assets import PRODUCTS


BACKAGES_ROOT = Path("_libraries/backages")
APPS_ROOT = Path("apps")

CATEGORY_SUMMARIES = {
    "CHEMISTRY": "Chemistry and molecular tooling for modeling domain data and workflows.",
    "DATASTRUCTURE": "Core data structures, containers, and supporting algorithms used across the stack.",
    "ELECTRONICS": "Electronics-oriented components for circuit and device level modeling.",
    "GRAPHICS": "Reusable graphics, windowing, and application-shell components for native interfaces.",
    "IO": "Input/output, networking, and exchange layers for moving data in and out of CoolBox.",
    "MISC": "Shared utilities and infrastructure that do not fit a narrower subsystem.",
    "ML": "Machine-learning libraries covering modeling, statistics, inference, and optimization.",
    "PARSER": "Parsing and language-processing components for structured text and syntax handling.",
    "SECURITY": "Security-oriented building blocks for hashes, crypto, and defensive utilities.",
    "SP": "Signal-processing libraries for transforms, synthesis, and numerical audio workflows.",
    "TOOLS": "Developer tooling, test infrastructure, and support code used throughout the repository.",
}

DESCRIPTION_TEMPLATES = {
    "CHEMISTRY": "Chemistry component focused on {topic}.",
    "DATASTRUCTURE": "Data-structure component focused on {topic}.",
    "ELECTRONICS": "Electronics component focused on {topic}.",
    "GRAPHICS": "Graphics and UI component focused on {topic}.",
    "IO": "I/O component focused on {topic}.",
    "MISC": "Shared utility component focused on {topic}.",
    "ML": "Machine-learning component focused on {topic}.",
    "PARSER": "Parsing component focused on {topic}.",
    "SECURITY": "Security component focused on {topic}.",
    "SP": "Signal-processing component focused on {topic}.",
    "TOOLS": "Tooling component focused on {topic}.",
}

SECTION_ALIASES = {
    "generalized_linear_model": "Generalized Linear Models",
    "decision_tree": "Decision Trees",
    "distribution": "Statistical Distributions",
    "multi_arm_bandit": "Multi-Arm Bandit",
    "support_vector_machine": "Support Vector Machine",
    "dimensionality_reduction": "Dimensionality Reduction",
    "self_organizing_maps": "Self Organizing Maps",
    "hidden_markov_model": "Hidden Markov Model",
    "deep_learning": "Deep Learning",
    "knn": "K Nearest Neighbors",
    "latent_sentiment_analysis": "Latent Sentiment Analysis",
    "marked_point_process": "Marked Point Process",
    "bayesian_network_ai": "Bayesian Network",
    "bayesian_network_db": "Bayesian Network",
    "gabor_patches": "Gabor Patches",
    "tracker": "Tracker",
    "fourier_tranforms": "Fourier Transforms",
}


@dataclass
class LibraryEntry:
    slug: str
    name: str
    category: str
    relative_path: str
    summary: str
    summary_source: str
    targets: list[str]
    dependencies: list[str]
    header_count: int
    source_count: int
    test_count: int
    has_tests: bool
    primary_headers: list[str]
    related_products: list[str]
    related_apps: list[str]
    page_url: str


@dataclass
class ConsumerEntry:
    kind: str
    slug: str
    title: str
    source_dir: str
    summary: str
    dependencies: list[str]
    page_url: str


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def html_page(title: str, body: str) -> str:
    return f"""<!DOCTYPE html>
<html lang=\"en\">
<head>
  <meta charset=\"UTF-8\">
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
  <title>{html.escape(title)}</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 1100px; padding: 0 1rem; color: #0f172a; background: #f8fafc; }}
    main {{ background: #ffffff; border-radius: 18px; padding: 2rem; box-shadow: 0 14px 40px rgba(15, 23, 42, 0.08); }}
    h1, h2, h3 {{ color: #111827; }}
    p, li {{ color: #475569; }}
    a {{ color: #1d4ed8; text-decoration: none; font-weight: 600; }}
    a:hover {{ text-decoration: underline; }}
    code {{ background: #e2e8f0; padding: 0.15rem 0.35rem; border-radius: 6px; font-family: Consolas, monospace; }}
    .muted {{ color: #64748b; }}
    .grid {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 1rem; }}
    .card {{ border: 1px solid #dbe4f0; border-radius: 14px; padding: 1rem; background: linear-gradient(180deg, #ffffff 0%, #f8fbff 100%); }}
    .pill {{ display: inline-block; margin-right: 0.4rem; margin-bottom: 0.4rem; padding: 0.25rem 0.55rem; border-radius: 999px; background: #dbeafe; color: #1d4ed8; font-size: 0.88rem; font-weight: 600; border: 0; cursor: pointer; }}
    .section {{ margin-top: 1.75rem; }}
    ul.compact li {{ margin: 0.45rem 0; }}
    table {{ border-collapse: collapse; width: 100%; margin-top: 0.75rem; }}
    th, td {{ text-align: left; padding: 0.7rem 0.75rem; border-bottom: 1px solid #e2e8f0; vertical-align: top; }}
  </style>
</head>
<body>
  <main>
{body}
  </main>
</body>
</html>
"""


def normalize_key(value: str) -> str:
    lowered = value.lower().replace("&", " and ")
    lowered = lowered.replace("_", " ").replace("-", " ").replace("/", " ")
    lowered = re.sub(r"[^a-z0-9\s]", " ", lowered)
    lowered = re.sub(r"\s+", " ", lowered).strip()
    words: list[str] = []
    for word in lowered.split():
        words.append(word[:-1] if len(word) > 4 and word.endswith("s") else word)
    return " ".join(words)


def humanize_name(name: str) -> str:
    pieces = [piece for piece in re.split(r"[_\-/]", name) if piece]
    rendered: list[str] = []
    for piece in pieces:
        upper = piece.upper()
        if upper in {"ML", "IO", "SP", "GUI", "JSON", "CSV", "API", "SVD", "PCA", "KNN", "UMAP", "LSP"}:
            rendered.append(upper)
        elif piece.isupper() and len(piece) <= 5:
            rendered.append(piece)
        else:
            rendered.append(piece.capitalize())
    return " ".join(rendered) if rendered else name


def read_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8")


def extract_markdown_paragraph(text: str) -> str:
    paragraph: list[str] = []
    in_code_block = False
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if line.startswith("```"):
            in_code_block = not in_code_block
            continue
        if in_code_block:
            continue
        if not line:
            if paragraph:
                break
            continue
        if line.startswith("#"):
            continue
        if line.startswith(("- ", "* ", "1. ", "2. ", "3. ")) and not paragraph:
            continue
        paragraph.append(line)
    return " ".join(paragraph).strip()


def parse_markdown_sections(text: str) -> dict[str, str]:
    sections: dict[str, str] = {}
    current_heading: str | None = None
    current_lines: list[str] = []

    def flush() -> None:
        if current_heading and current_lines:
            summary = extract_markdown_paragraph("\n".join(current_lines))
            if summary:
                sections[normalize_key(current_heading)] = summary

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if line.startswith("## ") or line.startswith("### "):
            flush()
            current_heading = line.split(" ", 1)[1].strip()
            current_lines = []
            continue
        if current_heading is not None:
            current_lines.append(raw_line)
    flush()
    return sections


def parse_cmake_targets(cmake_text: str) -> list[str]:
    return sorted(set(re.findall(r"add_library\s*\(\s*([A-Za-z0-9_:\-.]+)", cmake_text)))


def parse_cmake_executables(cmake_text: str) -> list[str]:
    return sorted(set(re.findall(r"add_executable\s*\(\s*([A-Za-z0-9_:\-.]+)", cmake_text)))


def parse_cmake_dependencies(cmake_text: str) -> list[str]:
    dependencies: set[str] = set()
    for match in re.finditer(r"target_link_libraries\s*\((.*?)\)", cmake_text, re.S):
        body = re.sub(r"#.*", "", match.group(1))
        tokens = re.findall(r"[A-Za-z0-9_:\-.]+", body)
        for token in tokens[1:]:
            if token in {"PUBLIC", "PRIVATE", "INTERFACE"}:
                continue
            if token.startswith("$"):
                continue
            dependencies.add(token)
    return sorted(dependencies)


def parse_target_dependencies(cmake_text: str) -> dict[str, list[str]]:
    target_dependencies: dict[str, list[str]] = {}
    for match in re.finditer(r"target_link_libraries\s*\((.*?)\)", cmake_text, re.S):
        body = re.sub(r"#.*", "", match.group(1))
        tokens = re.findall(r"[A-Za-z0-9_:\-.]+", body)
        if not tokens:
            continue
        target_dependencies.setdefault(tokens[0], [])
        for token in tokens[1:]:
            if token in {"PUBLIC", "PRIVATE", "INTERFACE"}:
                continue
            if token.startswith("$"):
                continue
            target_dependencies[tokens[0]].append(token)
    return {key: sorted(set(values)) for key, values in target_dependencies.items()}


def extract_metadata_summaries(package_dir: Path) -> list[str]:
    summaries: list[str] = []
    for source_file in package_dir.rglob("*"):
        if source_file.suffix.lower() not in {".h", ".hpp", ".hh", ".c", ".cc", ".cpp", ".cxx"}:
            continue
        text = read_text(source_file)
        for macro_name in ("LIBRARY_METADATA", "LIBRARY_DOC"):
            for match in re.finditer(rf"{macro_name}\s*\((.*?)\)\s*;", text, re.S):
                strings = re.findall(r'"([^"]+)"', match.group(1))
                if macro_name == "LIBRARY_METADATA" and len(strings) >= 3:
                    summaries.append(strings[2])
                elif macro_name == "LIBRARY_DOC" and strings:
                    summaries.append(strings[0])
    return summaries


def summarize_from_structure(category: str, relative_path: str) -> str:
    topic = humanize_name(relative_path.split("/")[-1]).lower()
    template = DESCRIPTION_TEMPLATES.get(category, "Reusable CoolBox component focused on {topic}.")
    return template.format(topic=topic)


def library_name_candidates(relative_path: str, targets: Iterable[str]) -> list[str]:
    parts = [part for part in relative_path.split("/") if part]
    candidates = [parts[-1], humanize_name(parts[-1])]
    if len(parts) > 1:
        candidates.append(humanize_name(parts[-2]))
    candidates.extend(targets)
    alias = SECTION_ALIASES.get(parts[-1])
    if alias:
        candidates.append(alias)
    return [candidate for candidate in candidates if candidate]


def select_summary(package_dir: Path, category: str, relative_path: str, targets: list[str], readme_sections: dict[str, str]) -> tuple[str, str]:
    readme_candidates = sorted([candidate for candidate in package_dir.glob("README*") if candidate.is_file()], key=lambda candidate: candidate.name)
    for readme_path in readme_candidates:
        paragraph = extract_markdown_paragraph(read_text(readme_path))
        if paragraph:
            return paragraph, f"local README ({readme_path.name})"

    metadata_summaries = extract_metadata_summaries(package_dir)
    if metadata_summaries:
        return metadata_summaries[0], "embedded library metadata"

    for candidate in library_name_candidates(relative_path, targets):
        summary = readme_sections.get(normalize_key(candidate))
        if summary:
            return summary, "root README section"

    return summarize_from_structure(category, relative_path), "generated from package structure"


def build_product_index() -> dict[str, list[str]]:
    index: dict[str, list[str]] = {}
    for product in PRODUCTS:
        for library in product.related_libraries:
            index.setdefault(library, []).append(product.title)
        index.setdefault(Path(product.source_dir).name, []).append(product.title)
    return {key: sorted(set(values)) for key, values in index.items()}


def build_product_entries() -> list[ConsumerEntry]:
    return [
        ConsumerEntry(
            kind="product",
            slug=product.slug,
            title=product.title,
            source_dir=product.source_dir,
            summary=product.summary,
            dependencies=list(product.related_libraries),
            page_url=f"../products/{product.slug}/index.html",
        )
        for product in PRODUCTS
    ]


def summarize_app(relative_path: str, target: str, dependencies: list[str]) -> str:
    if dependencies:
        return f"Application target {target} under {relative_path} built on {', '.join(dependencies[:3])}."
    return f"Application target {target} under {relative_path}."


def build_app_entries(project_root: Path) -> list[ConsumerEntry]:
    apps_dir = project_root / APPS_ROOT
    if not apps_dir.exists():
        return []

    entries: list[ConsumerEntry] = []
    for cmake_path in sorted(apps_dir.rglob("CMakeLists.txt")):
        if cmake_path == apps_dir / "CMakeLists.txt":
            continue
        relative_dir = cmake_path.parent.relative_to(project_root).as_posix()
        cmake_text = read_text(cmake_path)
        executables = parse_cmake_executables(cmake_text)
        dependency_map = parse_target_dependencies(cmake_text)
        for executable in executables:
            dependencies = dependency_map.get(executable, [])
            entries.append(
                ConsumerEntry(
                    kind="app",
                    slug=executable,
                    title=humanize_name(executable),
                    source_dir=relative_dir,
                    summary=summarize_app(relative_dir, executable, dependencies),
                    dependencies=dependencies,
                    page_url="",
                )
            )
    return entries


def build_app_index(project_root: Path) -> dict[str, list[str]]:
    index: dict[str, list[str]] = {}
    for app in build_app_entries(project_root):
        for dependency in app.dependencies:
            index.setdefault(dependency, []).append(app.title)
    return {key: sorted(set(values)) for key, values in index.items()}


def build_library_entries(project_root: Path) -> list[LibraryEntry]:
    backages_dir = project_root / BACKAGES_ROOT
    readme_sections = parse_markdown_sections(read_text(project_root / "README.md"))
    product_index = build_product_index()
    app_index = build_app_index(project_root)
    entries: list[LibraryEntry] = []

    for cmake_path in sorted(backages_dir.rglob("CMakeLists.txt")):
        package_dir = cmake_path.parent
        relative_dir = package_dir.relative_to(backages_dir).as_posix()
        parts = relative_dir.split("/")
        if len(parts) < 2:
            continue
        cmake_text = read_text(cmake_path)
        targets = parse_cmake_targets(cmake_text)
        if not targets:
            continue

        headers = sorted({candidate.relative_to(package_dir).as_posix() for candidate in package_dir.rglob("*") if candidate.is_file() and candidate.suffix.lower() in {".h", ".hpp", ".hh"}})
        sources = sorted({candidate.relative_to(package_dir).as_posix() for candidate in package_dir.rglob("*") if candidate.is_file() and candidate.suffix.lower() in {".c", ".cc", ".cpp", ".cxx"}})
        tests = [path for path in sources if path.startswith("tests/") or "/tests/" in path or path.startswith("test_")]
        summary, summary_source = select_summary(package_dir, parts[0], relative_dir, targets, readme_sections)
        key = parts[-1]
        entries.append(
            LibraryEntry(
                slug=relative_dir.replace("/", "-"),
                name=humanize_name(key),
                category=parts[0],
                relative_path=relative_dir,
                summary=summary,
                summary_source=summary_source,
                targets=targets,
                dependencies=parse_cmake_dependencies(cmake_text),
                header_count=len(headers),
                source_count=len([path for path in sources if path not in tests]),
                test_count=len(tests),
                has_tests=bool(tests or (package_dir / "tests").exists()),
                primary_headers=headers[:6],
                related_products=product_index.get(key, []),
                related_apps=app_index.get(key, []),
                page_url=f"libraries/{relative_dir}/index.html",
            )
        )
    return entries


def write_library_page(site_dir: Path, entry: LibraryEntry) -> None:
    target_dir = site_dir / "solutions" / "libraries" / Path(entry.relative_path)
    ensure_dir(target_dir)

    target_badges = " ".join(f'<span class="pill">{html.escape(target)}</span>' for target in entry.targets)
    dependency_items = "".join(f"<li><code>{html.escape(dep)}</code></li>" for dep in entry.dependencies[:12])
    header_items = "".join(f"<li><code>{html.escape(header)}</code></li>" for header in entry.primary_headers)
    product_items = "".join(f"<li>{html.escape(product)}</li>" for product in entry.related_products)
    app_items = "".join(f"<li>{html.escape(app)}</li>" for app in entry.related_apps)

    body = f"""
    <h1>{html.escape(entry.name)}</h1>
    <p>{html.escape(entry.summary)}</p>
    <p class=\"muted\">Category: <code>{html.escape(entry.category)}</code> · Package: <code>{html.escape(entry.relative_path)}</code> · Summary source: {html.escape(entry.summary_source)}</p>
    <div class=\"section\">{target_badges}</div>
    <div class=\"section\">
      <h2>Build Surface</h2>
      <table>
        <tr><th>Targets</th><td>{', '.join(f'<code>{html.escape(target)}</code>' for target in entry.targets)}</td></tr>
        <tr><th>Headers</th><td>{entry.header_count}</td></tr>
        <tr><th>Sources</th><td>{entry.source_count}</td></tr>
        <tr><th>Tests</th><td>{entry.test_count if entry.has_tests else 'No dedicated test sources detected'}</td></tr>
      </table>
    </div>
    <div class=\"section\">
      <h2>Primary Headers</h2>
      <ul class=\"compact\">{header_items or '<li class="muted">No public headers were detected.</li>'}</ul>
    </div>
    <div class=\"section\">
      <h2>Dependencies</h2>
      <ul class=\"compact\">{dependency_items or '<li class="muted">No explicit target_link_libraries entries were parsed.</li>'}</ul>
    </div>
    <div class=\"section\">
      <h2>Used By Products</h2>
      <ul class=\"compact\">{product_items or '<li class="muted">No product mapping has been declared for this library yet.</li>'}</ul>
    </div>
    <div class=\"section\">
      <h2>Used By Apps</h2>
      <ul class=\"compact\">{app_items or '<li class="muted">No app mapping has been declared for this library yet.</li>'}</ul>
    </div>
    <p class=\"section\"><a href=\"../../index.html\">Back to solutions index</a> · <a href=\"../index.html\">Back to library catalog</a> · <a href=\"../../../index.html\">Back to docs portal</a></p>
    """
    (target_dir / "index.html").write_text(html_page(f"{entry.name} Library", body), encoding="utf-8")


def write_category_page(site_dir: Path, category: str, entries: list[LibraryEntry]) -> None:
    target_dir = site_dir / "solutions" / "libraries" / category.lower()
    ensure_dir(target_dir)
    cards = "".join(
        f'<div class="card"><h3><a href="../{html.escape(entry.relative_path)}/index.html">{html.escape(entry.name)}</a></h3><p>{html.escape(entry.summary)}</p><p class="muted">{entry.header_count} headers · {entry.source_count} sources · {"tests" if entry.has_tests else "no tests detected"}</p></div>'
        for entry in entries
    )
    body = f"""
    <h1>{html.escape(category)} Libraries</h1>
    <p>{html.escape(CATEGORY_SUMMARIES.get(category, 'CoolBox library area.'))}</p>
    <div class=\"grid\">{cards}</div>
    <p class=\"section\"><a href=\"../index.html\">Back to library catalog</a> · <a href=\"../../index.html\">Back to solutions index</a> · <a href=\"../../../index.html\">Back to docs portal</a></p>
    """
    (target_dir / "index.html").write_text(html_page(f"{category} Libraries", body), encoding="utf-8")


def write_library_index(site_dir: Path, entries: list[LibraryEntry]) -> None:
    target_dir = site_dir / "solutions" / "libraries"
    ensure_dir(target_dir)
    category_counts: dict[str, int] = {}
    for entry in entries:
        category_counts[entry.category] = category_counts.get(entry.category, 0) + 1

    category_cards = "".join(
        f'<div class="card"><h3><a href="{html.escape(category.lower())}/index.html">{html.escape(category)}</a></h3><p>{html.escape(CATEGORY_SUMMARIES.get(category, "CoolBox library area."))}</p><p class="muted">{count} libraries indexed</p></div>'
        for category, count in sorted(category_counts.items())
    )
    featured_cards = "".join(
        f'<div class="card"><h3><a href="{html.escape(entry.relative_path)}/index.html">{html.escape(entry.name)}</a></h3><p>{html.escape(entry.summary)}</p><p class="muted">{html.escape(entry.category)} · {", ".join(entry.targets[:2])}</p></div>'
        for entry in entries[:12]
    )
    body = f"""
    <h1>CoolBox Library Catalog</h1>
    <p class=\"muted\">Narrative inventory of the reusable C++ libraries behind the CoolBox products, bindings, and demos. This catalog complements Doxygen by focusing on purpose, packaging, and relationships.</p>
    <div class=\"section\">
      <h2>Solution Areas</h2>
      <div class=\"grid\">{category_cards}</div>
    </div>
    <div class=\"section\">
      <h2>Featured Libraries</h2>
      <div class=\"grid\">{featured_cards}</div>
    </div>
    <p class=\"section\"><a href=\"../index.html\">Back to solutions index</a> · <a href=\"../../index.html\">Back to docs portal</a></p>
    """
    (target_dir / "index.html").write_text(html_page("CoolBox Library Catalog", body), encoding="utf-8")


def resolve_library_page(entries: list[LibraryEntry], dependency: str) -> str | None:
    for entry in entries:
        if dependency in entry.targets or dependency == entry.relative_path.split("/")[-1]:
            return entry.page_url
    return None


def write_dependency_map(site_dir: Path, library_entries: list[LibraryEntry], consumers: list[ConsumerEntry]) -> None:
    target_dir = site_dir / "solutions" / "dependencies"
    ensure_dir(target_dir)

    consumer_cards: list[str] = []
    for consumer in consumers:
        dependency_links: list[str] = []
        for dependency in consumer.dependencies:
            page_url = resolve_library_page(library_entries, dependency)
            if page_url:
                dependency_links.append(f'<a class="pill" href="../{html.escape(page_url)}">{html.escape(dependency)}</a>')
            else:
                dependency_links.append(f'<span class="pill">{html.escape(dependency)}</span>')
        source_markup = f'<a href="{html.escape(consumer.page_url)}">Open page</a>' if consumer.page_url else f'<code>{html.escape(consumer.source_dir)}</code>'
        dependency_markup = "".join(dependency_links) or '<span class="muted">No dependencies parsed.</span>'
        consumer_cards.append(
            f'<article class="card consumer-card" data-kind="{html.escape(consumer.kind)}">'
            f'<p class="muted">{html.escape(consumer.kind.upper())}</p>'
            f'<h3>{html.escape(consumer.title)}</h3>'
            f'<p>{html.escape(consumer.summary)}</p>'
            f'<p class="muted">Source: {source_markup}</p>'
            f'<div>{dependency_markup}</div>'
            f'</article>'
        )

    reverse_rows: list[str] = []
    for entry in sorted(library_entries, key=lambda item: (item.category, item.name.lower())):
        consumers_list = sorted(set(entry.related_products + entry.related_apps))
        used_by_markup = ", ".join(html.escape(name) for name in consumers_list) or '<span class="muted">None indexed</span>'
        reverse_rows.append(
            f'<tr><td><a href="../{html.escape(entry.page_url)}">{html.escape(entry.name)}</a></td>'
            f'<td><code>{html.escape(entry.category)}</code></td>'
            f'<td>{len(consumers_list)}</td>'
            f'<td>{used_by_markup}</td></tr>'
        )

    body = f"""
    <h1>Library Dependency Map</h1>
    <p>This view shows how reusable libraries flow upward into apps and products. Use it when you need impact analysis before changing a package or when you need to find which executable surfaces depend on a library.</p>
    <div class=\"section\">
      <button class=\"pill\" type=\"button\" onclick=\"filterConsumers('all')\">All</button>
      <button class=\"pill\" type=\"button\" onclick=\"filterConsumers('product')\">Products</button>
      <button class=\"pill\" type=\"button\" onclick=\"filterConsumers('app')\">Apps</button>
    </div>
    <div class=\"section grid\">{''.join(consumer_cards)}</div>
    <div class=\"section\">
      <h2>Reverse Dependencies</h2>
      <table>
        <tr><th>Library</th><th>Category</th><th>Consumers</th><th>Used By</th></tr>
        {''.join(reverse_rows)}
      </table>
    </div>
    <p class=\"section\"><a href=\"../index.html\">Back to solutions index</a> · <a href=\"../../index.html\">Back to docs portal</a></p>
    <script>
      function filterConsumers(kind) {{
        const cards = document.querySelectorAll('.consumer-card');
        cards.forEach((card) => {{
          card.style.display = kind === 'all' || card.dataset.kind === kind ? '' : 'none';
        }});
      }}
    </script>
    """
    (target_dir / "index.html").write_text(html_page("Library Dependency Map", body), encoding="utf-8")


def write_solutions_index(site_dir: Path, entries: list[LibraryEntry], consumers: list[ConsumerEntry]) -> None:
    target_dir = site_dir / "solutions"
    ensure_dir(target_dir)
    category_counts: dict[str, int] = {}
    for entry in entries:
        category_counts[entry.category] = category_counts.get(entry.category, 0) + 1
    app_count = sum(1 for consumer in consumers if consumer.kind == "app")
    product_count = sum(1 for consumer in consumers if consumer.kind == "product")
    area_cards = "".join(
        f'<div class="card"><h3><a href="libraries/{html.escape(category.lower())}/index.html">{html.escape(category)}</a></h3><p>{html.escape(CATEGORY_SUMMARIES.get(category, "CoolBox library area."))}</p><p class="muted">{count} indexed libraries</p></div>'
        for category, count in sorted(category_counts.items())
    )
    product_cards = "".join(
        f'<div class="card"><h3><a href="../products/{html.escape(product.slug)}/index.html">{html.escape(product.title)}</a></h3><p>{html.escape(product.summary)}</p></div>'
        for product in PRODUCTS
    )
    body = f"""
    <h1>CoolBox Solutions</h1>
    <p>This internal generator adds the narrative layer missing from raw API docs: what each library is for, how it is packaged, and which products depend on it.</p>
    <div class=\"section\">
      <h2>Library Solution Areas</h2>
      <div class=\"grid\">{area_cards}</div>
    </div>
    <div class=\"section\">
      <h2>Dependency View</h2>
      <p class=\"muted\">Trace library relationships into {app_count} apps and {product_count} products.</p>
      <p><a class=\"pill\" href=\"dependencies/index.html\">Open Dependency Map</a></p>
    </div>
    <div class=\"section\">
      <h2>Product Layer</h2>
      <div class=\"grid\">{product_cards}</div>
    </div>
    <div class=\"section\">
      <h2>How To Use This Catalog</h2>
      <ul class=\"compact\">
        <li>Start from a solution area when you need reusable libraries by domain.</li>
        <li>Open the dependency map when you need library impact analysis across apps and products.</li>
        <li>Use Doxygen for API-level details after the catalog tells you which package matters.</li>
      </ul>
    </div>
    <p class=\"section\"><a href=\"libraries/index.html\">Open the full library catalog</a> · <a href=\"../products/index.html\">Browse product pages</a> · <a href=\"../index.html\">Back to docs portal</a></p>
    """
    (target_dir / "index.html").write_text(html_page("CoolBox Solutions", body), encoding="utf-8")


def write_catalog_manifest(site_dir: Path, entries: list[LibraryEntry]) -> None:
    manifest_path = site_dir / "solutions" / "catalog.json"
    ensure_dir(manifest_path.parent)
    manifest_path.write_text(json.dumps([asdict(entry) for entry in entries], indent=2), encoding="utf-8")


def main() -> int:
    project_root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd()
    site_dir = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else project_root / ".site"
    entries = build_library_entries(project_root)
    consumers = build_product_entries() + build_app_entries(project_root)

    for entry in entries:
        write_library_page(site_dir, entry)

    grouped: dict[str, list[LibraryEntry]] = {}
    for entry in entries:
        grouped.setdefault(entry.category, []).append(entry)
    for category, category_entries in grouped.items():
        write_category_page(site_dir, category, sorted(category_entries, key=lambda item: item.name.lower()))

    write_library_index(site_dir, sorted(entries, key=lambda item: (item.category, item.name.lower())))
    write_dependency_map(site_dir, entries, consumers)
    write_solutions_index(site_dir, entries, consumers)
    write_catalog_manifest(site_dir, entries)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())