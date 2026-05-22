#!/bin/bash

set -euo pipefail

verify_only=false
if [[ "$#" -gt 0 && "$1" == "--verify-only" ]]; then
    verify_only=true
    shift
fi

default_dirs=( ./src ./groups )

if [[ "$#" -gt 0 ]]; then
    src_dirs=( "$@" )
else
    src_dirs=( "${default_dirs[@]}" )
fi

formatter="clang-format"
style="{BasedOnStyle: llvm, IndentWidth: 4, ColumnLimit: 100}"

find "${src_dirs[@]}" -type f \( -name "*.cpp" -o -name "*.h" \) -print0 | while IFS= read -r -d '' filepath; do
    >&2 echo "Processing: $filepath"
    if [[ "$verify_only" == true ]]; then
        diff -u "$filepath" <("$formatter" -style="$style" "$filepath")
    else
        "$formatter" -style="$style" -i "$filepath"
    fi
done