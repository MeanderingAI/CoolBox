from build_libraries import (
    Block, TutorialPage, PublicationEntry,
    slugify, split_payload, normalize_library_reference, parse_code_payload, is_remote_url, copy_asset,
    build_tutorial_site, render_page, render_block, parse_tutorial, youtube_embed, load_publications, render_publications, render_index
)
from pathlib import Path

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Build static HTML pages from CoolBox .tut tutorial files.")
    parser.add_argument("source", nargs="?", default="tutorials", help="Directory containing .tut files")
    parser.add_argument("output", nargs="?", default="build/tutorials-site", help="Directory for generated HTML")
    args = parser.parse_args()

    build_tutorial_site(Path(args.source).resolve(), Path(args.output).resolve())
    print(f"Built tutorials site at {Path(args.output).resolve()}")
