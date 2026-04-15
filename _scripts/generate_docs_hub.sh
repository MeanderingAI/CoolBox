#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SITE_DIR="${1:-${ROOT_DIR}/.site}"
CPP_INPUT_DIR="${ROOT_DIR}/_libraries/include"
CPP_PACKAGES_DIR="${ROOT_DIR}/_libraries/packages"
R_PKG_DIR="${ROOT_DIR}/_libraries/r_bindings/coolboxr"
R_DOCS_DIR="${DOCS_R_DIR:-${R_PKG_DIR}/docs}"
R_DIST_DIR="${R_PKG_DIR}/dist"
PYTHON_DOC_MD="${ROOT_DIR}/__GENERATED_CONTENT/read_mes/python_bindings_README.md"
EMSCRIPTEN_DIR="${ROOT_DIR}/_libraries/emscripten_bindings"
GO_BINDINGS_DIR="${ROOT_DIR}/_libraries/go_bindings"
GO_DIST_DIR="${GO_BINDINGS_DIR}/dist"
RUST_DOC_MD="${ROOT_DIR}/_libraries/rust_bindings/README.md"
RUST_BINDINGS_DIR="${ROOT_DIR}/_libraries/rust_bindings"
RUST_DIST_DIR="${RUST_BINDINGS_DIR}/dist"
PYTHON_BINDINGS_DIR="${ROOT_DIR}/_libraries/python_bindings"
PYTHON_BUILD_DIR="${PYTHON_BINDINGS_DIR}/build"
PYTHON_DIST_DIR="${PYTHON_BINDINGS_DIR}/dist"
JS_BUILD_DIR="${ROOT_DIR}/build-emscripten"
C_BINDINGS_DIR="${ROOT_DIR}/_libraries/c_bindings"
C_TARGET_DIR="${C_BINDINGS_DIR}/target"
JAVA_BINDINGS_DIR="${ROOT_DIR}/_libraries/java_bindings"
JAVA_TARGET_DIR="${JAVA_BINDINGS_DIR}/target"
REPOSITORY_SLUG="${DOCS_REPOSITORY:-${GITHUB_REPOSITORY:-}}"
RELEASE_TAG="${DOCS_RELEASE_TAG:-}"
PREBUILT_CPP_DOCS_DIR="${DOCS_CPP_DIR:-}"
TUTORIALS_INPUT_DIR="${DOCS_TUTORIALS_DIR:-${ROOT_DIR}/build/documentation_site}"

if [ -z "${REPOSITORY_SLUG}" ] && command -v git >/dev/null 2>&1; then
  remote_url="$(git -C "${ROOT_DIR}" config --get remote.origin.url 2>/dev/null || true)"
  case "${remote_url}" in
    https://github.com/*)
      REPOSITORY_SLUG="${remote_url#https://github.com/}"
      REPOSITORY_SLUG="${REPOSITORY_SLUG%.git}"
      ;;
    git@github.com:*)
      REPOSITORY_SLUG="${remote_url#git@github.com:}"
      REPOSITORY_SLUG="${REPOSITORY_SLUG%.git}"
      ;;
  esac
fi

# NOTE: preserve existing output directories so pre-built sections (publications, references, etc.) are not deleted.
#rm -rf "${SITE_DIR}"
mkdir -p "${SITE_DIR}" "${SITE_DIR}/extensions" "${SITE_DIR}/artifacts"
printf '' > "${SITE_DIR}/.nojekyll"

has_cpp_docs=false
has_r_docs=false
has_python_docs=false
has_go_docs=false
has_js_docs=false
has_c_docs=false
has_java_docs=false
has_rust_docs=false
has_tutorials=false
has_products=false
has_c_artifacts=false
has_r_artifacts=false
has_python_artifacts=false
has_go_artifacts=false
has_js_artifacts=false
has_java_artifacts=false
has_rust_artifacts=false
has_product_artifacts=false
tutorials_latest_posts=''
publications_latest_posts=''

release_asset_url() {
  local asset_name="$1"

  if [ -z "${REPOSITORY_SLUG}" ] || [ -z "${RELEASE_TAG}" ]; then
    return 1
  fi

  printf 'https://github.com/%s/releases/download/%s/%s' "${REPOSITORY_SLUG}" "${RELEASE_TAG}" "${asset_name}"
}

latest_release_page_url() {
  if [ -z "${REPOSITORY_SLUG}" ]; then
    return 1
  fi

  printf 'https://github.com/%s/releases/latest' "${REPOSITORY_SLUG}"
}

releases_page_url() {
  if [ -z "${REPOSITORY_SLUG}" ]; then
    return 1
  fi

  printf 'https://github.com/%s/releases' "${REPOSITORY_SLUG}"
}

html_escape() {
  printf '%s' "$1" | sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/g'
}

release_links_html() {
  local extension_name="$1"
  shift
  local html=''
  local platform

  if [ -z "${REPOSITORY_SLUG}" ] || [ -z "${RELEASE_TAG}" ]; then
    printf ''
    return 0
  fi

  for platform in "$@"; do
    local asset_base="coolbox-${extension_name}-${platform}-${RELEASE_TAG}"
    local label
    case "${platform}" in
      linux-x86_64) label='Linux' ;;
      macos-arm64) label='macOS' ;;
      windows-x86_64) label='Windows' ;;
      *) label="${platform}" ;;
    esac

    local tar_url zip_url asset_links
    tar_url="$(release_asset_url "${asset_base}.tar.gz")"
    zip_url="$(release_asset_url "${asset_base}.zip")"
    asset_links="<a href=\"${tar_url}\">tar.gz</a> · <a href=\"${zip_url}\">zip</a>"
    if [ -n "${html}" ]; then
      html="${html} · "
    fi
    html="${html}${label} (${asset_links})"
  done

  if [ -n "${html}" ]; then
    printf '<p class="muted">Release assets (%s): %s</p>' "${RELEASE_TAG}" "${html}"
  fi
}

single_release_links_html() {
  local label="$1"
  shift

  if [ -z "${REPOSITORY_SLUG}" ] || [ -z "${RELEASE_TAG}" ]; then
    printf ''
    return 0
  fi

  local html=''
  local item_text asset_name asset_url

  while [ "$#" -gt 1 ]; do
    item_text="$1"
    asset_name="$2"
    shift 2

    asset_url="$(release_asset_url "${asset_name}")"
    if [ -n "${html}" ]; then
      html="${html} · "
    fi
    html="${html}<a href=\"${asset_url}\">${item_text}</a>"
  done

  if [ -n "${html}" ]; then
    printf '<p class="muted">Release assets (%s): %s (%s)</p>' "${RELEASE_TAG}" "${label}" "${html}"
  fi
}

if [ -n "${PREBUILT_CPP_DOCS_DIR}" ] && [ -d "${PREBUILT_CPP_DOCS_DIR}" ]; then
  mkdir -p "${SITE_DIR}/cpp"
  cp -R "${PREBUILT_CPP_DOCS_DIR}/." "${SITE_DIR}/cpp/"
  has_cpp_docs=true
elif command -v doxygen >/dev/null 2>&1 && { [ -d "${CPP_INPUT_DIR}" ] || [ -d "${CPP_PACKAGES_DIR}" ]; }; then
  cpp_inputs=()
  if [ -d "${CPP_INPUT_DIR}" ]; then
    cpp_inputs+=("${CPP_INPUT_DIR}")
  fi
  if [ -d "${CPP_PACKAGES_DIR}" ]; then
    cpp_inputs+=("${CPP_PACKAGES_DIR}")
  fi

  cat > "${SITE_DIR}/Doxyfile" <<EOF
PROJECT_NAME = "CoolBox C++ API"
OUTPUT_DIRECTORY = ${SITE_DIR}
INPUT = ${cpp_inputs[*]}
RECURSIVE = YES
FILE_PATTERNS = *.h *.hpp
GENERATE_HTML = YES
HTML_OUTPUT = cpp
GENERATE_LATEX = NO
EXTRACT_ALL = YES
EXCLUDE_PATTERNS = */build/* */_deps/* */googletest-*/* */eigen-*/*
QUIET = YES
WARN_IF_UNDOCUMENTED = NO
WARN_IF_DOC_ERROR = YES
EOF
  doxygen "${SITE_DIR}/Doxyfile"
  rm -f "${SITE_DIR}/Doxyfile"
  has_cpp_docs=true
fi

if [ -d "${R_DOCS_DIR}" ]; then
  mkdir -p "${SITE_DIR}/extensions/r"
  cp -R "${R_DOCS_DIR}/." "${SITE_DIR}/extensions/r/"
  has_r_docs=true
fi

if [ -d "${R_DIST_DIR}" ]; then
  mkdir -p "${SITE_DIR}/artifacts/r"
  while IFS= read -r artifact; do
    cp "${artifact}" "${SITE_DIR}/artifacts/r/"
    has_r_artifacts=true
  done < <(find "${R_DIST_DIR}" -maxdepth 1 -type f \( -name '*.tar.gz' -o -name '*.zip' \) 2>/dev/null)

  if [ "${has_r_artifacts}" = true ]; then
    {
      cat <<'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox R Binding Artifacts</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; color: #1f2937; }
    h1 { color: #111827; }
    li { margin: 0.5rem 0; }
    a { color: #2563eb; text-decoration: none; }
    a:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <h1>CoolBox R Binding Artifacts</h1>
  <ul>
EOF
      find "${SITE_DIR}/artifacts/r" -maxdepth 1 -type f ! -name 'index.html' | sort | while read -r file; do
        base="$(basename "${file}")"
        printf '    <li><a href="%s">%s</a></li>\n' "${base}" "${base}"
      done
      cat <<'EOF'
  </ul>
  <p><a href="../../index.html">Back to docs index</a></p>
</body>
</html>
EOF
    } > "${SITE_DIR}/artifacts/r/index.html"
  fi
fi

if command -v pandoc >/dev/null 2>&1 && [ -f "${PYTHON_DOC_MD}" ]; then
  mkdir -p "${SITE_DIR}/extensions/python"
  pandoc "${PYTHON_DOC_MD}" \
    --standalone \
    --metadata title="CoolBox Python Bindings" \
    --output "${SITE_DIR}/extensions/python/index.html"
  has_python_docs=true
fi

if [ -f "${ROOT_DIR}/_libraries/go_bindings/go.mod" ]; then
  mkdir -p "${SITE_DIR}/extensions/go"
  go_module_name="$(sed -n 's/^module[[:space:]]\+//p' "${ROOT_DIR}/_libraries/go_bindings/go.mod" | head -n 1)"
  go_install_section=''
  go_import_example=''

  if [ -n "${go_module_name}" ]; then
    go_install_cmd="$(html_escape "go get ${go_module_name}@latest")"
    go_import_cmd="$(html_escape "import coolboxgo \"${go_module_name}\"")"
    go_install_section="<h2>Install from GitHub</h2><pre><code>${go_install_cmd}</code></pre>"
    go_import_example="<h2>Import</h2><pre><code>${go_import_cmd}</code></pre>"
  fi

  cat > "${SITE_DIR}/extensions/go/index.html" <<EOF
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Go Bindings</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; color: #1f2937; }
    h1, h2 { color: #111827; }
    .muted { color: #6b7280; }
    pre { background: #0f172a; color: #e2e8f0; padding: 0.85rem 1rem; border-radius: 10px; overflow-x: auto; }
    code { font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace; }
    a { color: #2563eb; text-decoration: none; }
    a:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <h1>CoolBox Go Bindings</h1>
  <p class="muted">Go bindings packaged as a standalone module in <code>_libraries/go_bindings</code>.</p>
  <p><strong>Module path:</strong> <code>$(html_escape "${go_module_name}")</code></p>
  ${go_install_section}
  ${go_import_example}
  <p><a href="../../index.html">Back to docs index</a></p>
</body>
</html>
EOF
  has_go_docs=true
fi

if [ -d "${GO_DIST_DIR}" ]; then
  mkdir -p "${SITE_DIR}/artifacts/go"
  while IFS= read -r artifact; do
    cp "${artifact}" "${SITE_DIR}/artifacts/go/"
    has_go_artifacts=true
  done < <(find "${GO_DIST_DIR}" -maxdepth 1 -type f \( -name '*.tar.gz' -o -name '*.zip' \) 2>/dev/null)

  if [ "${has_go_artifacts}" = true ]; then
    {
      cat <<'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Go Binding Artifacts</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; color: #1f2937; }
    h1 { color: #111827; }
    li { margin: 0.5rem 0; }
    a { color: #2563eb; text-decoration: none; }
    a:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <h1>CoolBox Go Binding Artifacts</h1>
  <ul>
EOF
      find "${SITE_DIR}/artifacts/go" -maxdepth 1 -type f ! -name 'index.html' | sort | while read -r file; do
        base="$(basename "${file}")"
        printf '    <li><a href="%s">%s</a></li>\n' "${base}" "${base}"
      done
      cat <<'EOF'
  </ul>
  <p><a href="../../index.html">Back to docs index</a></p>
</body>
</html>
EOF
    } > "${SITE_DIR}/artifacts/go/index.html"
  fi
fi

if command -v pandoc >/dev/null 2>&1 && [ -f "${RUST_DOC_MD}" ]; then
  mkdir -p "${SITE_DIR}/extensions/rust"
  pandoc "${RUST_DOC_MD}" \
    --standalone \
    --metadata title="CoolBox Rust Bindings" \
    --output "${SITE_DIR}/extensions/rust/index.html"
  has_rust_docs=true
fi

if [ -d "${RUST_DIST_DIR}" ]; then
  mkdir -p "${SITE_DIR}/artifacts/rust"
  while IFS= read -r artifact; do
    cp "${artifact}" "${SITE_DIR}/artifacts/rust/"
    has_rust_artifacts=true
  done < <(find "${RUST_DIST_DIR}" -maxdepth 1 -type f \( -name '*.crate' -o -name '*.tar.gz' -o -name '*.zip' \) 2>/dev/null)

  if [ "${has_rust_artifacts}" = true ]; then
    {
      cat <<'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Rust Binding Artifacts</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; color: #1f2937; }
    h1 { color: #111827; }
    li { margin: 0.5rem 0; }
    a { color: #2563eb; text-decoration: none; }
    a:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <h1>CoolBox Rust Binding Artifacts</h1>
  <ul>
EOF
      find "${SITE_DIR}/artifacts/rust" -maxdepth 1 -type f ! -name 'index.html' | sort | while read -r file; do
        base="$(basename "${file}")"
        printf '    <li><a href="%s">%s</a></li>\n' "${base}" "${base}"
      done
      cat <<'EOF'
  </ul>
  <p><a href="../../index.html">Back to docs index</a></p>
</body>
</html>
EOF
    } > "${SITE_DIR}/artifacts/rust/index.html"
  fi
fi

if [ -d "${PYTHON_DIST_DIR}" ] || [ -d "${PYTHON_BUILD_DIR}" ]; then
  mkdir -p "${SITE_DIR}/artifacts/python"
  while IFS= read -r artifact; do
    cp "${artifact}" "${SITE_DIR}/artifacts/python/"
    has_python_artifacts=true
  done < <(
    find "${PYTHON_DIST_DIR}" -maxdepth 1 -type f \( -name '*.whl' -o -name '*.tar.gz' -o -name '*.zip' \) 2>/dev/null
    find "${PYTHON_BUILD_DIR}" -type f \( -name '*.so' -o -name '*.pyd' \) 2>/dev/null
  )

  if [ "${has_python_artifacts}" = true ]; then
    {
      cat <<'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Python Binding Artifacts</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; color: #1f2937; }
    h1 { color: #111827; }
    li { margin: 0.5rem 0; }
    a { color: #2563eb; text-decoration: none; }
    a:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <h1>CoolBox Python Binding Artifacts</h1>
  <ul>
EOF
      find "${SITE_DIR}/artifacts/python" -maxdepth 1 -type f ! -name 'index.html' | sort | while read -r file; do
        base="$(basename "${file}")"
        printf '    <li><a href="%s">%s</a></li>\n' "${base}" "${base}"
      done
      cat <<'EOF'
  </ul>
  <p><a href="../../index.html">Back to docs index</a></p>
</body>
</html>
EOF
    } > "${SITE_DIR}/artifacts/python/index.html"
  fi
fi

if [ -d "${C_TARGET_DIR}/docs" ] && [ -f "${C_TARGET_DIR}/docs/index.html" ]; then
  mkdir -p "${SITE_DIR}/extensions/c"
  cp -R "${C_TARGET_DIR}/docs/." "${SITE_DIR}/extensions/c/"
  has_c_docs=true
fi

if [ -d "${C_TARGET_DIR}/lib" ] || [ -d "${C_TARGET_DIR}/include" ]; then
  mkdir -p "${SITE_DIR}/artifacts/c"

  if [ -d "${C_TARGET_DIR}/lib" ]; then
    mkdir -p "${SITE_DIR}/artifacts/c/lib"
    cp -R "${C_TARGET_DIR}/lib/." "${SITE_DIR}/artifacts/c/lib/"
    has_c_artifacts=true
  fi

  if [ -d "${C_TARGET_DIR}/include" ]; then
    mkdir -p "${SITE_DIR}/artifacts/c/include"
    cp -R "${C_TARGET_DIR}/include/." "${SITE_DIR}/artifacts/c/include/"
    has_c_artifacts=true
  fi

  if [ "${has_c_artifacts}" = true ]; then
    {
      cat <<'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox C Binding Artifacts</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; color: #1f2937; }
    h1 { color: #111827; }
    li { margin: 0.5rem 0; }
    a { color: #2563eb; text-decoration: none; }
    a:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <h1>CoolBox C Binding Artifacts</h1>
  <ul>
EOF
      find "${SITE_DIR}/artifacts/c" -type f ! -name 'index.html' | sort | while read -r file; do
        rel="${file#${SITE_DIR}/artifacts/c/}"
        printf '    <li><a href="%s">%s</a></li>\n' "${rel}" "${rel}"
      done
      cat <<'EOF'
  </ul>
  <p><a href="../../index.html">Back to docs index</a></p>
</body>
</html>
EOF
    } > "${SITE_DIR}/artifacts/c/index.html"
  fi
fi

if [ -d "${EMSCRIPTEN_DIR}" ]; then
  mkdir -p "${SITE_DIR}/extensions/javascript"
  {
    cat <<'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox JavaScript Bindings</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; color: #1f2937; }
    h1, h2 { color: #111827; }
    code { background: #f3f4f6; padding: 0.125rem 0.35rem; border-radius: 4px; }
    li { margin: 0.5rem 0; }
    a { color: #2563eb; text-decoration: none; }
    a:hover { text-decoration: underline; }
    .muted { color: #6b7280; }
  </style>
</head>
<body>
  <h1>CoolBox JavaScript / Emscripten Bindings</h1>
  <p class="muted">This page indexes the available Emscripten binding translation units in the repository.</p>
  <h2>Binding modules</h2>
  <ul>
EOF
    find "${EMSCRIPTEN_DIR}" -maxdepth 1 -name '*_bindings.cpp' -type f | sort | while read -r file; do
      base="$(basename "${file}")"
      module="${base%_bindings.cpp}"
      printf '    <li><strong>%s</strong> <span class="muted">(%s)</span></li>\n' "${module}" "${base}"
    done
    cat <<'EOF'
  </ul>
  <p><a href="../../index.html">Back to docs index</a></p>
</body>
</html>
EOF
  } > "${SITE_DIR}/extensions/javascript/index.html"
  has_js_docs=true
fi

if [ -d "${JS_BUILD_DIR}" ]; then
  mkdir -p "${SITE_DIR}/artifacts/javascript"
  while IFS= read -r artifact; do
    cp "${artifact}" "${SITE_DIR}/artifacts/javascript/"
    has_js_artifacts=true
  done < <(find "${JS_BUILD_DIR}" -maxdepth 1 -type f \( -name '*.js' -o -name '*.wasm' \) 2>/dev/null)

  if [ "${has_js_artifacts}" = true ]; then
    {
      cat <<'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox JavaScript Binding Artifacts</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; color: #1f2937; }
    h1 { color: #111827; }
    li { margin: 0.5rem 0; }
    a { color: #2563eb; text-decoration: none; }
    a:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <h1>CoolBox JavaScript Binding Artifacts</h1>
  <ul>
EOF
      find "${SITE_DIR}/artifacts/javascript" -maxdepth 1 -type f ! -name 'index.html' | sort | while read -r file; do
        base="$(basename "${file}")"
        printf '    <li><a href="%s">%s</a></li>\n' "${base}" "${base}"
      done
      cat <<'EOF'
  </ul>
  <p><a href="../../index.html">Back to docs index</a></p>
</body>
</html>
EOF
    } > "${SITE_DIR}/artifacts/javascript/index.html"
  fi
fi

if [ -d "${JAVA_TARGET_DIR}/site/apidocs" ]; then
  mkdir -p "${SITE_DIR}/extensions/java"
  cp -R "${JAVA_TARGET_DIR}/site/apidocs/." "${SITE_DIR}/extensions/java/"
  has_java_docs=true
fi

if [ -d "${JAVA_TARGET_DIR}" ]; then
  mkdir -p "${SITE_DIR}/artifacts/java"
  while IFS= read -r artifact; do
    cp "${artifact}" "${SITE_DIR}/artifacts/java/"
    has_java_artifacts=true
  done < <(find "${JAVA_TARGET_DIR}" -maxdepth 1 -type f \( -name '*.jar' -o -name '*.pom' \) 2>/dev/null)

  if [ "${has_java_artifacts}" = true ]; then
    {
      cat <<'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Java Binding Artifacts</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; color: #1f2937; }
    h1 { color: #111827; }
    li { margin: 0.5rem 0; }
    a { color: #2563eb; text-decoration: none; }
    a:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <h1>CoolBox Java Binding Artifacts</h1>
  <ul>
EOF
      find "${SITE_DIR}/artifacts/java" -maxdepth 1 -type f ! -name 'index.html' | sort | while read -r file; do
        base="$(basename "${file}")"
        printf '    <li><a href="%s">%s</a></li>\n' "${base}" "${base}"
      done
      cat <<'EOF'
  </ul>
  <p><a href="../../index.html">Back to docs index</a></p>
</body>
</html>
EOF
    } > "${SITE_DIR}/artifacts/java/index.html"
  fi
fi

if [ -d "${TUTORIALS_INPUT_DIR}" ] && [ -f "${TUTORIALS_INPUT_DIR}/index.html" ]; then
  mkdir -p "${SITE_DIR}/tutorials"

  # Avoid self-copy when tutorials input already points at .site/tutorials
  if [ "$(cd "${TUTORIALS_INPUT_DIR}" && pwd)" != "$(cd "${SITE_DIR}/tutorials" && pwd)" ]; then
    cp -R "${TUTORIALS_INPUT_DIR}/." "${SITE_DIR}/tutorials/"
  else
    echo "[generate_docs_hub.sh] Tutorials input already at ${SITE_DIR}/tutorials; skipping copy"
  fi

  # Remove publication/reference folders from tutorials output, they belong at root /publications and /references.
  for subfolder in publications references; do
    if [ -d "${SITE_DIR}/tutorials/${subfolder}" ]; then
      echo "[generate_docs_hub.sh] Removing tutorials/${subfolder} (generated as part of tutorial input)"
      rm -rf "${SITE_DIR}/tutorials/${subfolder}"
    fi
  done

  has_tutorials=true
  echo "[generate_docs_hub.sh] Generating latest tutorial posts from ${SITE_DIR}/tutorials"
  tutorials_latest_posts="$(TUTORIALS_DIR="${SITE_DIR}/tutorials" PYTHONWARNINGS=ignore python3 - <<'PY'
import html
import os
import re
import warnings
from datetime import datetime, timezone
from pathlib import Path

tutorials_dir = Path(os.environ["TUTORIALS_DIR"])
pages = [page for page in tutorials_dir.glob("*.html") if page.name not in ("index.html", "tags.html")]
pages.sort(key=lambda page: page.stat().st_mtime, reverse=True)

def title_for(page: Path) -> str:
    text = page.read_text(encoding="utf-8", errors="ignore")
    match = re.search(r"<h1>(.*?)</h1>", text, re.S)
    if match:
        title = re.sub(r"<[^>]+>", "", match.group(1)).strip()
        if title:
            return html.unescape(title)
    match = re.search(r"<title>(.*?)</title>", text, re.S)
    if match:
        title = html.unescape(match.group(1).split("|", 1)[0].strip())
        if title:
            return title
    return page.stem.replace("-", " ").title()

items = []
for page in pages[:3]:
    title = html.escape(title_for(page))
    with warnings.catch_warnings():
        warnings.filterwarnings("ignore", category=DeprecationWarning)
        stamp = datetime.fromtimestamp(page.stat().st_mtime, timezone.utc).strftime("%Y-%m-%d")
    items.append(f'<li><a href="tutorials/{page.name}">{title}</a><p class="muted">Latest tutorial post · published {stamp}</p></li>')

print("".join(items))
PY
)"
  echo "[generate_docs_hub.sh] Generated tutorials_latest_posts length $(printf '%s' "${tutorials_latest_posts}" | wc -c) chars"
fi

echo "[generate_docs_hub.sh] Preparing release links"

cpp_link=''
r_link=''
python_link=''
go_link=''
js_link=''
c_link=''
java_link=''
rust_link=''
tutorials_link=''
publications_link=''
products_link=''
dependency_link=''
c_artifacts_link=''
r_artifacts_link=''
python_artifacts_link=''
go_artifacts_link=''
js_artifacts_link=''
java_artifacts_link=''
rust_artifacts_link=''
product_artifacts_link=''
c_release_links=''
python_release_links=''
go_release_links=''
js_release_links=''
r_release_links=''
java_release_links=''
rust_release_links=''
latest_release_link=''

python_release_links="$(release_links_html 'python-bindings' linux-x86_64 macos-arm64 windows-x86_64)"
go_release_links="$(release_links_html 'go-bindings' linux-x86_64 macos-arm64 windows-x86_64)"
js_release_links="$(release_links_html 'js-bindings' linux-x86_64 macos-arm64 windows-x86_64)"
r_release_links="$(release_links_html 'r-bindings' linux-x86_64 macos-arm64 windows-x86_64)"
c_release_links="$(release_links_html 'c-bindings' linux-x86_64)"
if [ -n "${REPOSITORY_SLUG}" ] && [ -n "${RELEASE_TAG}" ]; then
  echo "[generate_docs_hub.sh] Placing java nd rust bindings."
  java_release_links="$(single_release_links_html 'Java package' 'tar.gz' "coolbox-java-bindings-${RELEASE_TAG}.tar.gz" 'zip' "coolbox-java-bindings-${RELEASE_TAG}.zip")"
  rust_release_links="$(single_release_links_html 'Rust package' 'crate' "coolbox-rust-bindings-${RELEASE_TAG}.crate" 'tar.gz' "coolbox-rust-bindings-${RELEASE_TAG}.tar.gz" 'zip' "coolbox-rust-bindings-${RELEASE_TAG}.zip")"
fi

if [ -n "${REPOSITORY_SLUG}" ]; then
  echo "[generate_docs_hub.sh] Placing latest release and releases page links for repository ${REPOSITORY_SLUG}."
  latest_release_url="$(latest_release_page_url)"
  releases_url="$(releases_page_url)"
  latest_release_link="<p class=\"muted\">GitHub releases: <a href=\"${latest_release_url}\">Latest release</a> · <a href=\"${releases_url}\">All releases</a></p>"
fi

if [ "${has_cpp_docs}" = true ]; then
  echo "[generate_docs_hub.sh] Placing C++ API reference link."
  cpp_link='<li><a href="cpp/index.html">C++ API Reference</a><p>Doxygen output for the native CoolBox headers.</p></li>'
fi

if [ "${has_r_docs}" = true ]; then
  echo "[generate_docs_hub.sh] Placing R extension docs link."
  r_link="<li><a href=\"extensions/r/index.html\">R Extension Docs</a><p>pkgdown site for the R package bindings.</p>${r_release_links}</li>"
fi

if [ "${has_python_docs}" = true ]; then
  echo "[generate_docs_hub.sh] Placing Python extension docs link."
  python_link="<li><a href=\"extensions/python/index.html\">Python Extension Docs</a><p>Rendered documentation for the Python bindings.</p>${python_release_links}</li>"
fi

if [ "${has_go_docs}" = true ]; then
  echo "[generate_docs_hub.sh] Placing Go extension docs link."
  go_link="<li><a href=\"extensions/go/index.html\">Go Extension Docs</a><p>Go module overview and GitHub installation instructions for the bindings.</p>${go_release_links}</li>"
elif [ -n "${go_release_links}" ]; then
  echo "[generate_docs_hub.sh] Placing Go extension release links without docs page."
  go_link="<li><span>Go Extension Docs</span><p>Go module release packages for the published bindings.</p>${go_release_links}</li>"
fi
if [ "${has_js_docs}" = true ]; then
  echo "[generate_docs_hub.sh] Placing JavaScript extension docs link."
  js_link="<li><a href=\"extensions/javascript/index.html\">JavaScript Extension Index</a><p>Inventory of Emscripten binding modules.</p>${js_release_links}</li>"
fi
if [ "${has_c_docs}" = true ]; then
  echo "[generate_docs_hub.sh] Placing C extension docs link."
  c_link="<li><a href=\"extensions/c/index.html\">C Extension Docs</a><p>Doxygen output for the plain C bindings.</p>${c_release_links}</li>"
fi
if [ "${has_java_docs}" = true ]; then
  echo "[generate_docs_hub.sh] Placing Java extension docs link."
  java_link="<li><a href=\"extensions/java/index.html\">Java Extension Docs</a><p>Javadoc output for the plain Java bindings.</p>${java_release_links}</li>"
fi
if [ "${has_rust_docs}" = true ]; then
  echo "[generate_docs_hub.sh] Placing Rust extension docs link."
  rust_link="<li><a href=\"extensions/rust/index.html\">Rust Extension Docs</a><p>Rendered documentation for the Rust crate bindings.</p>${rust_release_links}</li>"
elif [ -n "${rust_release_links}" ]; then
  echo "[generate_docs_hub.sh] Placing Rust extension release links without docs page."
  rust_link="<li><span>Rust Extension Docs</span><p>Rust crate release packages for the published bindings.</p>${rust_release_links}</li>"
fi
if [ "${has_tutorials}" = true ]; then
  echo "[generate_docs_hub.sh] Placing tutorials link."
  tutorials_link='<li><a href="tutorials/index.html">Tutorials</a><p>Quick tutorials for using the CoolBox library.</p></li>'
fi
if [ -f "${SITE_DIR}/products/index.html" ]; then
  echo "[generate_docs_hub.sh] Placing products link."
  has_products=true
  products_link='<li><a href="products/index.html">Products</a><p>Standalone CoolBox applications built from the shared library stack.</p></li>'
fi
if [ -f "${SITE_DIR}/solutions/index.html" ]; then
  echo "[generate_docs_hub.sh] Placing solutions link."
  solutions_link='<li><a href="solutions/index.html">Solutions Catalog</a><p>Narrative guide to library areas, package responsibilities, and product relationships.</p></li>'
fi
if [ -f "${SITE_DIR}/solutions/dependencies/index.html" ]; then
  echo "[generate_docs_hub.sh] Placing dependency map link."
  dependency_link='<li><a href="solutions/dependencies/index.html">Dependency Map</a><p>Interdependency view showing how libraries flow into apps and products.</p></li>'
fi
if [ -z "${tutorials_latest_posts}" ]; then
  echo "[generate_docs_hub.sh] No recent tutorial posts found; placing placeholder message."
  tutorials_latest_posts='<li><p class="muted">No recent tutorial posts are available yet.</p></li>'
fi
if [ "${has_c_artifacts}" = true ]; then
  echo "[generate_docs_hub.sh] Placing C binding artifacts link."
  c_artifacts_link='<li><a href="artifacts/c/index.html">C Binding Artifacts</a><p>Built C static libraries and installed headers.</p></li>'
fi
if [ "${has_r_artifacts}" = true ]; then
  echo "[generate_docs_hub.sh] Placing R binding artifacts link."
  r_artifacts_link='<li><a href="artifacts/r/index.html">R Binding Artifacts</a><p>Built R source packages and release archives.</p></li>'
fi
if [ "${has_python_artifacts}" = true ]; then
  echo "[generate_docs_hub.sh] Placing Python binding artifacts link."
  python_artifacts_link='<li><a href="artifacts/python/index.html">Python Binding Artifacts</a><p>Built Python extension outputs and package artifacts.</p></li>'
fi
if [ "${has_go_artifacts}" = true ]; then
  echo "[generate_docs_hub.sh] Placing go artifact link."
  go_artifacts_link='<li><a href="artifacts/go/index.html">Go Binding Artifacts</a><p>Built Go release archives for the bindings module.</p></li>'
fi
if [ "${has_js_artifacts}" = true ]; then
  echo "[generate_docs_hub.sh] Placing js artifact link."
  js_artifacts_link='<li><a href="artifacts/javascript/index.html">JavaScript Binding Artifacts</a><p>Built Emscripten JavaScript and WASM outputs.</p></li>'
fi
if [ "${has_java_artifacts}" = true ]; then
  echo "[generate_docs_hub.sh] Placing java artifact link."
  java_artifacts_link='<li><a href="artifacts/java/index.html">Java Binding Artifacts</a><p>Built Java jars and Maven metadata.</p></li>'
fi
if [ "${has_rust_artifacts}" = true ]; then
  echo "[generate_docs_hub.sh] Placing rust artifact link."
  rust_artifacts_link='<li><a href="artifacts/rust/index.html">Rust Binding Artifacts</a><p>Built Rust crate packages and release archives.</p></li>'
fi
if [ -f "${SITE_DIR}/artifacts/products/index.html" ]; then
  echo "[generate_docs_hub.sh] Placing product artifact link."
  has_product_artifacts=true
  product_artifacts_link='<li><a href="artifacts/products/index.html">Product Artifacts</a><p>Packaged executables, runtime bundles, and copied product outputs.</p></li>'
fi


echo "[generate_docs_hub.sh] Providing index.html."
cat > "${SITE_DIR}/index.html" <<EOF
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CoolBox Documentation</title>
  <style>
    :root { color-scheme: light dark; }
    body { font-family: Arial, sans-serif; margin: 2rem auto; max-width: 960px; padding: 0 1rem; background: #f8fafc; color: #0f172a; }
    main { background: white; border-radius: 16px; padding: 2rem; box-shadow: 0 10px 30px rgba(15, 23, 42, 0.08); }
    .page-header { display: flex; align-items: baseline; justify-content: space-between; gap: 1rem; flex-wrap: wrap; margin-bottom: 0.35rem; }
    h1 { margin-top: 0; margin-bottom: 0; }
    ul { padding-left: 1.25rem; }
    li { margin: 1rem 0; }
    a { color: #2563eb; text-decoration: none; font-weight: 600; }
    a:hover { text-decoration: underline; }
    p { margin: 0.25rem 0 0; color: #475569; }
    .section-title { margin: 0 0 0.75rem; }
    .section-divider { border: 0; border-top: 2px solid #cbd5e1; margin: 2rem 0; }
    .tutorials-list li { margin: 0.85rem 0; }
    .muted { color: #64748b; font-size: 0.95rem; }
    footer { margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; font-size: 0.95rem; }
    .footer-left-mark { color: #64748b; font-weight: 700; }
  </style>
</head>
<body>
  <main>
    <div class="page-header">
      <h1>☉ CoolBox Documentation Portal</h1>
      <p class="muted">𓁿 Meandering LLC © 2026</p>
    </div>
    <p class="muted">Unified entry point for native C++ documentation and extension-specific docs.</p>
    ${latest_release_link}
    <h2 class="section-title">Documentation & Downloads</h2>
    <ul>
      ${cpp_link}
      ${solutions_link}
      ${dependency_link}
      ${r_link}
      ${python_link}
      ${go_link}
      ${js_link}
      ${c_link}
      ${java_link}
      ${rust_link}
      ${products_link}
      ${c_artifacts_link}
      ${r_artifacts_link}
      ${python_artifacts_link}
      ${go_artifacts_link}
      ${js_artifacts_link}
      ${java_artifacts_link}
      ${rust_artifacts_link}
      ${product_artifacts_link}
    </ul>
    <hr class="section-divider">
    <section>
      <h2 class="section-title">𓂀 Tutorials</h2>
      <ul class="tutorials-list">
        ${tutorials_link}
        ${tutorials_latest_posts}
      </ul>
    </section>
    <hr class="section-divider">
    <section>
      <h2 class="section-title">இ Publications</h2>
      <ul class="tutorials-list">
        $(
          PUB_DIR="${SITE_DIR}/publications"
          if [ -f "${PUB_DIR}/index.html" ]; then
            printf '<li><a href="publications/index.html">Publications index</a><p class="muted">Publication metadata and PDFs</p></li>\n'
          else
            echo '<li><p class="muted">No publications are available yet.</p></li>'
          fi
        )
      </ul>
    </section>
    <footer>
      <span class="footer-left-mark">𓎱</span>
    </footer>
  </main>
</body>
</html>
EOF

echo "[generate_docs_hub.sh] Running translate docs."
if [ -f "${ROOT_DIR}/_scripts/translate_docs.py" ]; then
  if command -v python3 >/dev/null 2>&1; then
    python3 "${ROOT_DIR}/_scripts/translate_docs.py" "${SITE_DIR}"
  fi
fi

echo "Unified docs site generated at ${SITE_DIR}"