#!/bin/bash
missing=()
if ! brew list gsl &>/dev/null; then
  missing+=(gsl)
fi
if ! command -v doxygen >/dev/null 2>&1; then
  missing+=(doxygen)
fi
echo "${missing[@]}"