#!/usr/bin/env python3

"""
Build publications HTML pages from .bib files.

This script reads .bib files from the bib/ directory (or a custom path)
and generates a publications index page plus per-reference HTML pages
inside the output directory.

Depends on: build_libraries_bib.py (for parse_bib_entries)
Should run AFTER build_tags.py and BEFORE build_tutorials.py so that
the publications output is available when the tutorials site is assembled.
"""

from pathlib import Path
from build_libraries_bib import parse_bib_entries
from build_libraries import (
    load_publications,
    render_publications,
)
from build_references import build_references


def build_publications_site(bib_dir: Path, output_dir: Path) -> None:
    """Build publications and references HTML from bib files."""
    output_dir.mkdir(parents=True, exist_ok=True)

    publications = load_publications(bib_dir)
    print(f"Loaded {len(publications)} publication(s) from {bib_dir}")
    render_publications(publications, output_dir)
    print(f"Publications index written to {output_dir / 'publications' / 'index.html'}")

    # Build per-bib reference pages separately
    references_out = output_dir / "references"
    build_references(bib_dir, references_out)
    print(f"Reference pages written to {references_out}")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Build publications and reference HTML pages from .bib files."
    )
    parser.add_argument(
        "--bib",
        type=str,
        default="bib",
        help="Path to directory containing .bib files (default: bib)",
    )
    parser.add_argument(
        "--out",
        type=str,
        default="build/documentation_site",
        help="Output directory for generated HTML (default: build/documentation_site)",
    )
    args = parser.parse_args()

    build_publications_site(Path(args.bib).resolve(), Path(args.out).resolve())
