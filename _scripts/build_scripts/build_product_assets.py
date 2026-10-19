#!/usr/bin/env python3

from __future__ import annotations

import html
import json
import os
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


@dataclass(frozen=True)
class ProductDefinition:
    slug: str
    title: str
    target: str
    source_dir: str
    readme: str | None
    summary: str
    executable_names: tuple[str, ...]
    related_libraries: tuple[str, ...]
    build_hint: str
    run_hint: str
    architecture_note: str


PRODUCTS: tuple[ProductDefinition, ...] = (
    ProductDefinition(
        slug="mstudio",
        title="MStudio",
        target="MStudio",
        source_dir="_Product/MStudio",
        readme="_Product/MStudio/README.md",
        summary="Native editor product hosted by the GRAPHICS full_application_window shell and backed by reusable file-browser and component models under the MStudio product name.",
        executable_names=("MStudio.exe", "MStudio"),
        related_libraries=("components", "full_application_window", "file_browser_lib"),
        build_hint="cmake --build <build-dir> --target MStudio",
        run_hint="Launch the generated MStudio executable from the selected build tree.",
        architecture_note="Combines the reusable workspace shell, graphics components, and file-browser models into a desktop editing workflow.",
    ),
    ProductDefinition(
        slug="file_browser",
        title="File Browser",
        target="file_browser",
        source_dir="_Product/file_browser",
        readme="_Product/file_browser/README.md",
        summary="Standalone file-system browser product built on the same workspace dock host used by the editor product.",
        executable_names=("file_browser.exe", "file_browser"),
        related_libraries=("full_application_window", "file_browser_lib", "components"),
        build_hint="cmake --build <build-dir> --target file_browser",
        run_hint="Launch the generated file_browser executable from the selected build tree.",
        architecture_note="Reuses the same application shell as the editor, but narrows the workflow around navigation, browsing, and inspection.",
    ),
    ProductDefinition(
        slug="bower_shell",
        title="Bower Shell",
        target="bower_shell",
        source_dir="_Product/bower_shell",
        readme="_Product/bower_shell/README.md",
        summary="Cross-platform embeddable shell product that simulates jobs, prompts, and command execution without relying on an operating-system-specific shell runtime.",
        executable_names=("bower_shell.exe", "bower_shell"),
        related_libraries=("bower_shell_lib",),
        build_hint="cmake --build <build-dir> --target bower_shell",
        run_hint="Launch the generated bower_shell executable from the selected build tree.",
        architecture_note="Packages the reusable shell runtime as a standalone product for command-loop experimentation and embedding.",
    ),
)


def html_page(title: str, body: str) -> str:
    return f"""<!DOCTYPE html>
<html lang=\"en\">
<head>
  <meta charset=\"UTF-8\">
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">
  <title>{html.escape(title)}</title>
  <style>
    body {{ font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; color: #0f172a; background: #f8fafc; }}
    main {{ background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }}
    h1, h2 {{ color: #111827; }}
    p, li {{ color: #475569; }}
    a {{ color: #2563eb; text-decoration: none; font-weight: 600; }}
    a:hover {{ text-decoration: underline; }}
    code, pre {{ font-family: Consolas, monospace; background: #e2e8f0; border-radius: 8px; }}
    code {{ padding: 0.15rem 0.35rem; }}
    pre {{ padding: 1rem; overflow-x: auto; white-space: pre-wrap; }}
    .muted {{ color: #64748b; }}
  </style>
</head>
<body>
  <main>
{body}
  </main>
</body>
</html>
"""


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def read_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8")


def load_solution_catalog(site_dir: Path) -> dict[str, dict[str, object]]:
    catalog_path = site_dir / "solutions" / "catalog.json"
    if not catalog_path.exists():
        return {}
    try:
        payload = json.loads(catalog_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return {}

    index: dict[str, dict[str, object]] = {}
    for entry in payload:
        if not isinstance(entry, dict):
            continue
        keys = {
            str(entry.get("slug", "")),
            str(entry.get("name", "")),
            str(entry.get("relative_path", "")).split("/")[-1],
        }
        keys.update(str(target) for target in entry.get("targets", []))
        for key in keys:
            normalized = key.strip().lower()
            if normalized:
                index.setdefault(normalized, entry)
    return index


def markdownish_to_html(markdown_text: str) -> str:
    lines = markdown_text.splitlines()
    html_lines: list[str] = []
    in_list = False
    for raw_line in lines:
        line = raw_line.strip()
        if not line:
            if in_list:
                html_lines.append("</ul>")
                in_list = False
            continue
        if line.startswith("# "):
            if in_list:
                html_lines.append("</ul>")
                in_list = False
            html_lines.append(f"<h2>{html.escape(line[2:])}</h2>")
            continue
        if line.startswith("## "):
            if in_list:
                html_lines.append("</ul>")
                in_list = False
            html_lines.append(f"<h3>{html.escape(line[3:])}</h3>")
            continue
        if line.startswith("- "):
            if not in_list:
                html_lines.append("<ul>")
                in_list = True
            html_lines.append(f"<li>{html.escape(line[2:])}</li>")
            continue
        html_lines.append(f"<p>{html.escape(line)}</p>")
    if in_list:
        html_lines.append("</ul>")
    return "\n".join(html_lines)


def find_latest_executable(project_root: Path, executable_names: Iterable[str]) -> Path | None:
    candidates: list[Path] = []
    for build_dir in project_root.iterdir():
        if not build_dir.is_dir() or not build_dir.name.startswith("build"):
            continue
        for executable_name in executable_names:
            candidates.extend(build_dir.glob(f"**/{executable_name}"))
    existing = [candidate for candidate in candidates if candidate.is_file()]
    if not existing:
        return None
    return max(existing, key=lambda path: path.stat().st_mtime)


def copy_runtime_files(executable: Path, destination: Path) -> list[str]:
    ensure_dir(destination)
    copied: list[str] = []
    for candidate in executable.parent.iterdir():
        if not candidate.is_file():
            continue
        if candidate.suffix.lower() not in {".exe", ".dll", ".pdb", ".so", ".dylib"} and candidate.name != executable.name:
            continue
        shutil.copy2(candidate, destination / candidate.name)
        copied.append(candidate.name)
    archive_base = destination / "runtime_bundle"
    shutil.make_archive(str(archive_base), "zip", root_dir=destination)
    copied.append(f"{archive_base.name}.zip")
    return sorted(set(copied))


def write_product_page(project_root: Path, site_dir: Path, product: ProductDefinition, asset_listing: list[str]) -> None:
    product_dir = site_dir / "products" / product.slug
    ensure_dir(product_dir)
    solution_catalog = load_solution_catalog(site_dir)

    readme_html = ""
    if product.readme:
        readme_text = read_text(project_root / product.readme)
        if readme_text:
            readme_html = f"<h2>Product Notes</h2>\n{markdownish_to_html(readme_text)}"

    asset_section = '<p class="muted">No packaged runtime assets were found during this docs build.</p>'
    if asset_listing:
        items = "\n".join(
            f'<li><a href="../../artifacts/products/{product.slug}/{html.escape(name)}">{html.escape(name)}</a></li>'
            for name in asset_listing
        )
        asset_section = f"<ul>\n{items}\n</ul>"

    related_library_items: list[str] = []
    for library in product.related_libraries:
        entry = solution_catalog.get(library.lower())
        if entry:
            link = f'../../solutions/{html.escape(str(entry.get("page_url", "")))}'
            summary = html.escape(str(entry.get("summary", "")))
            related_library_items.append(
                f'<li><a href="{link}"><code>{html.escape(library)}</code></a><p>{summary}</p></li>'
            )
        else:
            related_library_items.append(f"<li><code>{html.escape(library)}</code></li>")
    libraries = "\n".join(related_library_items)
    body = f"""
    <h1>{html.escape(product.title)}</h1>
    <p>{html.escape(product.summary)}</p>
    <p class=\"muted\">Build target: <code>{html.escape(product.target)}</code> · Source: <code>{html.escape(product.source_dir)}</code></p>
    <h2>Build and Run</h2>
    <p><strong>Build:</strong> <code>{html.escape(product.build_hint)}</code></p>
    <p><strong>Run:</strong> {html.escape(product.run_hint)}</p>
    <h2>Architecture</h2>
    <p>{html.escape(product.architecture_note)}</p>
    <h2>Related Libraries</h2>
    <ul>
{libraries}
    </ul>
    <h2>Available Runtime Assets</h2>
    {asset_section}
    {readme_html}
    <p><a href="../../solutions/index.html">Browse solution catalog</a> · <a href="../index.html">Back to products index</a> · <a href="../../index.html">Back to docs portal</a></p>
    """
    (product_dir / "index.html").write_text(html_page(product.title, body), encoding="utf-8")


def write_products_index(site_dir: Path, products: list[ProductDefinition]) -> None:
    products_dir = site_dir / "products"
    ensure_dir(products_dir)
    items = "\n".join(
        f'<li><a href="{html.escape(product.slug)}/index.html">{html.escape(product.title)}</a><p>{html.escape(product.summary)}</p></li>'
        for product in products
    )
    body = f"""
    <h1>CoolBox Products</h1>
        <p class=\"muted\">Standalone desktop and console applications built from reusable CoolBox libraries. Each product page now includes build targets, runtime assets, and links back into the narrative library catalog.</p>
    <ul>
      {items}
    </ul>
        <p><a href="../solutions/index.html">Open solution catalog</a> · <a href="../index.html">Back to docs portal</a></p>
    """
    (products_dir / "index.html").write_text(html_page("CoolBox Products", body), encoding="utf-8")


def write_artifacts_index(site_dir: Path, manifest: dict[str, list[str]]) -> None:
    artifacts_dir = site_dir / "artifacts" / "products"
    ensure_dir(artifacts_dir)
    manifest_path = artifacts_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    item_lines: list[str] = []
    for slug, files in manifest.items():
        if not files:
            item_lines.append(f'<li><strong>{html.escape(slug)}</strong><p class="muted">No runtime assets were discovered for this product.</p></li>')
            continue
        links = " ".join(
            f'<li><a href="{html.escape(slug)}/{html.escape(name)}">{html.escape(name)}</a></li>'
            for name in files
        )
        item_lines.append(f'<li><strong>{html.escape(slug)}</strong><ul>{links}</ul></li>')

    body = f"""
    <h1>CoolBox Product Artifacts</h1>
    <p class=\"muted\">Packaged runtime bundles and copied executables discovered during the docs build pipeline.</p>
    <ul>
      {' '.join(item_lines)}
    </ul>
    <p><a href="../../index.html">Back to docs portal</a></p>
    """
    (artifacts_dir / "index.html").write_text(html_page("CoolBox Product Artifacts", body), encoding="utf-8")


def main() -> int:
    project_root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path.cwd()
    site_dir = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else project_root / ".site"
    ensure_dir(site_dir)

    manifest: dict[str, list[str]] = {}
    for product in PRODUCTS:
        asset_listing: list[str] = []
        executable = find_latest_executable(project_root, product.executable_names)
        if executable is not None:
            destination = site_dir / "artifacts" / "products" / product.slug
            asset_listing = copy_runtime_files(executable, destination)
        manifest[product.slug] = asset_listing
        write_product_page(project_root, site_dir, product, asset_listing)

    write_products_index(site_dir, list(PRODUCTS))
    write_artifacts_index(site_dir, manifest)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())