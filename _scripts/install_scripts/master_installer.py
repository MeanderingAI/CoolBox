
#!/usr/bin/env python3
"""
master_installer.py
Install language toolchains required by CoolBox bindings.
Works on Windows, Linux, and macOS.

Supported tools:
  go      — Go compiler (https://go.dev/dl/)
  maven   — Apache Maven (https://maven.apache.org/download.cgi)
  c3c     — C3 compiler (https://c3-lang.org)
  vlang   — V compiler (https://vlang.io)
  r       — R / Rscript (https://cran.r-project.org)

Usage:
  python master_installer.py                 # list available tools
  python master_installer.py go              # install Go
  python master_installer.py go maven vlang  # install multiple
  python master_installer.py --all           # install all
  python master_installer.py --check         # check which are installed

Each installer streams output line-by-line and exits with the sentinel
"""

TOOLS["install_dep"] = {"label": "Install missing dependencies (GSL, Doxygen)", "fn": install_dep, "check": tuple()}

def _download(url: str, dest: Path) -> None:
    print(f"  Downloading {url}", flush=True)
    headers = {"User-Agent": "CoolBox-Installer/1.0"}
    req = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(req) as r, open(dest, "wb") as f:
        total = int(r.headers.get("Content-Length", 0))
        done = 0
        chunk = 65536
        while True:
            buf = r.read(chunk)
            if not buf:
                break
            f.write(buf)
            done += len(buf)
            if total:
                pct = done * 100 // total
                print(f"\r  {pct}% ({done // 1024} KB / {total // 1024} KB)", end="", flush=True)
    print(flush=True)

def _extract(archive: Path, dest: Path) -> None:
    print(f"  Extracting {archive.name} ...", flush=True)
    dest.mkdir(parents=True, exist_ok=True)
    name = archive.name.lower()
    if name.endswith(".zip"):
        with zipfile.ZipFile(archive) as z:
            z.extractall(dest)
    elif name.endswith(".tar.gz") or name.endswith(".tgz"):
        with tarfile.open(archive, "r:gz") as t:
            t.extractall(dest)
    elif name.endswith(".tar.xz"):
        with tarfile.open(archive, "r:xz") as t:
            t.extractall(dest)
    else:
        raise ValueError(f"Unsupported archive format: {archive.name}")

def _run(cmd: list[str], cwd: Path | None = None) -> int:
    print(f"  Running: {' '.join(str(c) for c in cmd)}", flush=True)
    proc = subprocess.Popen(
        cmd, cwd=str(cwd) if cwd else None,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, encoding="utf-8", errors="replace",
    )
    for line in proc.stdout:
        print(line, end="", flush=True)
    proc.wait()
    return proc.returncode

def _run_shell(cmd: str, cwd: Path | None = None) -> int:
    print(f"  Shell: {cmd}", flush=True)
    proc = subprocess.Popen(
        cmd, shell=True, cwd=str(cwd) if cwd else None,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, encoding="utf-8", errors="replace",
    )
    for line in proc.stdout:
        print(line, end="", flush=True)
    proc.wait()
    return proc.returncode

def _banner(tool: str) -> None:
    print(f"\n{'='*60}", flush=True)
    print(f"  Installing: {tool}", flush=True)
    print(f"{'='*60}", flush=True)

# ── Go ────────────────────────────────────────────────────────────────────────

GO_VERSION = "1.22.4"

def _go_url() -> str:
    os_map = {"Windows": "windows", "Darwin": "darwin", "Linux": "linux"}
    ext_map = {"Windows": "zip", "Darwin": "tar.gz", "Linux": "tar.gz"}
    system = _os()
    return (
        f"https://go.dev/dl/go{GO_VERSION}.{os_map[system]}-{_arch()}"
        f".{ext_map[system]}"
    )

def _go_install_dir() -> Path:
    return _tmp_dir() / "go_install"

def install_go() -> bool:
    _banner("Go")
    if _which("go"):
        print(f"  go is already installed: {shutil.which('go')}", flush=True)
        rc = _run(["go", "version"])
        return rc == 0

    tmp = _tmp_dir()
    url = _go_url()
    archive = tmp / url.rsplit("/", 1)[-1]
    try:
        _download(url, archive)
    except Exception as e:
        print(f"  ERROR downloading Go: {e}", flush=True)
        return False

    install_dir = _go_install_dir()
    if install_dir.exists():
        shutil.rmtree(install_dir)

    try:
        _extract(archive, install_dir)
        archive.unlink(missing_ok=True)
    except Exception as e:
        print(f"  ERROR extracting Go: {e}", flush=True)
        return False

    # The archive extracts to a "go" subdirectory
    go_bin = install_dir / "go" / "bin"
    go_exe = go_bin / ("go.exe" if _os() == "Windows" else "go")

    if not go_exe.exists():
        print(f"  ERROR: go executable not found at {go_exe}", flush=True)
        return False

    # Add to PATH for this process (child processes launched later will see it)
    os.environ["PATH"] = str(go_bin) + os.pathsep + os.environ.get("PATH", "")

    print(f"\n  Go installed to: {go_bin}", flush=True)
    print(f"  Add to your PATH permanently: {go_bin}", flush=True)
    _run([str(go_exe), "version"])

    if _os() == "Windows":
        print(f"\n  To make permanent, run in PowerShell:", flush=True)
        print(f'    $env:PATH = "{go_bin}" + ";" + $env:PATH', flush=True)
        print(f'  Or use: [Environment]::SetEnvironmentVariable("PATH", ...)', flush=True)
    else:
        print(f"\n  To make permanent, add to ~/.bashrc or ~/.zshrc:", flush=True)
        print(f'    export PATH="{go_bin}:$PATH"', flush=True)

    return True

# ── Maven ─────────────────────────────────────────────────────────────────────

MAVEN_VERSION_FALLBACK = "3.9.15"

def _maven_latest_version() -> str:
    """Fetch the latest Maven 3.x version from the Apache CDN listing."""
    try:
        import html
        req = urllib.request.Request(
            "https://dlcdn.apache.org/maven/maven-3/",
            headers={"User-Agent": "CoolBox-Installer/1.0"}
        )
        with urllib.request.urlopen(req, timeout=8) as r:
            body = r.read().decode("utf-8", errors="replace")
        versions = re.findall(r'href="(\d+\.\d+\.\d+)/"', body)
        if versions:
            # Sort semantically and take the highest
            versions.sort(key=lambda v: tuple(int(x) for x in v.split(".")))
            return versions[-1]
    except Exception:
        pass
    return MAVEN_VERSION_FALLBACK

def _maven_url() -> str:
    v = _maven_latest_version()
    return (
        f"https://dlcdn.apache.org/maven/maven-3/{v}/binaries/"
        f"apache-maven-{v}-bin.tar.gz"
    )

def _maven_archive_name(url: str) -> str:
    return url.rsplit("/", 1)[-1]

def _maven_install_dir() -> Path:
    return _tmp_dir() / "maven_install"

def install_maven() -> bool:
    _banner("Maven (mvn)")
    for name in ("mvn", "mvn.cmd"):
        if _which(name):
            print(f"  Maven is already installed: {shutil.which(name)}", flush=True)
            return True

    tmp = _tmp_dir()
    url = _maven_url()
    version_used = re.search(r'apache-maven-([\d.]+)-bin', url)
    v = version_used.group(1) if version_used else MAVEN_VERSION_FALLBACK
    print(f"  Latest Maven version: {v}", flush=True)
    archive = tmp / url.rsplit("/", 1)[-1]
    try:
        _download(url, archive)
    except Exception as e:
        # Fall back to Apache archive mirror
        print(f"  CDN failed ({e}), trying archive mirror ...", flush=True)
        url = (
            f"https://archive.apache.org/dist/maven/maven-3/{v}/binaries/"
            f"apache-maven-{v}-bin.tar.gz"
        )
        archive = tmp / url.rsplit("/", 1)[-1]
        try:
            _download(url, archive)
        except Exception as e2:
            print(f"  ERROR downloading Maven: {e2}", flush=True)
            return False

    install_dir = _maven_install_dir()
    if install_dir.exists():
        shutil.rmtree(install_dir)

    try:
        _extract(archive, install_dir)
        archive.unlink(missing_ok=True)
    except Exception as e:
        print(f"  ERROR extracting Maven: {e}", flush=True)
        return False

    # Archive extracts to apache-maven-<version>/
    extracted = install_dir / f"apache-maven-{v}"
    mvn_bin = extracted / "bin"
    mvn_exe = mvn_bin / ("mvn.cmd" if _os() == "Windows" else "mvn")

    if not mvn_exe.exists():
        print(f"  ERROR: mvn not found at {mvn_exe}", flush=True)
        return False

    os.environ["PATH"] = str(mvn_bin) + os.pathsep + os.environ.get("PATH", "")

    print(f"\n  Maven installed to: {mvn_bin}", flush=True)
    print(f"  Add to your PATH permanently: {mvn_bin}", flush=True)
    _run([str(mvn_exe), "--version"])

    if _os() == "Windows":
        print(f'\n  PowerShell: $env:PATH = "{mvn_bin}" + ";" + $env:PATH', flush=True)
    else:
        print(f'\n  Shell: export PATH="{mvn_bin}:$PATH"', flush=True)

    return True

# ── C3C ───────────────────────────────────────────────────────────────────────

C3C_VERSION = "0.6.5"

def _c3c_url() -> str:
    system = _os()
    arch = _arch()
    if system == "Windows":
        return f"https://github.com/c3lang/c3c/releases/download/v{C3C_VERSION}/c3-windows.zip"
    elif system == "Darwin":
        return f"https://github.com/c3lang/c3c/releases/download/v{C3C_VERSION}/c3-macos.tar.gz"
    else:
        return f"https://github.com/c3lang/c3c/releases/download/v{C3C_VERSION}/c3-linux.tar.gz"

def _c3c_install_dir() -> Path:
    return _tmp_dir() / "c3c_install"

def install_c3c() -> bool:
    _banner("c3c (C3 compiler)")
    if _which("c3c"):
        print(f"  c3c is already installed: {shutil.which('c3c')}", flush=True)
        _run(["c3c", "--version"])
        return True

    tmp = _tmp_dir()
    url = _c3c_url()
    archive = tmp / url.rsplit("/", 1)[-1]
    try:
        _download(url, archive)
    except Exception as e:
        print(f"  ERROR downloading c3c: {e}", flush=True)
        return False

    install_dir = _c3c_install_dir()
    if install_dir.exists():
        shutil.rmtree(install_dir)

    try:
        _extract(archive, install_dir)
        archive.unlink(missing_ok=True)
    except Exception as e:
        print(f"  ERROR extracting c3c: {e}", flush=True)
        return False

    # Find c3c executable anywhere inside the extracted dir
    exe_name = "c3c.exe" if _os() == "Windows" else "c3c"
    c3c_exe = None
    for p in install_dir.rglob(exe_name):
        c3c_exe = p
        break

    if not c3c_exe or not c3c_exe.exists():
        print(f"  ERROR: c3c executable not found under {install_dir}", flush=True)
        return False

    c3c_bin = c3c_exe.parent
    os.environ["PATH"] = str(c3c_bin) + os.pathsep + os.environ.get("PATH", "")

    print(f"\n  c3c installed to: {c3c_bin}", flush=True)
    print(f"  Add to your PATH permanently: {c3c_bin}", flush=True)
    _run([str(c3c_exe), "--version"])

    if _os() == "Windows":
        print(f'\n  PowerShell: $env:PATH = "{c3c_bin}" + ";" + $env:PATH', flush=True)
    else:
        print(f'\n  Shell: export PATH="{c3c_bin}:$PATH"', flush=True)

    return True

# ── V (vlang) ─────────────────────────────────────────────────────────────────

def _vlang_url() -> str:
    system = _os()
    if system == "Windows":
        return "https://github.com/vlang/v/releases/latest/download/v_windows.zip"
    elif system == "Darwin":
        return "https://github.com/vlang/v/releases/latest/download/v_macos_arm64.zip" \
               if _arch() == "arm64" else \
               "https://github.com/vlang/v/releases/latest/download/v_macos_x86_64.zip"
    else:
        return "https://github.com/vlang/v/releases/latest/download/v_linux.zip"

def _vlang_install_dir() -> Path:
    return _tmp_dir() / "vlang_install"

def install_vlang() -> bool:
    _banner("V compiler")
    if _which("v"):
        print(f"  V is already installed: {shutil.which('v')}", flush=True)
        _run(["v", "version"])
        return True

    tmp = _tmp_dir()
    url = _vlang_url()
    archive = tmp / url.rsplit("/", 1)[-1]
    try:
        _download(url, archive)
    except Exception as e:
        print(f"  ERROR downloading V: {e}", flush=True)
        return False

    install_dir = _vlang_install_dir()
    if install_dir.exists():
        shutil.rmtree(install_dir)

    try:
        _extract(archive, install_dir)
        archive.unlink(missing_ok=True)
    except Exception as e:
        print(f"  ERROR extracting V: {e}", flush=True)
        return False

    exe_name = "v.exe" if _os() == "Windows" else "v"
    v_exe = None
    for p in install_dir.rglob(exe_name):
        # Skip test binaries / examples named "v"
        if p.parent.name not in ("examples", "tests", "vlib"):
            v_exe = p
            break

    if not v_exe or not v_exe.exists():
        print(f"  ERROR: v executable not found under {install_dir}", flush=True)
        return False

    v_bin = v_exe.parent
    os.environ["PATH"] = str(v_bin) + os.pathsep + os.environ.get("PATH", "")

    print(f"\n  V installed to: {v_bin}", flush=True)
    print(f"  Add to your PATH permanently: {v_bin}", flush=True)
    _run([str(v_exe), "version"])

    if _os() == "Windows":
        print(f'\n  PowerShell: $env:PATH = "{v_bin}" + ";" + $env:PATH', flush=True)
    else:
        print(f'\n  Shell: export PATH="{v_bin}:$PATH"', flush=True)

    return True

# ── R ─────────────────────────────────────────────────────────────────────────

R_VERSION_FALLBACK = "4.6.0"

def _r_latest_version_windows() -> str:
    """Scrape the current R version from the CRAN Windows page."""
    try:
        req = urllib.request.Request(
            "https://cran.r-project.org/bin/windows/base/",
            headers={"User-Agent": "CoolBox-Installer/1.0"}
        )
        with urllib.request.urlopen(req, timeout=8) as r:
            body = r.read().decode("utf-8", errors="replace")
        m = re.search(r'R-(\d+\.\d+\.\d+)-win\.exe', body)
        if m:
            return m.group(1)
    except Exception:
        pass
    return R_VERSION_FALLBACK

def _r_url() -> str:
    system = _os()
    if system == "Windows":
        v = _r_latest_version_windows()
        print(f"  Latest R version: {v}", flush=True)
        return f"https://cran.r-project.org/bin/windows/base/R-{v}-win.exe"
    elif system == "Darwin":
        arch = _arch()
        # Use cloud.r-project.org which always has the latest .pkg
        if arch == "arm64":
            return f"https://cran.r-project.org/bin/macosx/big-sur-arm64/base/R-{R_VERSION_FALLBACK}-arm64.pkg"
        return f"https://cran.r-project.org/bin/macosx/big-sur-x86_64/base/R-{R_VERSION_FALLBACK}-x86_64.pkg"
    else:
        return ""

def install_r() -> bool:
    _banner("R")
    r_extra = [
        r"C:\Program Files\R\R-4.4.0\bin\Rscript.exe",
        r"C:\Program Files\R\R-4.3.0\bin\Rscript.exe",
        r"C:\Program Files\R\R-4.2.0\bin\Rscript.exe",
    ]
    if _which("Rscript", r_extra) or _which("R"):
        print(f"  R is already installed.", flush=True)
        _run(["Rscript", "--version"]) if shutil.which("Rscript") else None
        return True

    system = _os()

    if system == "Linux":
        print("  Attempting to install R via package manager ...", flush=True)
        # Detect distro and install
        if shutil.which("apt-get"):
            rc = _run_shell("apt-get install -y r-base")
        elif shutil.which("dnf"):
            rc = _run_shell("dnf install -y R")
        elif shutil.which("pacman"):
            rc = _run_shell("pacman -Sy --noconfirm r")
        elif shutil.which("brew"):
            rc = _run_shell("brew install r")
        else:
            print("  ERROR: No supported package manager found (apt-get/dnf/pacman/brew).", flush=True)
            print("  Install R manually from: https://cran.r-project.org", flush=True)
            return False
        return rc == 0

    if system == "Darwin":
        if shutil.which("brew"):
            print("  Installing R via Homebrew ...", flush=True)
            rc = _run_shell("brew install --cask r")
            return rc == 0
        # Fall back to downloading .pkg
        url = _r_url()
        tmp = _tmp_dir()
        archive = tmp / url.rsplit("/", 1)[-1]
        try:
            _download(url, archive)
        except Exception as e:
            print(f"  ERROR downloading R installer: {e}", flush=True)
            return False
        print("  Launching macOS installer (requires admin) ...", flush=True)
        rc = _run(["sudo", "installer", "-pkg", str(archive), "-target", "/"])
        archive.unlink(missing_ok=True)
        return rc == 0

    if system == "Windows":
        url = _r_url()
        tmp = _tmp_dir()
        installer = tmp / url.rsplit("/", 1)[-1]
        try:
            _download(url, installer)
        except Exception as e:
            print(f"  ERROR downloading R installer: {e}", flush=True)
            return False
        print("  Running R installer (a UAC prompt may appear) ...", flush=True)
        rc = _run([str(installer), "/SILENT", "/NORESTART"])
        installer.unlink(missing_ok=True)
        return rc == 0

# ── Rtools (Windows C++ toolchain for R packages) ───────────────────────────

def _rtools_dir() -> Path | None:
    """Return the Rtools bin dir if found in standard or managed locations."""
    for d in [
        r"C:\rtools45\x86_64-w64-mingw32.static.posix\bin",
        r"C:\rtools44\x86_64-w64-mingw32.static.posix\bin",
        r"C:\rtools43\x86_64-w64-mingw32.static.posix\bin",
        r"C:\rtools42\mingw64\bin",
        str(_tmp_dir() / "rtools45" / "x86_64-w64-mingw32.static.posix" / "bin"),
        str(Path.home() / "rtools45" / "x86_64-w64-mingw32.static.posix" / "bin"),
    ]:
        if os.path.isfile(os.path.join(d, "gcc.exe")):
            return Path(d)
    return None


def _rtools_installed() -> bool:
    return _rtools_dir() is not None


def install_rtools() -> bool:
    _banner("Rtools (Windows C++ toolchain for R packages)")
    if _rtools_installed():
        print(f"  Rtools is already installed at: {_rtools_dir()}", flush=True)
        return True

    if _os() != "Windows":
        print("  Rtools is only needed on Windows. On Linux/macOS gcc is used directly.", flush=True)
        return True

    # Rtools45 covers R 4.5+ (including R 4.6)
    url = "https://cran.r-project.org/bin/windows/Rtools/rtools45/files/rtools45-6768-6492.exe"
    tmp = _tmp_dir()
    installer = tmp / "rtools45.exe"
    try:
        _download(url, installer)
    except Exception as e:
        print(f"  ERROR downloading Rtools installer: {e}", flush=True)
        return False

    # Install to user profile to avoid requiring UAC
    install_dir = str(Path.home() / "rtools45")
    print(f"  Installing Rtools45 to {install_dir} (no admin required) ...", flush=True)
    rc = _run([str(installer), "/VERYSILENT", "/NORESTART", f"/DIR={install_dir}"])
    installer.unlink(missing_ok=True)
    if rc == 0 and _rtools_installed():
        print(f"  Rtools45 installed. gcc found at: {_rtools_dir()}", flush=True)
        return True
    print("  Rtools installer exited but gcc not found — a UAC prompt may have been needed.", flush=True)
    print(f"  Try installing manually from: {url}", flush=True)
    return False


# ── Emscripten (emsdk) ────────────────────────────────────────────────────────

def _emsdk_dir() -> Path:
    """Canonical install location used by both installer and build scripts."""
    return _tmp_dir() / "emsdk"

def _emcc_exe() -> Path | None:
    """Return the emcc executable inside the managed emsdk install, if present."""
    emsdk = _emsdk_dir()
    # emsdk activates into upstream/emscripten/
    candidate = emsdk / "upstream" / "emscripten" / ("emcc.bat" if _os() == "Windows" else "emcc")
    if candidate.exists():
        return candidate
    # Walk in case the layout differs
    for p in emsdk.rglob("emcc.bat" if _os() == "Windows" else "emcc"):
        if "upstream" in str(p) or "emscripten" in str(p.parent.name):
            return p
    return None

def install_emsdk() -> bool:
    _banner("Emscripten SDK (emsdk / emcc)")

    # Check if already available on PATH
    if shutil.which("emcc"):
        print(f"  emcc is already on PATH: {shutil.which('emcc')}", flush=True)
        _run(["emcc", "--version"])
        return True

    # Check if already installed in our managed location
    existing = _emcc_exe()
    if existing:
        os.environ["PATH"] = str(existing.parent) + os.pathsep + os.environ.get("PATH", "")
        print(f"  emcc already installed at: {existing}", flush=True)
        _run([str(existing), "--version"])
        return True

    emsdk_dir = _emsdk_dir()

    # Step 1: Obtain emsdk — prefer git clone, fall back to zip download
    if shutil.which("git"):
        if (emsdk_dir / "emsdk").exists() or (emsdk_dir / "emsdk.bat").exists():
            print(f"  Updating existing emsdk at {emsdk_dir} ...", flush=True)
            rc = _run(["git", "-C", str(emsdk_dir), "pull"], emsdk_dir)
        else:
            print(f"  Cloning emsdk from GitHub ...", flush=True)
            if emsdk_dir.exists():
                shutil.rmtree(emsdk_dir)
            rc = _run(["git", "clone", "--depth=1",
                        "https://github.com/emscripten-core/emsdk.git",
                        str(emsdk_dir)])
        if rc != 0:
            print("  git clone failed, falling back to zip download ...", flush=True)
    else:
        rc = 1  # force zip path

    if rc != 0 or not ((emsdk_dir / "emsdk").exists() or (emsdk_dir / "emsdk.bat").exists()):
        # Download the main branch zip
        url = "https://github.com/emscripten-core/emsdk/archive/refs/heads/main.zip"
        tmp = _tmp_dir()
        archive = tmp / "emsdk-main.zip"
        try:
            _download(url, archive)
        except Exception as e:
            print(f"  ERROR downloading emsdk: {e}", flush=True)
            return False
        extract_tmp = tmp / "emsdk_extract_tmp"
        if extract_tmp.exists():
            shutil.rmtree(extract_tmp)
        try:
            _extract(archive, extract_tmp)
            archive.unlink(missing_ok=True)
            # The zip extracts to emsdk-main/ — rename to our install dir
            extracted = extract_tmp / "emsdk-main"
            if not extracted.exists():
                # Some zips use the repo name directly
                subdirs = [d for d in extract_tmp.iterdir() if d.is_dir()]
                extracted = subdirs[0] if subdirs else extract_tmp
            if emsdk_dir.exists():
                shutil.rmtree(emsdk_dir)
            extracted.rename(emsdk_dir)
            shutil.rmtree(extract_tmp, ignore_errors=True)
        except Exception as e:
            print(f"  ERROR extracting emsdk: {e}", flush=True)
            return False

    # Locate the emsdk script
    emsdk_exe = emsdk_dir / ("emsdk.bat" if _os() == "Windows" else "emsdk")
    if not emsdk_exe.exists():
        print(f"  ERROR: emsdk script not found at {emsdk_exe}", flush=True)
        return False

    if _os() != "Windows":
        emsdk_exe.chmod(0o755)

    # Step 2: Install latest Emscripten (downloads LLVM/clang — can be 500 MB+)
    print("  Installing latest Emscripten (this may take several minutes) ...", flush=True)
    rc = _run([str(emsdk_exe), "install", "latest"], emsdk_dir)
    if rc != 0:
        print("  ERROR: emsdk install failed", flush=True)
        return False

    # Step 3: Activate
    print("  Activating latest Emscripten ...", flush=True)
    rc = _run([str(emsdk_exe), "activate", "latest"], emsdk_dir)
    if rc != 0:
        print("  ERROR: emsdk activate failed", flush=True)
        return False

    # Add emscripten bin to PATH for this process
    emcc = _emcc_exe()
    if emcc and emcc.exists():
        emcc_bin = emcc.parent
        os.environ["PATH"] = str(emcc_bin) + os.pathsep + os.environ.get("PATH", "")
        print(f"\n  Emscripten installed to: {emcc_bin}", flush=True)
        _run([str(emcc), "--version"])
    else:
        emcc_bin = emsdk_dir / "upstream" / "emscripten"
        print(f"\n  Emscripten installed. Expected emcc at: {emcc_bin}", flush=True)

    env_script = emsdk_dir / ("emsdk_env.bat" if _os() == "Windows" else "emsdk_env.sh")
    print(f"\n  To activate in every new shell, run:", flush=True)
    if _os() == "Windows":
        print(f'    {env_script}', flush=True)
        print(f"  Or add permanently via PowerShell:", flush=True)
        print(f'    $env:PATH = "{emcc_bin}" + ";" + $env:PATH', flush=True)
    else:
        print(f'    source {env_script}', flush=True)

    return True

# ── JDK (Eclipse Temurin 21 LTS) ─────────────────────────────────────────────

def _jdk_install_dir() -> Path:
    return _tmp_dir() / "jdk_install"

def _jdk_java_exe() -> Path | None:
    """Return the java.exe path inside the managed JDK install, if present."""
    import glob as _glob
    managed = _jdk_install_dir()
    _java = "java.exe" if _os() == "Windows" else "java"
    hits = _glob.glob(str(managed / "jdk-*" / "bin" / _java))
    return Path(hits[0]) if hits else None

def _jdk_installed() -> bool:
    if shutil.which("java"):
        return True
    exe = _jdk_java_exe()
    if exe and exe.exists():
        return True
    if _os() == "Windows":
        import glob as _glob
        for pattern in [
            r"C:\Program Files\Java\jdk-*\bin\java.exe",
            r"C:\Program Files\Eclipse Adoptium\jdk-*\bin\java.exe",
            r"C:\Program Files\Microsoft\jdk-*\bin\java.exe",
        ]:
            if _glob.glob(pattern):
                return True
    return False

def _jdk_url() -> str:
    """Return Eclipse Temurin 21 LTS binary download URL for the current OS."""
    system = _os()
    if system == "Windows":
        return ("https://api.adoptium.net/v3/binary/latest/21/ga/windows/x64/jdk/hotspot/normal/eclipse")
    if system == "Linux":
        return ("https://api.adoptium.net/v3/binary/latest/21/ga/linux/x64/jdk/hotspot/normal/eclipse")
    # macOS
    return ("https://api.adoptium.net/v3/binary/latest/21/ga/mac/x64/jdk/hotspot/normal/eclipse")

def install_jdk() -> bool:
    _banner("JDK 21 LTS (Eclipse Temurin)")
    if _jdk_installed():
        print("  JDK is already installed.", flush=True)
        return True

    url = _jdk_url()
    ext = ".zip" if _os() == "Windows" else ".tar.gz"
    tmp = _tmp_dir()
    archive = tmp / f"jdk21{ext}"
    try:
        _download(url, archive)
    except Exception as e:
        print(f"  ERROR downloading JDK: {e}", flush=True)
        return False

    install_dir = _jdk_install_dir()
    if install_dir.exists():
        shutil.rmtree(install_dir)

    try:
        _extract(archive, install_dir)
        archive.unlink(missing_ok=True)
    except Exception as e:
        print(f"  ERROR extracting JDK: {e}", flush=True)
        return False

    java_exe = _jdk_java_exe()
    if not java_exe or not java_exe.exists():
        print(f"  ERROR: java executable not found after extraction in {install_dir}", flush=True)
        return False

    java_home = str(java_exe.parent.parent)
    java_bin  = str(java_exe.parent)
    os.environ["JAVA_HOME"] = java_home
    os.environ["PATH"] = java_bin + os.pathsep + os.environ.get("PATH", "")

    print(f"\n  JDK installed to: {java_home}", flush=True)
    _run([str(java_exe), "-version"])

    if _os() == "Windows":
        print(f"\n  To make permanent, run in PowerShell:", flush=True)
        print(f'    $env:JAVA_HOME = "{java_home}"', flush=True)
        print(f'    $env:PATH = "{java_bin}" + ";" + $env:PATH', flush=True)
    else:
        print(f"\n  To make permanent, add to ~/.bashrc or ~/.zshrc:", flush=True)
        print(f'    export JAVA_HOME="{java_home}"', flush=True)
        print(f'    export PATH="{java_bin}:$PATH"', flush=True)

    return True


# ── Registry ─────────────────────────────────────────────────────────────────

TOOLS: dict[str, dict] = {
    "go":     {"label": "Go compiler",        "fn": install_go,     "check": ("go",)},
    "maven":  {"label": "Apache Maven",       "fn": install_maven,  "check": ("mvn", "mvn.cmd")},
    "jdk":    {"label": "JDK 21 (Temurin)",   "fn": install_jdk,    "check": ("java",)},
    "c3c":    {"label": "C3 compiler",        "fn": install_c3c,    "check": ("c3c",)},
    "vlang":  {"label": "V compiler",         "fn": install_vlang,  "check": ("v",)},
    "r":      {"label": "R / Rscript",        "fn": install_r,      "check": ("R", "Rscript")},
    "rtools": {"label": "Rtools (gcc for R)",  "fn": install_rtools, "check": ("gcc",)},
    "emsdk":  {"label": "Emscripten (emcc)",  "fn": install_emsdk,  "check": ("emcc",)},
}

def _is_installed(tool_key: str) -> bool:
    for name in TOOLS[tool_key]["check"]:
        if shutil.which(name):
            return True
    # JDK: check managed install and common system paths
    if tool_key == "jdk":
        return _jdk_installed()
    # Extra Windows paths for R
    if tool_key == "r":
        for p in [r"C:\Program Files\R\R-4.6.0\bin\Rscript.exe",
                  r"C:\Program Files\R\R-4.5.0\bin\Rscript.exe",
                  r"C:\Program Files\R\R-4.4.0\bin\Rscript.exe",
                  r"C:\Program Files\R\R-4.3.0\bin\Rscript.exe"]:
            if os.path.isfile(p):
                return True
    # Rtools: check known install locations
    if tool_key == "rtools":
        return _rtools_installed()
    # Check our managed install location for emsdk
    if tool_key == "emsdk":
        emcc = _emcc_exe()
        if emcc and emcc.exists():
            return True
    return False

def check_all() -> None:
    print(f"{'Tool':<10}  {'Label':<20}  Status", flush=True)
    print("-" * 50, flush=True)
    for key, meta in TOOLS.items():
        status = "INSTALLED" if _is_installed(key) else "not found"
        print(f"{key:<10}  {meta['label']:<20}  {status}", flush=True)

# ── Entry point ───────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Install language toolchains required by CoolBox bindings.",
    )
    parser.add_argument(
        "tools", nargs="*",
        choices=list(TOOLS.keys()) + ["all", "install_dep"],
        help="Tool(s) to install: go | maven | c3c | vlang | r | install_dep | all",
    )
    parser.add_argument("--all",   action="store_true", help="Install all tools")
    parser.add_argument("--check", action="store_true", help="Check which tools are installed")
    parser.add_argument("--list",  action="store_true", help="List available tools")
    args = parser.parse_args()

    if args.check:
        check_all()
        return 0

    if args.list:
        for key, meta in TOOLS.items():
            print(f"  {key:<10}  {meta['label']}", flush=True)
        return 0

    targets = list(TOOLS.keys()) if (args.all or "all" in args.tools) else args.tools
    if not targets:
        check_all()
        return 0

    results = {}
    for key in targets:
        if key not in TOOLS:
            print(f"Unknown tool: {key}", flush=True)
            results[key] = False
            continue
        ok = TOOLS[key]["fn"]()
        results[key] = ok
        print(f"\n>> {key}: {'OK' if ok else 'FAILED'}", flush=True)

    print(f"\n{'='*60}", flush=True)
    print(f"  Install summary ({len(results)} tool(s)):", flush=True)
    for key, ok in results.items():
        mark = "OK  " if ok else "FAIL"
        print(f"  [{mark}] {key}", flush=True)
    print(f"{'='*60}", flush=True)
    print(f"  {sum(results.values())}/{len(results)} succeeded", flush=True)

    all_ok = all(results.values())
    exit_code = 0 if all_ok else 1
    print(f"\n__EXIT_CODE__:{exit_code}", flush=True)
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
