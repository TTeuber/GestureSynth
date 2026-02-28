#!/bin/bash
set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <branch-name>"
    exit 1
fi

BRANCH="$1"
REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
WORKTREE_DIR="$REPO_ROOT/worktrees/$BRANCH"

if [ -d "$WORKTREE_DIR" ]; then
    echo "Error: worktree already exists at $WORKTREE_DIR"
    exit 1
fi

echo "Creating worktree at $WORKTREE_DIR..."
git worktree add -b "$BRANCH" "$WORKTREE_DIR"

echo "Initializing submodules..."
cd "$WORKTREE_DIR"
git submodule update --init --recursive --reference "$REPO_ROOT"

# Fix submodules that may have an empty index after worktree creation
echo "Checking submodule working trees..."
git submodule foreach --recursive '
    if [ -z "$(ls -A . 2>/dev/null | grep -v "^\.git$")" ]; then
        echo "Fixing empty working tree in $sm_path..."
        git reset HEAD -- .
        git checkout -- .
    fi
'

echo "Done. Worktree ready at $WORKTREE_DIR"