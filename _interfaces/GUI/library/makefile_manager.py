"""
makefile_manager.py
Detects the OS and invokes the correct Makefile (Makefile.win or Makefile) for
build and test operations. Used by the GUI backend instead of calling cmake/ctest directly.
"""

import os
import platform
import subprocess
import re
from typing import Optional


def _repo_root() -> str:
    """Return repository root (CoolBox/) from _interfaces/GUI/library/."""
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

def _is_windows() -> bool:
    return platform.system() == "Windows"


def _make_cmd() -> list[str]:
    """Return the base make invocation for the current OS."""
    root = _repo_root()
    if _is_windows():
        return ["make", "-f", os.path.join(root, "Makefile.win")]
    return ["make", "-f", os.path.join(root, "Makefile")]


def run_make(
    target: str,
    timeout: int = 300,
    extra_env: Optional[dict] = None,
) -> dict:
    """
    Run a Makefile target and return { success, output, returncode }.
    target: e.g. "build_libraries", "test-TystFrameworkTests", "test"
    """
    # Sanitize: only allow safe characters in target names
    if not re.match(r'^[\w:\.\-]+$', target):
        return {"success": False, "output": "Invalid target name.", "returncode": -1}

    cmd = _make_cmd() + [target]
    env = {**os.environ, **(extra_env or {})}

    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
            cwd=_repo_root(),
            env=env,
        )
        return {
            "success": result.returncode == 0,
            "output": result.stdout + result.stderr,
            "returncode": result.returncode,
        }
    except subprocess.TimeoutExpired:
        return {"success": False, "output": f"Timed out after {timeout}s.", "returncode": -1}
    except FileNotFoundError:
        return {"success": False, "output": "make not found. Is make/nmake on PATH?", "returncode": -1}
    except Exception as e:
        return {"success": False, "output": str(e), "returncode": -1}


def find_artifacts(target_names: list[str]) -> list[str]:
    """
    Search the build directory for compiled artifacts (.dll, .lib, .exe, .so, .dylib)
    whose stem matches one of the given cmake target names.
    Returns paths relative to the repo root (using forward slashes), suitable for
    passing to the /download endpoint.
    """
    root = _repo_root()
    build_dir = os.path.join(root, "build")
    if not os.path.isdir(build_dir):
        return []

    _ARTIFACT_EXTS = {'.dll', '.lib', '.exe', '.so', '.dylib', '.a'}
    name_set = {n.lower() for n in target_names}
    results: list[str] = []

    for dirpath, dirs, files in os.walk(build_dir):
        # Skip cmake internal dirs
        dirs[:] = [d for d in dirs if d not in {'CMakeFiles', '.cmake'}]
        for fname in files:
            stem, ext = os.path.splitext(fname)
            if ext.lower() in _ARTIFACT_EXTS and stem.lower() in name_set:
                full = os.path.join(dirpath, fname)
                rel = os.path.relpath(full, root).replace('\\', '/')
                results.append(rel)

    return sorted(results)


def build_target(cmake_target: str, timeout: int = 300) -> dict:
    """Build a named CMake target (or package directory) via cmake.
    If cmake_target is an exact cmake target name, builds it via the Makefile build_<target> rule.
    If cmake_target matches a package directory (e.g. 'LSP'), resolves all library targets inside
    it and builds them together with a single cmake --build invocation.
    Returns dict with keys: success, output, returncode, artifacts (list of rel paths)."""
    cmake_deps = _parse_all_cmake_deps(_repo_root())

    exact = cmake_target if cmake_target in cmake_deps else next(
        (k for k in cmake_deps if k.lower() == cmake_target.lower()), None
    )

    if exact:
        # Known cmake target — use the Makefile rule (handles configure + sign)
        result = run_make(f"build_{exact}", timeout=timeout)
        if result["success"]:
            result["artifacts"] = find_artifacts([exact])
        else:
            result["artifacts"] = []
        return result

    # Not a direct cmake target — try package-directory lookup
    pkg_targets = _targets_in_package_dir(_repo_root(), cmake_target, cmake_deps)

    # Also try snake_case conversion (CTest names are PascalCase, cmake targets snake_case).
    if not pkg_targets and not exact:
        def _to_snake(s: str) -> str:
            s = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', s)
            s = re.sub(r'([a-z\d])([A-Z])', r'\1_\2', s)
            return s.lower()
        snake = _to_snake(cmake_target)
        if snake != cmake_target:
            exact = snake if snake in cmake_deps else next(
                (k for k in cmake_deps if k.lower() == snake.lower()), None
            )
            if exact:
                result = run_make(f"build_{exact}", timeout=timeout)
                if result["success"]:
                    result["artifacts"] = find_artifacts([exact])
                else:
                    result["artifacts"] = []
                return result

    # Last resort: target exists as add_library/add_executable but has zero deps
    # (so it never appeared in cmake_deps).  Verify via source scan, then build directly.
    if not pkg_targets:
        if cmake_target in _scan_defined_targets(_repo_root()):
            root = _repo_root()
            build_dir = os.path.join(root, "build")
            cmd = ["cmake", "--build", build_dir, "--config", "Release", "--target", cmake_target]
            try:
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, cwd=root)
                success = result.returncode == 0
                return {
                    "success": success,
                    "output": result.stdout + result.stderr,
                    "returncode": result.returncode,
                    "artifacts": find_artifacts([cmake_target]) if success else [],
                }
            except subprocess.TimeoutExpired:
                return {"success": False, "output": f"Build timed out after {timeout}s.", "returncode": -1, "artifacts": []}
            except Exception as e:
                return {"success": False, "output": str(e), "returncode": -1, "artifacts": []}
        return {"success": False,
                "output": f"No cmake target or package directory named '{cmake_target}' found.",
                "returncode": -1, "artifacts": []}

    # Filter out test executables (keep libraries and app executables)
    _TEST_PAT = re.compile(r'(^test_|_tests?$)', re.IGNORECASE)
    lib_targets = [t for t in pkg_targets if not _TEST_PAT.search(t)]
    build_targets = lib_targets if lib_targets else pkg_targets

    # Build all resolved targets in one cmake invocation
    root = _repo_root()
    build_dir = os.path.join(root, "build")
    target_flags: list[str] = []
    for t in build_targets:
        target_flags += ["--target", t]

    cmd = ["cmake", "--build", build_dir, "--config", "Release"] + target_flags
    try:
        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=timeout, cwd=root,
        )
        label = f"Package '{cmake_target}' → targets: {', '.join(build_targets)}"
        success = result.returncode == 0
        return {
            "success": success,
            "output": label + "\n" + result.stdout + result.stderr,
            "returncode": result.returncode,
            "artifacts": find_artifacts(build_targets) if success else [],
        }
    except subprocess.TimeoutExpired:
        return {"success": False, "output": f"Build timed out after {timeout}s.", "returncode": -1, "artifacts": []}
    except FileNotFoundError:
        return {"success": False, "output": "cmake not found on PATH.", "returncode": -1, "artifacts": []}
    except Exception as e:
        return {"success": False, "output": str(e), "returncode": -1, "artifacts": []}


def _find_test_executable(test_name: str, build_dir: str, config: str = "Release") -> Optional[str]:
    """
    Parse CTestTestfile.cmake entries to find the executable path for a named test
    in the given config, then check which config's exe actually exists on disk.
    Returns the absolute exe path, or None.
    """
    # Collect all CTestTestfile.cmake files
    testfiles: list[str] = []
    for dirpath, dirs, files in os.walk(build_dir):
        dirs[:] = [d for d in dirs if d not in {'CMakeFiles'}]
        for f in files:
            if f == 'CTestTestfile.cmake':
                testfiles.append(os.path.join(dirpath, f))

    pat = re.compile(
        r'add_test\s*\(\s*\[=\[' + re.escape(test_name) + r']=\]\s+"([^"]+)"',
        re.IGNORECASE,
    )
    candidates: list[str] = []
    for tf in testfiles:
        try:
            with open(tf, encoding='utf-8', errors='replace') as fh:
                content = fh.read()
            for m in pat.finditer(content):
                candidates.append(m.group(1))
        except Exception:
            pass

    # Prefer the requested config, then fall back to any that exists
    preferred = [c for c in candidates if f'/{config}/' in c.replace('\\', '/')]
    for path in (preferred + candidates):
        if os.path.isfile(path):
            return path
    return None


def _cmake_build_target(cmake_target: str, build_dir: str, repo_root: str,
                         config: str = "Release", timeout: int = 300) -> dict:
    """Run cmake --build for a single target. Returns {success, output, returncode}."""
    cmd = ["cmake", "--build", build_dir, "--config", config, "--target", cmake_target]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, cwd=repo_root)
        return {"success": r.returncode == 0, "output": r.stdout + r.stderr, "returncode": r.returncode}
    except subprocess.TimeoutExpired:
        return {"success": False, "output": f"Build timed out after {timeout}s.", "returncode": -1}
    except Exception as e:
        return {"success": False, "output": str(e), "returncode": -1}


def ctest_run(test_name: str, timeout: int = 180) -> dict:
    """Run a CTest test by name.
    If the test executable is missing, automatically builds it first.
    Returns {success, output, returncode}."""
    if not re.match(r'^[\w:\.\- ]+$', test_name):
        return {"success": False, "output": "Invalid test name.", "returncode": -1}

    repo_root = _repo_root()
    build_dir = os.path.join(repo_root, "build")
    config = "Release"
    build_log = ""

    # Auto-build if executable is missing
    exe = _find_test_executable(test_name, build_dir, config)
    if exe is None:
        # Derive cmake target: CTest names are typically PascalCase but cmake targets are snake_case.
        def _to_snake(s: str) -> str:
            s = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', s)
            s = re.sub(r'([a-z\d])([A-Z])', r'\1_\2', s)
            return s.lower()

        cmake_target = _to_snake(test_name)
        build_result = _cmake_build_target(cmake_target, build_dir, repo_root, config, timeout=300)
        build_log = f"[auto-build {cmake_target}]\n{build_result['output']}\n\n"
        if not build_result["success"]:
            return {"success": False,
                    "output": build_log + "Build failed — cannot run test.",
                    "returncode": build_result["returncode"]}
        exe = _find_test_executable(test_name, build_dir, config)
        # Some projects default to Debug; if Release exe still missing, try Debug config
        if exe is None:
            config = "Debug"
            exe = _find_test_executable(test_name, build_dir, config)

    try:
        result = subprocess.run(
            ["ctest", "--test-dir", build_dir,
             "-R", f"^{re.escape(test_name)}$",
             "--output-on-failure", "-C", config],
            capture_output=True, text=True, timeout=timeout, cwd=repo_root,
        )
        return {
            "success": result.returncode == 0,
            "output": build_log + result.stdout + result.stderr,
            "returncode": result.returncode,
        }
    except subprocess.TimeoutExpired:
        return {"success": False, "output": build_log + f"Test timed out after {timeout}s.", "returncode": -1}
    except FileNotFoundError:
        return {"success": False, "output": "ctest not found on PATH.", "returncode": -1}
    except Exception as e:
        return {"success": False, "output": str(e), "returncode": -1}


def run_test(test_name: str, timeout: int = 180) -> dict:
    """Build and run a named test via the Makefile test-<name> rule (legacy, uses CTest name as cmake target)."""
    return run_make(f"test-{test_name}", timeout=timeout)


def run_all_tests(timeout: int = 600) -> dict:
    """Run all CTest tests via the 'test' Makefile rule."""
    return run_make("test", timeout=timeout)


def get_deps(target: str, max_depth: int = 6) -> dict:
    """
    Return the dependency graph for a CMake target by parsing source CMakeLists.txt files.
    No cmake invocation — reads target_link_libraries() calls directly from source.
    Returns {success, edges:[{from, to, kind}], dot:None, output, available_targets}.
    kind is PUBLIC | PRIVATE | INTERFACE (the cmake link visibility).
    """
    if not re.match(r'^[\w\-\.]+$', target):
        return {"success": False, "edges": [], "dot": None,
                "output": "Invalid target name.", "available_targets": []}

    cmake_deps = _parse_all_cmake_deps(_repo_root())
    available = sorted(cmake_deps)

    # 1. Exact match, then case-insensitive match
    resolved_single = target if target in cmake_deps else next(
        (k for k in cmake_deps if k.lower() == target.lower()), None
    )

    # 2. If still not found, the name is probably a package/group directory
    #    (e.g. "LSP" → _libraries/groups/COMMS/LSP/ → targets: plang_lsp_lib, plang_lsp_test)
    #    Collect all cmake targets defined inside that package dir and BFS from all of them.
    if resolved_single is None:
        roots = _targets_in_package_dir(_repo_root(), target, cmake_deps)
        if not roots:
            return {"success": False, "edges": [], "dot": None,
                    "output": f"No target_link_libraries found for '{target}'. "
                               f"Is '{target}' a library target? (Test-only targets and "
                               f"pure header libraries won't appear here.)",
                    "available_targets": available}
        note = f"Package '{target}' → targets: {', '.join(roots)}"
    else:
        roots = [resolved_single]
        note = ""

    # BFS up to max_depth from all root targets
    edges: list[dict] = []
    seen: set[tuple[str, str]] = set()
    visited: set[str] = set()
    queue: list[tuple[str, int]] = [(r, 0) for r in roots]

    while queue:
        node, depth = queue.pop(0)
        if node in visited or depth >= max_depth:
            continue
        visited.add(node)
        for dep, visibility in cmake_deps.get(node, []):
            key = (node, dep)
            if key not in seen:
                seen.add(key)
                edges.append({"from": node, "to": dep, "kind": visibility})
            dep_resolved = dep if dep in cmake_deps else next(
                (k for k in cmake_deps if k.lower() == dep.lower()), dep
            )
            if dep_resolved not in visited:
                queue.append((dep_resolved, depth + 1))

    return {"success": True, "edges": edges, "dot": None,
            "output": note, "available_targets": available}


# ── CMakeLists.txt dependency parser ─────────────────────────────────────────

def _targets_in_package_dir(
    repo_root: str, package_name: str, cmake_deps: dict
) -> list[str]:
    """
    When `package_name` is a directory name (e.g. 'LSP') rather than a cmake
    target name, locate the matching source directory and parse all
    target_link_libraries() calls within it.  Returns the target names found.
    """
    pkg_dirs: list[str] = []
    for sub in _SEARCH_DIRS:
        d = os.path.join(repo_root, sub)
        if not os.path.isdir(d):
            continue
        for dirpath, dirs, _ in os.walk(d):
            dirs[:] = [x for x in dirs if x not in {'build', '__pycache__', '.git'}]
            if os.path.basename(dirpath).upper() == package_name.upper():
                pkg_dirs.append(dirpath)

    if not pkg_dirs:
        return []

    local_deps: dict[str, list] = {}
    for pkg_dir in pkg_dirs:
        for dirpath, dirs, files in os.walk(pkg_dir):
            dirs[:] = [x for x in dirs if x not in {'build', '__pycache__', '.git'}]
            for f in files:
                if f == 'CMakeLists.txt':
                    try:
                        with open(
                            os.path.join(dirpath, f), encoding='utf-8', errors='replace'
                        ) as fh:
                            _extract_link_libs(fh.read(), local_deps)
                    except Exception:
                        pass

    # Merge newly discovered targets into the global cmake_deps so BFS works
    for tgt, deps in local_deps.items():
        if tgt not in cmake_deps:
            cmake_deps[tgt] = deps

    return sorted(local_deps.keys())


# System / OS libraries that are never CoolBox targets
_SYSTEM_LIBS = re.compile(
    r'^(kernel32|user32|gdi32|winspool|shell32|ole32|oleaut32|uuid|comdlg32|advapi32'
    r'|pthread|dl|m|rt|c|stdc\+\+|gcc_s|ws2_32|iphlpapi|Dbghelp'
    r'|Cocoa|Foundation|IOKit|CoreFoundation|Security'
    r'|X11|GL|GLU|GLUT|OpenGL|Xrandr|Xi'
    r'|z|bz2|lzma|ssl|crypto|curl)$',
    re.IGNORECASE
)
_LINK_KEYWORDS = frozenset({'PUBLIC', 'PRIVATE', 'INTERFACE', 'debug', 'optimized', 'general'})
_SEARCH_DIRS = ('_libraries', 'apps', '_Product')


def _scan_defined_targets(repo_root: str) -> set[str]:
    """
    Return the set of ALL cmake target names defined in source CMakeLists.txt files
    (add_library, add_executable, add_custom_target) — including those with zero deps
    that never appear in the cmake_deps map.
    """
    pat = re.compile(
        r'\b(?:add_library|add_executable|add_custom_target)\s*\(\s*([\w\-\.]+)',
        re.IGNORECASE,
    )
    names: set[str] = set()
    for sub in _SEARCH_DIRS:
        d = os.path.join(repo_root, sub)
        if not os.path.isdir(d):
            continue
        for dirpath, dirs, files in os.walk(d):
            dirs[:] = [x for x in dirs if x not in {'build', '__pycache__', '.git'}]
            for f in files:
                if f == 'CMakeLists.txt':
                    try:
                        with open(os.path.join(dirpath, f), encoding='utf-8', errors='replace') as fh:
                            content = fh.read()
                        # Strip cmake line comments before matching
                        content = re.sub(r'#[^\n]*', '', content)
                        names.update(m.group(1) for m in pat.finditer(content))
                    except Exception:
                        pass
    return names


def _parse_all_cmake_deps(repo_root: str) -> dict[str, list[tuple[str, str]]]:
    """
    Walk source CMakeLists.txt files and extract target_link_libraries() calls.
    Returns {target_name: [(dep_name, visibility), ...]}
    Only includes CoolBox library targets; excludes system libs and cmake vars.
    """
    result: dict[str, list[tuple[str, str]]] = {}
    for sub in _SEARCH_DIRS:
        d = os.path.join(repo_root, sub)
        if not os.path.isdir(d):
            continue
        for dirpath, dirs, files in os.walk(d):
            dirs[:] = [x for x in dirs if x not in {'build', '__pycache__', '.git'}]
            for f in files:
                if f == 'CMakeLists.txt':
                    try:
                        with open(os.path.join(dirpath, f), encoding='utf-8', errors='replace') as fh:
                            _extract_link_libs(fh.read(), result)
                    except Exception:
                        pass
    return result


def _extract_link_libs(content: str, result: dict) -> None:
    """
    Parse all target_link_libraries() calls in a CMakeLists.txt string.
    Handles single-line and multi-line forms by tracking parenthesis depth.
    Populates result as {target: [(dep, visibility), ...]}.
    """
    i = 0
    while True:
        m = re.search(r'target_link_libraries\s*\(', content[i:], re.IGNORECASE)
        if not m:
            break
        # Walk forward tracking paren depth to find matching ')'
        start = i + m.end()
        depth = 1
        j = start
        while j < len(content) and depth > 0:
            ch = content[j]
            if ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
            j += 1
        body = content[start:j - 1]
        i = i + m.start() + 1

        # Tokenize (strip cmake comments # ...)
        tokens = []
        for line in body.splitlines():
            line = line.split('#')[0].strip()
            tokens.extend(line.split())

        if not tokens:
            continue
        tgt = tokens[0]
        visibility = 'PUBLIC'
        for tok in tokens[1:]:
            if tok in ('PUBLIC', 'PRIVATE', 'INTERFACE'):
                visibility = tok
                continue
            if tok in _LINK_KEYWORDS:
                continue
            # Skip cmake generator expressions and variable references
            if tok.startswith('$') or tok.startswith('"'):
                continue
            # Skip system libs
            if _SYSTEM_LIBS.match(tok):
                continue
            entry = (tok, visibility)
            deps = result.setdefault(tgt, [])
            if entry not in deps:
                deps.append(entry)


def scan_tests() -> list:
    """
    Run ctest -N --verbose to discover all tests and their working directories.
    Returns list of { num, name, working_dir, build_target }.
    """
    repo_root = _repo_root()
    build_dir = os.path.join(repo_root, "build")
    tests = []

    _SKIP_VCXPROJ = {"ALL_BUILD.vcxproj", "INSTALL.vcxproj", "RUN_TESTS.vcxproj", "ZERO_CHECK.vcxproj"}

    def _to_snake(s: str) -> str:
        """Convert PascalCase / camelCase CTest name to snake_case cmake target name."""
        s = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1_\2', s)
        s = re.sub(r'([a-z\d])([A-Z])', r'\1_\2', s)
        return s.lower()

    def _find_build_target(working_dir: str, test_name: str = "") -> Optional[str]:
        """Return the cmake target name (vcxproj stem) for a test's working directory.
        When the directory contains multiple vcxproj files (library + test), prefers the one
        that matches the test name.  Falls back to any test-named target, then first found."""
        if not working_dir:
            return None
        # Resolve ../ segments so os.path.isdir works correctly on paths from ctest output
        try:
            real_dir = os.path.realpath(working_dir)
        except Exception:
            real_dir = working_dir
        if not os.path.isdir(real_dir):
            return None

        vcxprojs = [
            f[:-len(".vcxproj")]
            for f in os.listdir(real_dir)
            if f.endswith(".vcxproj") and f not in _SKIP_VCXPROJ
        ]
        if not vcxprojs:
            return None
        if len(vcxprojs) == 1:
            return vcxprojs[0]

        # Multiple candidates — prefer the one that matches the test name.
        snake = _to_snake(test_name) if test_name else ""

        # 1. Exact match on snake_case name
        if snake and snake in vcxprojs:
            return snake

        # 2. Among targets whose name contains "test", prefer exact match or only candidate
        test_targets = [t for t in vcxprojs if re.search(r'test', t, re.IGNORECASE)]
        if len(test_targets) == 1:
            return test_targets[0]

        # 3. Among test targets, prefer the one closest to the snake name
        if snake and test_targets:
            # Try progressively shorter prefixes stripped of _tests/_test suffix
            base = re.sub(r'_tests?$', '', snake)
            for t in test_targets:
                if t.startswith(base) or base in t:
                    return t
            return test_targets[0]

        # 4. Fall back to first vcxproj (original behaviour)
        return vcxprojs[0]

    try:
        result = subprocess.run(
            ["ctest", "--test-dir", build_dir, "-N", "--verbose"],
            capture_output=True, text=True, timeout=15, cwd=repo_root,
        )
        current_num = None
        current_name = None
        # The ctest -N --verbose output places "Working Directory:" BEFORE "Test #N:" on the
        # very next line.  So when we see "Test #N:", current_workdir already holds test N's
        # dir — not the previous test's dir.  We therefore track the dir belonging to the
        # PREVIOUS test separately so the flush uses the right path.
        current_workdir = None
        prev_workdir: Optional[str] = None

        for line in result.stdout.splitlines():
            m = re.match(r'\s+Test\s+#(\d+):\s+(.+)', line)
            if m:
                # current_workdir is THIS test's working dir (just read on the preceding line).
                # Flush the PREVIOUS test using prev_workdir.
                if current_name is not None:
                    build_tgt = _find_build_target(prev_workdir, current_name) if _is_windows() else None
                    tests.append({
                        "num": current_num,
                        "name": current_name,
                        "working_dir": prev_workdir,
                        "build_target": build_tgt,
                    })
                # Promote the current working dir to previous for the next flush.
                prev_workdir = current_workdir
                current_num = int(m.group(1))
                current_name = m.group(2).strip()
                current_workdir = None
                continue
            wd = re.match(r'\d+: Working Directory:\s+(.+)', line)
            if wd:
                current_workdir = wd.group(1).strip()

        if current_name is not None:
            build_tgt = _find_build_target(prev_workdir, current_name) if _is_windows() else None
            tests.append({
                "num": current_num,
                "name": current_name,
                "working_dir": prev_workdir,
                "build_target": build_tgt,
            })
    except Exception:
        pass

    return tests
