#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SITE_DIR="${1:-${ROOT_DIR}/.site}"
CPP_INPUT_DIR="${ROOT_DIR}/_libraries/include"
R_PKG_DIR="${ROOT_DIR}/_libraries/r_bindings/coolboxr"
PYTHON_DOC_MD="${ROOT_DIR}/__GENERATED_CONTENT/read_mes/python_bindings_README.md"
EMSCRIPTEN_DIR="${ROOT_DIR}/_libraries/emscripten_bindings"
PYTHON_BINDINGS_DIR="${ROOT_DIR}/_libraries/python_bindings"
PYTHON_BUILD_DIR="${PYTHON_BINDINGS_DIR}/build"
PYTHON_DIST_DIR="${PYTHON_BINDINGS_DIR}/dist"
JS_BUILD_DIR="${ROOT_DIR}/build-emscripten"

rm -rf "${SITE_DIR}"
mkdir -p "${SITE_DIR}" "${SITE_DIR}/extensions" "${SITE_DIR}/artifacts"
printf '' > "${SITE_DIR}/.nojekyll"

has_cpp_docs=false
has_r_docs=false
has_python_docs=false
has_js_docs=false
has_python_artifacts=false
has_js_artifacts=false

if command -v doxygen >/dev/null 2>&1 && [ -d "${CPP_INPUT_DIR}" ]; then
  cat > "${SITE_DIR}/Doxyfile" <<EOF
PROJECT_NAME = "CoolBox C++ API"
OUTPUT_DIRECTORY = ${SITE_DIR}
INPUT = ${CPP_INPUT_DIR}
RECURSIVE = YES
FILE_PATTERNS = *.h *.hpp
GENERATE_HTML = YES
HTML_OUTPUT = cpp
GENERATE_LATEX = NO
EXTRACT_ALL = YES
QUIET = YES
WARN_IF_UNDOCUMENTED = NO
WARN_IF_DOC_ERROR = YES
EOF
  doxygen "${SITE_DIR}/Doxyfile"
  rm -f "${SITE_DIR}/Doxyfile"
  has_cpp_docs=true
fi

if command -v Rscript >/dev/null 2>&1 && [ -d "${R_PKG_DIR}" ]; then
  pushd "${R_PKG_DIR}" >/dev/null
  Rscript -e "devtools::document()"
  Rscript -e "pkgdown::build_site_github_pages(new_process = FALSE, install = FALSE)"
  popd >/dev/null
  if [ -d "${R_PKG_DIR}/docs" ]; then
    mkdir -p "${SITE_DIR}/extensions/r"
    cp -R "${R_PKG_DIR}/docs/." "${SITE_DIR}/extensions/r/"
    has_r_docs=true
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

cpp_link=''
r_link=''
python_link=''
js_link=''
python_artifacts_link=''
js_artifacts_link=''

if [ "${has_cpp_docs}" = true ]; then
  cpp_link='<li><a href="cpp/index.html">C++ API Reference</a><p>Doxygen output for the native CoolBox headers.</p></li>'
fi
if [ "${has_r_docs}" = true ]; then
  r_link='<li><a href="extensions/r/index.html">R Extension Docs</a><p>pkgdown site for the R package bindings.</p></li>'
fi
if [ "${has_python_docs}" = true ]; then
  python_link='<li><a href="extensions/python/index.html">Python Extension Docs</a><p>Rendered documentation for the Python bindings.</p></li>'
fi
if [ "${has_js_docs}" = true ]; then
  js_link='<li><a href="extensions/javascript/index.html">JavaScript Extension Index</a><p>Inventory of Emscripten binding modules.</p></li>'
fi
if [ "${has_python_artifacts}" = true ]; then
  python_artifacts_link='<li><a href="artifacts/python/index.html">Python Binding Artifacts</a><p>Built Python extension outputs and package artifacts.</p></li>'
fi
if [ "${has_js_artifacts}" = true ]; then
  js_artifacts_link='<li><a href="artifacts/javascript/index.html">JavaScript Binding Artifacts</a><p>Built Emscripten JavaScript and WASM outputs.</p></li>'
fi

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
    h1 { margin-top: 0; }
    ul { padding-left: 1.25rem; }
    li { margin: 1rem 0; }
    a { color: #2563eb; text-decoration: none; font-weight: 600; }
    a:hover { text-decoration: underline; }
    p { margin: 0.25rem 0 0; color: #475569; }
    .muted { color: #64748b; font-size: 0.95rem; }
  </style>
</head>
<body>
  <main>
    <h1>CoolBox Documentation Portal</h1>
    <p class="muted">Unified entry point for native C++ documentation and extension-specific docs.</p>
    <ul>
      ${cpp_link}
      ${r_link}
      ${python_link}
      ${js_link}
      ${python_artifacts_link}
      ${js_artifacts_link}
    </ul>
  </main>
</body>
</html>
EOF

echo "Unified docs site generated at ${SITE_DIR}"
