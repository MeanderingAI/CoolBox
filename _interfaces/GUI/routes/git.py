from fastapi import APIRouter, BackgroundTasks, Request
from fastapi.responses import JSONResponse
import os
import subprocess
import re
from __init__ import REPO_ROOT

p1 = APIRouter()

@p1.post("/git/sub-repos/pull")
async def git_sub_repos_pull(background_tasks: BackgroundTasks):
    repo_root = REPO_ROOT
    sub_repos_dir = os.path.join(REPO_ROOT, "_sub_repos")
    if not os.path.isdir(sub_repos_dir):
        return JSONResponse({"success": False, "error": "_sub_repos/ directory not found"}, status_code=404)
    results = {}
    errors = {}
    for name in sorted(os.listdir(sub_repos_dir)):
        path = os.path.join(sub_repos_dir, name)
        if not os.path.exists(os.path.join(path, ".git")):
            continue
        try:
            branch = None
            try:
                branch = subprocess.check_output([
                    "git", "rev-parse", "--abbrev-ref", "HEAD"
                ], cwd=path, text=True, stderr=subprocess.STDOUT).strip()
            except Exception:
                pass
            if branch == "HEAD":
                checked_out = False
                for candidate in ["main", "master"]:
                    try:
                        subprocess.check_output([
                            "git", "checkout", candidate
                        ], cwd=path, text=True, stderr=subprocess.STDOUT)
                        checked_out = True
                        branch = candidate
                        break
                    except subprocess.CalledProcessError:
                        continue
                if not checked_out:
                    errors[name] = "Detached HEAD and could not checkout main or master."
                    continue
            out = subprocess.check_output([
                "git", "pull", "--recurse-submodules"
            ], cwd=path, text=True, stderr=subprocess.STDOUT)
            results[name] = f"[{branch}]\n" + out.strip()
        except subprocess.CalledProcessError as e:
            errors[name] = e.output or str(e)
        except Exception as e:
            errors[name] = str(e)
    if errors and not results:
        return JSONResponse({"success": False, "error": f"All sub-repo pulls failed: {errors}"}, status_code=500)
    return JSONResponse({"success": True, "results": results, "errors": errors})


@p1.get("/git/log")
def git_log(n: int = 60, branch: str = ""):
    """Return the last `n` git commits as structured JSON.
    Optional `branch` param filters to a specific branch."""
    repo_root = REPO_ROOT
    # Validate inputs
    if not (1 <= n <= 500):
        n = 60
    fmt = "%x1f".join(["%H", "%h", "%s", "%an", "%ae", "%ai", "%D"]) + "%x1e"
    cmd = ["git", "log", f"--max-count={n}", f"--format={fmt}", "--decorate=full"]
    if branch and re.match(r'^[\w\.\-/]+$', branch):
        cmd.append(branch)
    try:
        out = subprocess.check_output(cmd, cwd=repo_root, text=True, stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        return JSONResponse({"success": False, "commits": [], "error": "git log failed"})
    except FileNotFoundError:
        return JSONResponse({"success": False, "commits": [], "error": "git not found"})

    commits = []
    for record in out.strip().split("\x1e"):
        record = record.strip()
        if not record:
            continue
        parts = record.split("\x1f")
        if len(parts) < 6:
            continue
        sha, short, subject, author, email, date_iso = parts[:6]
        refs = parts[6] if len(parts) > 6 else ""
        # Parse branch/tag refs
        tags = [r.strip().removeprefix("refs/tags/") for r in refs.split(",")
                if "refs/tags/" in r]
        branches = [r.strip().removeprefix("refs/heads/")
                    .removeprefix("refs/remotes/")
                    for r in refs.split(",")
                    if "refs/heads/" in r or "refs/remotes/" in r]
        commits.append({
            "sha": sha.strip(),
            "short": short.strip(),
            "subject": subject.strip(),
            "author": author.strip(),
            "email": email.strip(),
            "date": date_iso.strip(),
            "branches": branches,
            "tags": tags,
        })

    # Also return list of local branches for the branch switcher
    try:
        branches_raw = subprocess.check_output(
            ["git", "branch", "--format=%(refname:short)"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip().splitlines()
    except Exception:
        branches_raw = []

    return JSONResponse({"success": True, "commits": commits, "branches": branches_raw})


@p1.get("/git/diff")
def git_diff(ref: str = ""):
    """Return a unified diff.
    If `ref` is a valid SHA, shows that commit via `git show`.
    Otherwise shows working tree vs HEAD via `git diff HEAD`."""
    repo_root = REPO_ROOT
    if ref and re.match(r'^[0-9a-fA-F]{4,40}$', ref):
        cmd = ["git", "show", ref, "--no-color", "--patch"]
        label = ref
    else:
        cmd = ["git", "diff", "HEAD", "--no-color"]
        label = "working tree vs HEAD"
    try:
        diff = subprocess.check_output(
            cmd, cwd=repo_root, stderr=subprocess.DEVNULL,
            encoding='utf-8', errors='replace',
        )
    except subprocess.CalledProcessError as e:
        diff = e.output or ""
    except FileNotFoundError:
        return JSONResponse({"success": False, "diff": "", "ref": label, "error": "git not found"})
    except Exception as e:
        return JSONResponse({"success": False, "diff": "", "ref": label, "error": str(e)})
    return JSONResponse({"success": True, "diff": diff, "ref": label})


@p1.get("/git/stash")
def git_stash_list():
    """Return the git stash list as structured entries."""
    repo_root = REPO_ROOT
    try:
        raw = subprocess.check_output(
            ["git", "stash", "list", "--format=%gd\x1f%s\x1f%ci"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return JSONResponse({"success": False, "entries": [], "error": "git stash list failed"})

    entries = []
    for line in raw.splitlines():
        parts = line.split("\x1f")
        ref = parts[0].strip() if len(parts) > 0 else ""
        msg = parts[1].strip() if len(parts) > 1 else ""
        date = parts[2].strip() if len(parts) > 2 else ""
        entries.append({"ref": ref, "message": msg, "date": date})
    return JSONResponse({"success": True, "entries": entries})


@p1.get("/git/stash/{index}")
def git_stash_show(index: int):
    """Return the unified diff for stash@{index}."""
    if not (0 <= index <= 99):
        return JSONResponse({"success": False, "diff": "", "error": "Invalid stash index"}, status_code=400)
    repo_root = REPO_ROOT
    try:
        diff = subprocess.check_output(
            ["git", "stash", "show", "-p", "--no-color", f"stash@{{{index}}}"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError:
        diff = ""
    except FileNotFoundError:
        return JSONResponse({"success": False, "diff": "", "error": "git not found"})
    return JSONResponse({"success": True, "diff": diff})


@p1.get("/git/remotes")
def git_remotes():
    """Return all configured git remotes with their fetch and push URLs."""
    repo_root = REPO_ROOT
    try:
        raw = subprocess.check_output(
            ["git", "remote", "-v"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return JSONResponse({"success": False, "remotes": [], "error": "git remote failed"})

    seen: dict = {}
    for line in raw.splitlines():
        parts = line.split()
        if len(parts) < 3:
            continue
        name, url, kind = parts[0], parts[1], parts[2].strip("()")
        if name not in seen:
            seen[name] = {"name": name, "fetch": "", "push": ""}
        seen[name][kind] = url

    return JSONResponse({"success": True, "remotes": list(seen.values())})


@p1.get("/git/branches")
def git_branches_all():
    """Return local and remote branches plus the current branch."""
    repo_root = REPO_ROOT
    try:
        local_raw = subprocess.check_output(
            ["git", "branch", "--format=%(refname:short)"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip().splitlines()
        remote_raw = subprocess.check_output(
            ["git", "branch", "-r", "--format=%(refname:short)"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip().splitlines()
        current = subprocess.check_output(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip()
    except (subprocess.CalledProcessError, FileNotFoundError) as exc:
        return JSONResponse({"success": False, "local": [], "remote": [], "current": "", "error": str(exc)})

    return JSONResponse({
        "success": True,
        "local": [b.strip() for b in local_raw if b.strip()],
        "remote": [b.strip() for b in remote_raw if b.strip()],
        "current": current,
    })


@p1.post("/git/checkout")
async def git_checkout(request: Request):
    """Check out a local branch by name."""
    body = await request.json()
    branch = body.get("branch", "").strip()
    if not branch or not re.match(r'^[\w\.\-/]+$', branch):
        return JSONResponse({"success": False, "error": "Invalid branch name"}, status_code=400)
    repo_root = REPO_ROOT
    try:
        out = subprocess.check_output(
            ["git", "checkout", branch],
            cwd=repo_root, text=True, stderr=subprocess.STDOUT,
        )
        return JSONResponse({"success": True, "output": out, "branch": branch})
    except subprocess.CalledProcessError as e:
        return JSONResponse({"success": False, "output": e.output or "", "error": "checkout failed"})
    except FileNotFoundError:
        return JSONResponse({"success": False, "error": "git not found"}, status_code=500)


@p1.post("/git/commit")
async def git_commit(request: Request):
    """Stage all changes (git add -A) and commit with the given message."""
    body = await request.json()
    message = body.get("message", "").strip()
    if not message:
        return JSONResponse({"success": False, "error": "Commit message is required"}, status_code=400)
    # Sanitise: no shell injection possible since we pass args as a list, but
    # still reject unusually short/empty messages caught above.
    repo_root = REPO_ROOT
    try:
        subprocess.check_output(
            ["git", "add", "-A"],
            cwd=repo_root, text=True, stderr=subprocess.STDOUT,
        )
        out = subprocess.check_output(
            ["git", "commit", "-m", message],
            cwd=repo_root, text=True, stderr=subprocess.STDOUT,
        )
        return JSONResponse({"success": True, "output": out})
    except subprocess.CalledProcessError as e:
        return JSONResponse({"success": False, "output": e.output or "", "error": "commit failed"})
    except FileNotFoundError:
        return JSONResponse({"success": False, "error": "git not found"}, status_code=500)


@p1.post("/git/merge")
async def git_merge(request: Request):
    """Merge a branch into the current branch (git merge --no-ff <branch>)."""
    body = await request.json()
    source = body.get("branch", "").strip()
    strategy = body.get("strategy", "no-ff").strip()
    if not source or not re.match(r'^[\w\.\-/]+$', source):
        return JSONResponse({"success": False, "error": "Invalid branch name"}, status_code=400)
    if strategy not in ("no-ff", "ff", "squash"):
        return JSONResponse({"success": False, "error": "Invalid strategy"}, status_code=400)
    repo_root = REPO_ROOT
    flag = {"no-ff": "--no-ff", "ff": "--ff-only", "squash": "--squash"}[strategy]
    try:
        current = subprocess.check_output(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            cwd=repo_root, text=True, stderr=subprocess.DEVNULL,
        ).strip()
        out = subprocess.check_output(
            ["git", "merge", flag, source],
            cwd=repo_root, text=True, stderr=subprocess.STDOUT,
        )
        return JSONResponse({"success": True, "output": out, "source": source, "target": current})
    except subprocess.CalledProcessError as e:
        return JSONResponse({"success": False, "output": e.output or "", "error": "merge failed"})
    except FileNotFoundError:
        return JSONResponse({"success": False, "error": "git not found"}, status_code=500)


@p1.get("/git/sub-repos")
async def git_sub_repos():
    """Return status of each repo inside _sub_repos/."""
    repo_root = REPO_ROOT
    sub_repos_dir = os.path.join(repo_root, "_sub_repos")
    results = []
    if not os.path.isdir(sub_repos_dir):
        return JSONResponse({"success": True, "repos": []})
    for name in sorted(os.listdir(sub_repos_dir)):
        path = os.path.join(sub_repos_dir, name)
        if not os.path.exists(os.path.join(path, ".git")):
            continue
        entry = {"name": name, "path": os.path.join("_sub_repos", name)}
        try:
            entry["branch"] = subprocess.check_output(
                ["git", "rev-parse", "--abbrev-ref", "HEAD"],
                cwd=path, text=True, stderr=subprocess.DEVNULL,
            ).strip()
            log = subprocess.check_output(
                ["git", "log", "-1", "--format=%H%x00%s%x00%an%x00%ai"],
                cwd=path, text=True, stderr=subprocess.DEVNULL,
            ).strip()
            if log:
                sha, subject, author, date = log.split("\x00", 3)
                entry["sha"] = sha[:8]
                entry["subject"] = subject
                entry["author"] = author
                entry["date"] = date.strip()
            else:
                entry["sha"] = entry["subject"] = entry["author"] = entry["date"] = ""
            status_out = subprocess.check_output(
                ["git", "status", "--porcelain"],
                cwd=path, text=True, stderr=subprocess.DEVNULL,
            )
            lines_out = [l for l in status_out.splitlines() if l.strip()]
            entry["dirty"] = len(lines_out) > 0
            entry["changed_files"] = len(lines_out)
            remotes_out = subprocess.check_output(
                ["git", "remote", "-v"],
                cwd=path, text=True, stderr=subprocess.DEVNULL,
            )
            remote_urls = {}
            for rline in remotes_out.splitlines():
                parts = rline.split()
                if len(parts) >= 2 and parts[0] not in remote_urls:
                    remote_urls[parts[0]] = parts[1]
            entry["remotes"] = remote_urls
        except (subprocess.CalledProcessError, FileNotFoundError, ValueError):
            entry.setdefault("branch", "unknown")
            entry.setdefault("sha", "")
            entry.setdefault("subject", "")
            entry.setdefault("author", "")
            entry.setdefault("date", "")
            entry.setdefault("dirty", False)
            entry.setdefault("changed_files", 0)
            entry.setdefault("remotes", {})
        results.append(entry)
    return JSONResponse({"success": True, "repos": results})