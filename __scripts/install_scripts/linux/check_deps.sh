#!/bin/bash
missing=()
if ! ldconfig -p | grep -q gsl; then
  missing+=(gsl)
fi
if ! command -v doxygen >/dev/null 2>&1; then
  missing+=(doxygen)
fi
echo "${missing[@]}"