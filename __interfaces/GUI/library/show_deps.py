"""
show_deps.py  —  CLI wrapper around makefile_manager.get_deps()
Usage: python show_deps.py <target_name>
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
import interfaces.GUI.library.makefile_manager as mm


def main():
    if len(sys.argv) < 2:
        print("Usage: show_deps.py <cmake_target_name>")
        sys.exit(1)

    target = sys.argv[1]
    result = mm.get_deps(target)

    if not result["success"]:
        print(f"\n  No target_link_libraries found for '{target}'.")
        print(f"  ({result['output']})")
        if result["available_targets"]:
            avail = result["available_targets"]
            print(f"\n  Targets that have link deps ({len(avail)} total):")
            for t in avail[:40]:
                print(f"    {t}")
            if len(avail) > 40:
                print(f"    ... and {len(avail) - 40} more")
        print()
        return  # not an error — target just has no declared deps

    edges = result["edges"]
    note = result.get("output", "")  # non-empty when package-dir fallback was used

    # When the target is a package directory (not a cmake target directly),
    # `note` says "Package 'X' → targets: a, b, c".  In that case group by source.
    if note and "→ targets:" in note:
        print(f"\n  {note}")
        # Group edges by their 'from' target
        from collections import defaultdict
        by_src: dict = defaultdict(list)
        for e in edges:
            by_src[e["from"]].append((e["to"], e["kind"]))
        for src, deps in sorted(by_src.items()):
            print(f"\n  {src}:")
            for dep, vis in deps:
                print(f"    [{vis}]  {dep}")
        print()
        return

    # Direct deps (from == target)
    direct = [(e["to"], e["kind"]) for e in edges if e["from"] == target]
    # Transitive deps (from != target)
    transitive = {e["to"] for e in edges if e["from"] != target}
    transitive -= {d for d, _ in direct}

    print(f"\n  Dependencies of '{target}':")
    if direct:
        for dep, vis in direct:
            print(f"    [{vis}]  {dep}")
    else:
        print("    (no external CoolBox dependencies)")

    if transitive:
        print(f"\n  Transitive ({len(transitive)}):")
        for dep in sorted(transitive):
            print(f"    {dep}")

    print()


if __name__ == "__main__":
    main()
