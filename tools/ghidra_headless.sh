#!/bin/sh
set -eu

command -v brew >/dev/null 2>&1 || {
    echo "Homebrew bulunamadı." >&2
    exit 1
}

ghidra_dir=$(brew --prefix ghidra)/libexec
java_home=$(brew --prefix openjdk@21)/libexec/openjdk.jdk/Contents/Home

test -x "$ghidra_dir/support/analyzeHeadless" || {
    echo "Ghidra headless analyzer bulunamadı." >&2
    exit 1
}

JAVA_HOME="$java_home" exec "$ghidra_dir/support/analyzeHeadless" "$@"

