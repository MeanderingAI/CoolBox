#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SITE_DIR="${1:-${ROOT_DIR}/.site}"
CPP_INPUT_DIR="${ROOT_DIR}/_libraries/include"
CPP_BACKAGES_DIR="${ROOT_DIR}/_libraries/backages"
R_PKG_DIR="${ROOT_DIR}/_libraries/r_bindings/coolboxr"
R_DOCS_DIR="${DOCS_R_DIR:-${R_PKG_DIR}/docs}"
PYTHON_DOC_MD="${ROOT_DIR}/__GENERATED_CONTENT/read_mes/python_bindings_README.md"
EMSCRIPTEN_DIR="${ROOT_DIR}/_libraries/emscripten_bindings"
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
TUTORIALS_INPUT_DIR="${DOCS_TUTORIALS_DIR:-${ROOT_DIR}/build/tutorials-site}"

rm -rf "${SITE_DIR}"
mkdir -p "${SITE_DIR}" "${SITE_DIR}/extensions" "${SITE_DIR}/artifacts"
printf '' > "${SITE_DIR}/.nojekyll"

has_cpp_docs=false
has_r_docs=false
has_python_docs=false
has_js_docs=false
has_c_docs=false
has_java_docs=false
has_tutorials=false
has_c_artifacts=false
has_python_artifacts=false
has_js_artifacts=false
has_java_artifacts=false

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
    local asset_name="coolbox-${extension_name}-${platform}-${RELEASE_TAG}.tar.gz"
    local label
    case "${platform}" in
      linux-x86_64) label='Linux' ;;
      macos-arm64) label='macOS' ;;
      windows-x86_64) label='Windows' ;;
      *) label="${platform}" ;;
    esac

    local asset_url
    asset_url="$(release_asset_url "${asset_name}")"
    if [ -n "${html}" ]; then
      html="${html} · "
    fi
    html="${html}<a href=\"${asset_url}\">${label}</a>"
  done

  if [ -n "${html}" ]; then
    printf '<p class="muted">Release assets (%s): %s</p>' "${RELEASE_TAG}" "${html}"
  fi
}

if [ -n "${PREBUILT_CPP_DOCS_DIR}" ] && [ -d "${PREBUILT_CPP_DOCS_DIR}" ]; then
  mkdir -p "${SITE_DIR}/cpp"
  cp -R "${PREBUILT_CPP_DOCS_DIR}/." "${SITE_DIR}/cpp/"
  has_cpp_docs=true
elif command -v doxygen >/dev/null 2>&1 && { [ -d "${CPP_INPUT_DIR}" ] || [ -d "${CPP_BACKAGES_DIR}" ]; }; then
  cpp_inputs=()
  if [ -d "${CPP_INPUT_DIR}" ]; then
    cpp_inputs+=("${CPP_INPUT_DIR}")
  fi
  if [ -d "${CPP_BACKAGES_DIR}" ]; then
    cpp_inputs+=("${CPP_BACKAGES_DIR}")
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
  cp -R "${TUTORIALS_INPUT_DIR}/." "${SITE_DIR}/tutorials/"
  has_tutorials=true
fi

cpp_link=''
r_link=''
python_link=''
js_link=''
c_link=''
java_link=''
tutorials_link=''
c_artifacts_link=''
python_artifacts_link=''
js_artifacts_link=''
java_artifacts_link=''
c_release_links=''
python_release_links=''
js_release_links=''
r_release_links=''
java_release_links=''
latest_release_link=''

python_release_links="$(release_links_html 'python-bindings' linux-x86_64 macos-arm64 windows-x86_64)"
js_release_links="$(release_links_html 'js-bindings' linux-x86_64 macos-arm64 windows-x86_64)"
r_release_links="$(release_links_html 'r-bindings' linux-x86_64 macos-arm64 windows-x86_64)"
c_release_links="$(release_links_html 'c-bindings' linux-x86_64)"
if [ -n "${REPOSITORY_SLUG}" ] && [ -n "${RELEASE_TAG}" ]; then
  java_release_links="<p class=\"muted\">Release assets (${RELEASE_TAG}): <a href=\"https://github.com/${REPOSITORY_SLUG}/releases/download/${RELEASE_TAG}/coolbox-java-bindings-${RELEASE_TAG}.tar.gz\">Java package</a></p>"
fi

if [ -n "${REPOSITORY_SLUG}" ]; then
  latest_release_url="$(latest_release_page_url)"
  releases_url="$(releases_page_url)"
  latest_release_link="<p class=\"muted\">GitHub releases: <a href=\"${latest_release_url}\">Latest release</a> · <a href=\"${releases_url}\">All releases</a></p>"
fi

if [ "${has_cpp_docs}" = true ]; then
  cpp_link='<li><a href="cpp/index.html">C++ API Reference</a><p>Doxygen output for the native CoolBox headers.</p></li>'
fi
if [ "${has_r_docs}" = true ]; then
  r_link="<li><a href=\"extensions/r/index.html\">R Extension Docs</a><p>pkgdown site for the R package bindings.</p>${r_release_links}</li>"
fi
if [ "${has_python_docs}" = true ]; then
  python_link="<li><a href=\"extensions/python/index.html\">Python Extension Docs</a><p>Rendered documentation for the Python bindings.</p>${python_release_links}</li>"
fi
if [ "${has_js_docs}" = true ]; then
  js_link="<li><a href=\"extensions/javascript/index.html\">JavaScript Extension Index</a><p>Inventory of Emscripten binding modules.</p>${js_release_links}</li>"
fi
if [ "${has_c_docs}" = true ]; then
  c_link="<li><a href=\"extensions/c/index.html\">C Extension Docs</a><p>Doxygen output for the plain C bindings.</p>${c_release_links}</li>"
fi
if [ "${has_java_docs}" = true ]; then
  java_link="<li><a href=\"extensions/java/index.html\">Java Extension Docs</a><p>Javadoc output for the plain Java bindings.</p>${java_release_links}</li>"
fi
if [ "${has_tutorials}" = true ]; then
  tutorials_link='<li><a href="tutorials/index.html">Tutorials</a><p>Interactive-style tutorial pages generated from .tut source files.</p></li>'
fi
if [ "${has_python_artifacts}" = true ]; then
  python_artifacts_link='<li><a href="artifacts/python/index.html">Python Binding Artifacts</a><p>Built Python extension outputs and package artifacts.</p></li>'
fi
if [ "${has_js_artifacts}" = true ]; then
  js_artifacts_link='<li><a href="artifacts/javascript/index.html">JavaScript Binding Artifacts</a><p>Built Emscripten JavaScript and WASM outputs.</p></li>'
fi
if [ "${has_c_artifacts}" = true ]; then
  c_artifacts_link='<li><a href="artifacts/c/index.html">C Binding Artifacts</a><p>Built C static libraries and installed headers.</p></li>'
fi
if [ "${has_java_artifacts}" = true ]; then
  java_artifacts_link='<li><a href="artifacts/java/index.html">Java Binding Artifacts</a><p>Built Java jars and Maven metadata.</p></li>'
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
    .brand-mark { margin-right: 0.35rem; }
    footer { margin-top: 2rem; padding-top: 1rem; border-top: 1px solid #e2e8f0; color: #64748b; font-size: 0.95rem; }
  </style>
</head>
<body>
  <main>
    <h1><span class="brand-mark">☉ 𓂀</span>CoolBox Documentation Portal</h1>
    <p class="muted">Unified entry point for native C++ documentation and extension-specific docs.</p>
    ${latest_release_link}
    <ul>
      ${cpp_link}
      ${r_link}
      ${python_link}
      ${js_link}
      ${c_link}
      ${java_link}
      ${tutorials_link}
      ${c_artifacts_link}
      ${python_artifacts_link}
      ${js_artifacts_link}
      ${java_artifacts_link}
    </ul>
    <footer>
      <p>☉ 𓂀 Meandering LLC © 2026</p>
    </footer>
  </main>
</body>
</html>
EOF

echo "Unified docs site generated at ${SITE_DIR}"
