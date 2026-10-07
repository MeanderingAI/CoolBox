from fastapi import APIRouter, Request
from fastapi.responses import JSONResponse, StreamingResponse
from starlette.responses import FileResponse as StarletteFileResponse
import os
import re
import sys
import asyncio
import subprocess
from typing import AsyncIterator, Optional, cast
try:
    from .. import REPO_ROOT
except ImportError:
    from __init__ import REPO_ROOT

p1 = APIRouter()

@p1.get("/extensions")
def list_extensions():
    """List all binding packages from _deliverables/libraries/bindings/."""
    repo_root = REPO_ROOT
    bindings_dir = os.path.join(repo_root, "_deliverables", "libraries", "bindings")
    if not os.path.isdir(bindings_dir):
        return JSONResponse({"bindings": []})

    # Map binding folder name → language label
    _LANG_MAP = {
        "c_bindings":          "C",
        "c3_bindings":         "C3",
        "d_bindings":          "D",
        "emscripten_bindings": "Emscripten",
        "go_bindings":         "Go",
        "java_bindings":       "Java",
        "scala_bindings":      "Scala",
        "postgres_bindings":   "Postgres",
        "python_bindings":     "Python",
        "r_bindings":          "R",
        "rust_bindings":       "Rust",
        "swift_bindings":      "Swift",
        "vlang_bindings":      "V",
        "zig_bindings":        "Zig",
    }

    bindings: list[dict[str, object]] = []
    for entry in sorted(os.listdir(bindings_dir)):
        entry_path = os.path.join(bindings_dir, entry)
        if not os.path.isdir(entry_path) or entry.startswith(('.', '_')):
            continue
        has_cmake   = os.path.isfile(os.path.join(entry_path, "CMakeLists.txt"))
        has_setup   = (os.path.isfile(os.path.join(entry_path, "setup.py")) or
                       os.path.isfile(os.path.join(entry_path, "pyproject.toml")))
        has_cargo   = os.path.isfile(os.path.join(entry_path, "Cargo.toml"))
        has_go      = os.path.isfile(os.path.join(entry_path, "go.mod"))
        has_pom     = os.path.isfile(os.path.join(entry_path, "pom.xml"))
        has_package = os.path.isfile(os.path.join(entry_path, "package.json"))
        has_swift_pkg = os.path.isfile(os.path.join(entry_path, "Package.swift"))
        has_postgres_sql = bool(list(__import__("glob").glob(os.path.join(entry_path, "**", "*.sql"), recursive=True)))
        has_v_mod   = (os.path.isfile(os.path.join(entry_path, "v.mod")) or
                       bool(list(__import__("glob").glob(os.path.join(entry_path, "*.v")))))
        has_c3      = bool(list(__import__("glob").glob(os.path.join(entry_path, "**", "*.c3"), recursive=True)))
        has_d       = (os.path.isfile(os.path.join(entry_path, "dub.json")) or
                   os.path.isfile(os.path.join(entry_path, "dub.sdl")) or
                   bool(list(__import__("glob").glob(os.path.join(entry_path, "**", "*.d"), recursive=True))))
        has_zig     = (os.path.isfile(os.path.join(entry_path, "build.zig")) or
                   os.path.isfile(os.path.join(entry_path, "build.zig.zon")) or
                   bool(list(__import__("glob").glob(os.path.join(entry_path, "**", "*.zig"), recursive=True))))
        has_r       = os.path.isfile(os.path.join(entry_path, "DESCRIPTION"))

        # Emscripten needs its own cmake+emcc toolchain build
        if entry == "emscripten_bindings":
            import shutil as _shutil
            repo_root_emcc = REPO_ROOT
            _emcc_managed = os.path.join(
                repo_root_emcc, "_local_build_pipeline", "tmp", "installers",
                "emsdk", "upstream", "emscripten",
                "emcc.bat" if os.name == "nt" else "emcc"
            )
            build_type = "emscripten"
            has_emcc   = bool(
                _shutil.which("emcc") or
                os.path.isfile(_emcc_managed) or
                os.path.isfile(r"C:\emsdk\upstream\emscripten\emcc.bat")
            )
        else:
            # Python: prefer pip over cmake even when CMakeLists.txt is present
            build_type = (
                "emscripten" if entry == "emscripten_bindings" else
                "python"     if has_setup   else
                "cmake"      if has_cmake   else
                "cargo"      if has_cargo   else
                "go"         if has_go      else
                "maven"      if has_pom     else
                "npm"        if has_package else
                "swift"      if has_swift_pkg else
                "postgres"   if has_postgres_sql and "postgres" in entry.lower() else
                "vlang"      if has_v_mod   else
                "c3"         if has_c3      else
                "dlang"      if has_d       else
                "zig"        if has_zig     else
                "r"          if has_r       else
                "unknown"
            )
            has_emcc = None

        # Go: check if go is actually installed (including managed install dir)
        has_go_exec = None
        if has_go:
            import shutil as _shutil
            import glob as _glob
            _repo_root = REPO_ROOT
            _go_managed = os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                                       "go_install", "go", "bin",
                                       "go.exe" if os.name == "nt" else "go")
            has_go_exec = bool(
                _shutil.which("go") or
                os.path.isfile(_go_managed) or
                any(os.path.isfile(p) for p in [
                    r"C:\Program Files\Go\bin\go.exe",
                    r"C:\Go\bin\go.exe",
                ])
            )

        # Maven: check if mvn is installed (including managed install dir)
        has_mvn_exec = None
        has_java_exec = None
        if has_pom:
            import shutil as _shutil
            import glob as _glob
            _repo_root = REPO_ROOT
            _mvn_managed = _glob.glob(
                os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                             "maven_install", "apache-maven-*", "bin",
                             "mvn.cmd" if os.name == "nt" else "mvn"),
            )
            has_mvn_exec = bool(
                _shutil.which("mvn") or _shutil.which("mvn.cmd") or
                _mvn_managed or
                any(os.path.isfile(p) for p in [
                    r"C:\Program Files\Maven\bin\mvn.cmd",
                    r"C:\tools\maven\bin\mvn.cmd",
                    r"C:\mvn\bin\mvn.cmd",
                ])
            )
            # Java (JDK) is required by Maven
            _java_exe = "java.exe" if os.name == "nt" else "java"
            _jdk_managed = _glob.glob(
                os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                             "jdk_install", "jdk-*", "bin", _java_exe),
            )
            _jdk_system = []
            if os.name == "nt":
                import glob as _g
                for _pat in [
                    r"C:\Program Files\Java\jdk-*\bin\java.exe",
                    r"C:\Program Files\Eclipse Adoptium\jdk-*\bin\java.exe",
                    r"C:\Program Files\Microsoft\jdk-*\bin\java.exe",
                ]:
                    _jdk_system.extend(_g.glob(_pat))
            has_java_exec = bool(
                _shutil.which("java") or _jdk_managed or _jdk_system or
                os.environ.get("JAVA_HOME")
            )

        # V compiler: check if v is installed (including managed install dir)
        has_v_exec = None
        if has_v_mod:
            import shutil as _shutil
            import glob as _glob
            _repo_root = REPO_ROOT
            _v_exe = "v.exe" if os.name == "nt" else "v"
            _v_managed = _glob.glob(
                os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                             "vlang_install", "**", _v_exe), recursive=True)
            has_v_exec = bool(
                _shutil.which("v") or
                any(os.path.isfile(p) for p in [
                    r"C:\V\v.exe",
                    r"C:\tools\vlang\v.exe",
                ]) or
                _v_managed
            )

        # C3 compiler: check if c3c is installed (including managed install dir)
        has_c3c_exec = None
        if has_c3:
            import shutil as _shutil
            import glob as _glob
            _repo_root = REPO_ROOT
            _c3c_exe = "c3c.exe" if os.name == "nt" else "c3c"
            _c3c_managed = _glob.glob(
                os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                             "c3c_install", "**", _c3c_exe), recursive=True)
            has_c3c_exec = bool(
                _shutil.which("c3c") or
                any(os.path.isfile(p) for p in [
                    r"C:\c3\c3c.exe",
                    r"C:\tools\c3\c3c.exe",
                ]) or
                _c3c_managed
            )

        # D compiler / package manager: check if dmd|ldc2 and dub are installed
        has_d_exec = None
        if has_d:
            import shutil as _shutil
            import glob as _glob
            _repo_root = REPO_ROOT
            _dmd_exe = "dmd.exe" if os.name == "nt" else "dmd"
            _ldc2_exe = "ldc2.exe" if os.name == "nt" else "ldc2"
            _dub_exe = "dub.exe" if os.name == "nt" else "dub"
            _d_managed = _glob.glob(
                os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                             "dlang_install", "**", "*"), recursive=True)
            has_d_exec = bool(
                _shutil.which("dmd") or _shutil.which("ldc2") or _shutil.which("dub") or
                any(os.path.basename(p).lower() in {_dmd_exe, _ldc2_exe, _dub_exe} for p in _d_managed if os.path.isfile(p))
            )

        # Zig compiler: check if zig is installed (including managed install dir)
        has_zig_exec = None
        if has_zig:
            import shutil as _shutil
            import glob as _glob
            _repo_root = REPO_ROOT
            _zig_exe = "zig.exe" if os.name == "nt" else "zig"
            _zig_managed = _glob.glob(
                os.path.join(_repo_root, "_local_build_pipeline", "tmp", "installers",
                             "zig_install", "**", _zig_exe), recursive=True)
            has_zig_exec = bool(
                _shutil.which("zig") or
                any(os.path.isfile(p) for p in _zig_managed) or
                any(os.path.isfile(p) for p in [
                    r"C:\zig\zig.exe",
                    r"C:\tools\zig\zig.exe",
                ])
            )

        # R: check if R/Rscript is installed
        has_r_exec = None
        has_rtools = None
        has_swift_exec = None
        has_psql_exec = None
        if has_r:
            import shutil as _shutil
            has_r_exec = bool(
                _shutil.which("Rscript") or _shutil.which("R") or
                any(os.path.isfile(p) for p in [
                    r"C:\Program Files\R\R-4.6.0\bin\Rscript.exe",
                    r"C:\Program Files\R\R-4.5.0\bin\Rscript.exe",
                    r"C:\Program Files\R\R-4.4.0\bin\Rscript.exe",
                    r"C:\Program Files\R\R-4.3.0\bin\Rscript.exe",
                    r"C:\Program Files\R\R-4.2.0\bin\Rscript.exe",
                ])
            )
            # On Windows, R packages with C++ code also need Rtools (gcc)
            if os.name == "nt":
                has_rtools = any(os.path.isfile(p) for p in [
                    r"C:\rtools45\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                    r"C:\rtools44\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                    r"C:\rtools43\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                    r"C:\rtools42\mingw64\bin\gcc.exe",
                    os.path.expanduser(r"~\rtools45\x86_64-w64-mingw32.static.posix\bin\gcc.exe"),
                ])
            else:
                has_rtools = True  # Linux/macOS use system gcc

        # Swift toolchain check
        if has_swift_pkg:
            import shutil as _shutil
            has_swift_exec = bool(
                _shutil.which("swift") or
                any(os.path.isfile(p) for p in [
                    r"C:\Swift\bin\swift.exe",
                    r"C:\Library\Developer\Toolchains\unknown-Asserts-development.xctoolchain\usr\bin\swift.exe",
                ])
            )

        # Postgres CLI check (optional live SQL validation)
        if has_postgres_sql and "postgres" in entry.lower():
            import shutil as _shutil
            has_psql_exec = bool(
                _shutil.which("psql") or
                any(os.path.isfile(p) for p in [
                    r"C:\Program Files\PostgreSQL\17\bin\psql.exe",
                    r"C:\Program Files\PostgreSQL\16\bin\psql.exe",
                    r"C:\Program Files\PostgreSQL\15\bin\psql.exe",
                    r"C:\Program Files\PostgreSQL\14\bin\psql.exe",
                    r"C:\Program Files\PostgreSQL\13\bin\psql.exe",
                ])
            )
        # Parse CMakeLists.txt to find the real primary cmake target name
        cmake_target = None
        if has_cmake:
            try:
                with open(os.path.join(entry_path, "CMakeLists.txt"), encoding="utf-8", errors="replace") as f:
                    cml = f.read()
                import re as _re
                m = _re.search(r'add_library\s*\(\s*([\w]+)\s+(?!ALIAS)', cml, _re.IGNORECASE)
                if not m:
                    m = _re.search(r'add_executable\s*\(\s*([\w]+)', cml, _re.IGNORECASE)
                if not m:
                    m = _re.search(r'pybind11_add_module\s*\(\s*([\w]+)', cml, _re.IGNORECASE)
                if m:
                    cmake_target = m.group(1)
            except Exception:
                pass
        bindings.append({
            "name":         entry,
            "lang":         _LANG_MAP.get(entry, entry.replace("_bindings", "").title()),
            "build_type":   build_type,
            "cmake_target": cmake_target,
            "has_cmake":    has_cmake,
            "has_cargo":    has_cargo,
            "has_go":       has_go,
            "has_pom":      has_pom,
            "has_setup":    has_setup,
            "has_emcc":     has_emcc,
            "has_go_exec":  has_go_exec,
            "has_mvn_exec": has_mvn_exec,
            "has_java_exec": has_java_exec,
            "has_v_exec":   has_v_exec,
            "has_c3c_exec": has_c3c_exec,
            "has_d_exec":   has_d_exec,
            "has_zig_exec": has_zig_exec,
            "has_r_exec":   has_r_exec,
            "has_rtools":   has_rtools,
            "has_swift_exec": has_swift_exec,
            "has_psql_exec": has_psql_exec,
        })
    return JSONResponse({"bindings": bindings})


@p1.get("/extensions/tools")
def list_tools():
    """Return install-status for every toolchain supported by master_installer.py."""
    import shutil as _shutil
    import glob as _glob
    repo_root = REPO_ROOT
    _tmp = os.path.join(repo_root, "_local_build_pipeline", "tmp", "installers")
    _ext = ".exe" if os.name == "nt" else ""
    _bat = ".bat" if os.name == "nt" else ""

    def _managed(*rel: str) -> str:
        """Return a path inside the managed installers dir."""
        return os.path.join(_tmp, *rel)

    def _glob_managed(*pattern: str) -> bool:
        """Return True if any file matching glob exists inside managed installers dir."""
        return bool(_glob.glob(os.path.join(_tmp, *pattern), recursive=True))

    # Managed Go path is deterministic
    _go_managed = _managed("go_install", "go", "bin", f"go{_ext}")
    # Managed Maven: version varies, glob for mvn.cmd / mvn
    _mvn_pattern = ("maven_install", "apache-maven-*", "bin", f"mvn{_bat or _ext}")
    # Managed jdk: version dir varies, glob for java.exe
    _jdk_pattern = ("jdk_install", "jdk-*", "bin", f"java{_ext}")
    # Managed c3c: extracted dir varies, recurse
    _c3c_pattern = ("c3c_install", "**", f"c3c{_ext}")
    # Managed vlang: extracted dir varies, recurse
    _v_pattern = ("vlang_install", "**", f"v{_ext}")
    # Managed emsdk
    _emsdk_managed = _managed("emsdk")
    _emcc_managed = _managed("emsdk", "upstream", "emscripten", f"emcc{_bat}")

    TOOLS: dict[str, dict[str, object]] = {
        "go":    {"label": "Go compiler",       "check": ["go"],
                  "extra": [_go_managed,
                             r"C:\Program Files\Go\bin\go.exe", r"C:\Go\bin\go.exe"]},
        "maven": {"label": "Apache Maven",      "check": ["mvn", "mvn.cmd"],
                  "extra": [r"C:\Program Files\Maven\bin\mvn.cmd", r"C:\tools\maven\bin\mvn.cmd"],
                  "glob":  _mvn_pattern},
        "jdk":   {"label": "JDK 21 (Temurin)",  "check": ["java"],
                  "extra": [r"C:\Program Files\Java\bin\java.exe",
                             r"C:\Program Files\Eclipse Adoptium\bin\java.exe"],
                  "glob":  _jdk_pattern},
        "c3c":   {"label": "C3 compiler",       "check": ["c3c"],
                  "extra": [r"C:\c3\c3c.exe", r"C:\tools\c3\c3c.exe"],
                  "glob":  _c3c_pattern},
        "vlang": {"label": "V compiler",        "check": ["v"],
                  "extra": [r"C:\V\v.exe", r"C:\tools\vlang\v.exe"],
                  "glob":  _v_pattern},
        "dlang": {"label": "D compiler / dub",  "check": ["dmd", "ldc2", "dub"],
              "extra": [r"C:\D\dmd2\windows\bin64\dmd.exe",
                     r"C:\D\ldc2\bin\ldc2.exe",
                     r"C:\D\dmd2\windows\bin64\dub.exe"],
              "glob":  ("dlang_install", "**", f"dub{_ext}")},
        "zig":   {"label": "Zig compiler",      "check": ["zig"],
              "extra": [r"C:\zig\zig.exe", r"C:\tools\zig\zig.exe"],
              "glob":  ("zig_install", "**", f"zig{_ext}")},
        "r":     {"label": "R / Rscript",       "check": ["Rscript", "R"],
                  "extra": [r"C:\Program Files\R\R-4.6.0\bin\Rscript.exe",
                             r"C:\Program Files\R\R-4.5.0\bin\Rscript.exe",
                             r"C:\Program Files\R\R-4.4.0\bin\Rscript.exe",
                             r"C:\Program Files\R\R-4.3.0\bin\Rscript.exe"]},
        "rtools": {"label": "Rtools (gcc for R)", "check": [],
                   "extra": [
                       r"C:\rtools45\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                       r"C:\rtools44\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                       r"C:\rtools43\x86_64-w64-mingw32.static.posix\bin\gcc.exe",
                       r"C:\rtools42\mingw64\bin\gcc.exe",
                       os.path.expanduser(r"~\rtools45\x86_64-w64-mingw32.static.posix\bin\gcc.exe"),
                   ]},
        "emsdk": {"label": "Emscripten (emcc)", "check": ["emcc"],
                  "extra": [_emcc_managed,
                             r"C:\emsdk\upstream\emscripten\emcc.bat"]},
    }
    result: list[dict[str, object]] = []
    for key, meta in TOOLS.items():
        check = cast(list[str], meta["check"])
        extra = cast(list[str], meta.get("extra", []))
        glob_pattern = cast(Optional[tuple[str, ...]], meta.get("glob"))
        installed = (
            any(_shutil.which(n) for n in check) or
            any(os.path.isfile(p) for p in extra) or
            (glob_pattern is not None and _glob_managed(*glob_pattern))
        )
        result.append({"tool": key, "label": cast(str, meta["label"]), "installed": installed})
    return JSONResponse({"tools": result})


@p1.post("/extensions/install")
async def install_tool(request: Request):
    """Stream master_installer.py output for the requested tool.
    Body: { tool: str }  e.g. "go" | "maven" | "c3c" | "vlang" | "r" | "all"
    Response: text/plain stream, last line is __EXIT_CODE__:<n>
    """
    body = await request.json()
    tool = body.get("tool", "").strip()
    VALID_TOOLS = {"go", "maven", "jdk", "c3c", "vlang", "dlang", "zig", "r", "rtools", "emsdk", "all"}
    if tool not in VALID_TOOLS:
        return JSONResponse({"success": False, "output": f"Unknown tool: {tool}"}, status_code=400)

    repo_root = REPO_ROOT
    script = os.path.join(repo_root, "_scripts", "install_scripts", "master_installer.py")
    if not os.path.isfile(script):
        return JSONResponse({"success": False, "output": "master_installer.py not found."}, status_code=500)

    selected_tool: str = tool if tool != "all" else "--all"
    cmd: list[str] = [sys.executable, "-u", script, selected_tool]

    async def _stream() -> AsyncIterator[str]:
        loop = asyncio.get_event_loop()
        queue: asyncio.Queue[Optional[str]] = asyncio.Queue()

        def _run_proc():
            try:
                proc = subprocess.Popen(
                    cmd, cwd=repo_root,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                    text=True, encoding="utf-8", errors="replace",
                )
                if proc.stdout is not None:
                    for line in proc.stdout:
                        loop.call_soon_threadsafe(queue.put_nowait, line)
                proc.wait()
                loop.call_soon_threadsafe(
                    queue.put_nowait, f"\n__EXIT_CODE__:{proc.returncode}\n"
                )
            except Exception as exc:
                loop.call_soon_threadsafe(
                    queue.put_nowait, f"\nERROR: {exc}\n__EXIT_CODE__:1\n"
                )
            finally:
                loop.call_soon_threadsafe(queue.put_nowait, None)

        loop.run_in_executor(None, _run_proc)

        while True:
            item: Optional[str] = await queue.get()
            if item is None:
                break
            yield item

    return StreamingResponse(_stream(), media_type="text/plain")


@p1.post("/extensions/build")
async def build_extension(request: Request):
    """Stream _scripts/build_scripts/build_extensions.py output for the given binding.
    Body: { binding: str }  -- binding folder name, e.g. "python_bindings"
          { binding: "" }   -- empty string means build all
    Response: text/plain stream, last line is __EXIT_CODE__:<n>
    """
    body = await request.json()
    binding = body.get("binding", "").strip()
    if binding and not re.match(r'^[\w\-]+$', binding):
        return JSONResponse({"success": False, "output": "Invalid binding name."}, status_code=400)

    repo_root = REPO_ROOT
    script = os.path.join(repo_root, "_scripts", "build_scripts", "build_extensions.py")
    if not os.path.isfile(script):
        return JSONResponse({"success": False, "output": "build_extensions.py not found."}, status_code=500)

    cmd: list[str] = [sys.executable, "-u", script]
    if binding:
        cmd.append(binding)

    async def _stream() -> AsyncIterator[str]:
        # asyncio.create_subprocess_exec requires ProactorEventLoop on Windows,
        # which uvicorn doesn't use. Run the blocking Popen in a thread pool and
        # forward lines via an asyncio.Queue with call_soon_threadsafe.
        loop = asyncio.get_event_loop()
        queue: asyncio.Queue[Optional[str]] = asyncio.Queue()

        def _run_proc():
            try:
                proc = subprocess.Popen(
                    cmd, cwd=repo_root,
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                    text=True, encoding='utf-8', errors='replace',
                )
                if proc.stdout is not None:
                    for line in proc.stdout:
                        loop.call_soon_threadsafe(queue.put_nowait, line)
                proc.wait()
                loop.call_soon_threadsafe(
                    queue.put_nowait, f"\n__EXIT_CODE__:{proc.returncode}\n"
                )
            except Exception as exc:
                loop.call_soon_threadsafe(
                    queue.put_nowait, f"\nERROR: {exc}\n__EXIT_CODE__:1\n"
                )
            finally:
                loop.call_soon_threadsafe(queue.put_nowait, None)  # sentinel

        loop.run_in_executor(None, _run_proc)

        while True:
            item: Optional[str] = await queue.get()
            if item is None:
                break
            yield item

    return StreamingResponse(_stream(), media_type="text/plain")


@p1.get("/extensions/artifacts")
def list_extension_artifacts(binding: str):
    """Return downloadable artifacts produced by a binding build.
    Scans the cmake Debug output dir and the binding's own target/ dir.
    Query param: binding=<folder_name>  e.g. c_bindings
    """
    if not re.match(r'^[\w\-]+$', binding):
        return JSONResponse({"error": "Invalid binding name."}, status_code=400)
    repo_root = REPO_ROOT

    ARTIFACT_EXTS = {".dll", ".so", ".dylib", ".wasm", ".js", ".a", ".lib",
                     ".jar", ".pyd", ".exe", ".rlib"}

    artifacts: list[dict[str, object]] = []
    seen: set[str] = set()

    def _scan(directory: str):
        if not os.path.isdir(directory):
            return
        for fname in sorted(os.listdir(directory)):
            _, ext = os.path.splitext(fname)
            if ext.lower() in ARTIFACT_EXTS and fname not in seen:
                fpath = os.path.join(directory, fname)
                if os.path.isfile(fpath):
                    seen.add(fname)
                    artifacts.append({
                        "name": fname,
                        "size": os.path.getsize(fpath),
                        "url":  f"/extensions/download?binding={binding}&file={fname}",
                    })

    # cmake builds land in build/_deliverables/libraries/bindings/<name>/Debug/
    _scan(os.path.join(repo_root, "build", "_deliverables", "libraries", "bindings", binding, "Debug"))
    # cargo builds output into target/debug/
    _scan(os.path.join(repo_root, "_deliverables", "libraries", "bindings", binding, "target", "debug"))
    _scan(os.path.join(repo_root, "_deliverables", "libraries", "bindings", binding, "target"))
    # Python pip installs place .pyd files in the binding root and its immediate subdirs
    binding_root = os.path.join(repo_root, "_deliverables", "libraries", "bindings", binding)
    _scan(binding_root)
    if os.path.isdir(binding_root):
        for entry in sorted(os.listdir(binding_root)):
            sub = os.path.join(binding_root, entry)
            if os.path.isdir(sub):
                _scan(sub)

    return JSONResponse({"binding": binding, "artifacts": artifacts})


@p1.get("/extensions/download")
def download_extension_artifact(binding: str, file: str):
    """Serve a build artifact file for download."""
    if not re.match(r'^[\w\-]+$', binding) or not re.match(r'^[\w\-\.]+$', file):
        return JSONResponse({"error": "Invalid parameters."}, status_code=400)
    repo_root = REPO_ROOT

    ARTIFACT_EXTS = {".dll", ".so", ".dylib", ".wasm", ".js", ".a", ".lib",
                     ".jar", ".pyd", ".exe", ".rlib"}
    _, ext = os.path.splitext(file)
    if ext.lower() not in ARTIFACT_EXTS:
        return JSONResponse({"error": "File type not allowed."}, status_code=403)

    # Build search dirs: same set as list_extension_artifacts
    binding_root = os.path.join(repo_root, "_deliverables", "libraries", "bindings", binding)
    search_dirs = [
        os.path.join(repo_root, "build", "_deliverables", "libraries", "bindings", binding, "Debug"),
        os.path.join(binding_root, "target", "debug"),
        os.path.join(binding_root, "target"),
        binding_root,
    ]
    # Add one level of subdirectories under binding root (e.g. ml_toolbox/ for Python)
    if os.path.isdir(binding_root):
        for entry in sorted(os.listdir(binding_root)):
            sub = os.path.join(binding_root, entry)
            if os.path.isdir(sub):
                search_dirs.append(sub)

    for directory in search_dirs:
        if not os.path.isdir(directory):
            continue
        base_real = os.path.realpath(directory)
        candidate = os.path.realpath(os.path.join(directory, file))
        if candidate.startswith(base_real + os.sep) and os.path.isfile(candidate):
            return StarletteFileResponse(
                candidate,
                filename=file,
                media_type="application/octet-stream",
            )

    return JSONResponse({"error": "Artifact not found."}, status_code=404)


@p1.get("/extensions/docs")
def get_extension_docs(binding: str):
    """Return README.md content for a binding if available."""
    if not re.match(r'^[\w\-]+$', binding):
        return JSONResponse({"error": "Invalid binding name."}, status_code=400)
    repo_root = REPO_ROOT
    binding_dir = os.path.join(repo_root, "_deliverables", "libraries", "bindings", binding)
    if not os.path.isdir(binding_dir):
        return JSONResponse({"content": "", "filename": None})
    for doc_name in ("README.md", "README.txt", "DOCS.md"):
        doc_path = os.path.join(binding_dir, doc_name)
        if os.path.isfile(doc_path):
            with open(doc_path, "r", encoding="utf-8") as f:
                return JSONResponse({"content": f.read(), "filename": doc_name})
    return JSONResponse({"content": "", "filename": None})
