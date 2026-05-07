#!/usr/bin/env bash
# PostToolUse hook: run clang-format on .cpp / .h / .hpp files after every write.
#
# Claude Code passes the tool event as JSON on stdin.
# We extract file_path, check the extension, run clang-format in-place.
# Exit 0 always — this hook formats, it does not block.

set -euo pipefail

INPUT=$(cat)
FILE_PATH=$(echo "$INPUT" | jq -r '.tool_input.file_path // empty')

if [[ -z "$FILE_PATH" ]]; then
    exit 0
fi

# Only act on C/C++ source and header files
if [[ "$FILE_PATH" =~ \.(cpp|cc|cxx|c|h|hpp|hxx)$ ]]; then
    if ! command -v clang-format &>/dev/null; then
        echo "clang-format not found in PATH — skipping" >&2
        exit 0
    fi

    clang-format -i "$FILE_PATH"

    # Tell Claude what happened (shown in the tool result context)
    echo "clang-format applied to $FILE_PATH"
fi

exit 0
