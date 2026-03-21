import argparse
from pathlib import Path
from collections import defaultdict
import html

# Dummy function to simulate tag page generation
def generate_tag_pages(output_dir, tags):
    output_dir.mkdir(parents=True, exist_ok=True)
    for tag, pages in tags.items():
        tag_file = output_dir / f"{tag}.html"
        tag_file.write_text(
            f"<html><head><title>{html.escape(tag)}</title></head><body>\n"
            f"<h1>Tag: {html.escape(tag)}</h1>\n"
            f"<ul>\n"
            + "\n".join(f"<li>{html.escape(page)}</li>" for page in pages)
            + "\n</ul></body></html>",
            encoding="utf-8"
        )

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Build tag pages for tutorials.")
    parser.add_argument("--out", type=str, required=True, help="Output directory for tag pages.")
    args = parser.parse_args()

    # Example tags data (replace with actual data source)
    tags = {
        "python": ["Tutorial 1", "Tutorial 2"],
        "machine-learning": ["Tutorial 3"],
        "data-science": ["Tutorial 4", "Tutorial 5"],
    }

    output_dir = Path(args.out)
    generate_tag_pages(output_dir, tags)
    print(f"Tag pages built at {output_dir}")