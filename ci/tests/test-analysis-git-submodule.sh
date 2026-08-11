#!/bin/bash
# Offline test suite for ci/analysis-git-submodule.sh.
#
# Builds a throwaway superproject and two throwaway submodule repositories in a
# temporary directory, then runs the check against pins that are known to be
# valid or invalid. Remotes are plain filesystem paths, so nothing here depends
# on GitHub, on the network, or on the state of the real submodules - which is
# what makes the failing case reproducible long after the pull requests that
# motivated this check are merged.

set -u

CHECK=$(cd "$(dirname "$0")/.." && pwd)/analysis-git-submodule.sh
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

FAILURES=0

run_case() {
    local name=$1 expected=$2 root=$3
    local output rc
    output=$("$CHECK" "$root" 2>&1)
    rc=$?
    if [ "$rc" -eq "$expected" ]; then
        echo "ok   - ${name} (exit ${rc})"
    else
        echo "FAIL - ${name}: expected exit ${expected}, got ${rc}"
        echo "$output" | sed 's/^/       /'
        FAILURES=$((FAILURES + 1))
    fi
    LAST_OUTPUT=$output
}

expect_output() {
    local name=$1 pattern=$2
    if echo "$LAST_OUTPUT" | grep -q -- "$pattern"; then
        echo "ok   - ${name}"
    else
        echo "FAIL - ${name}: output does not contain '${pattern}'"
        echo "$LAST_OUTPUT" | sed 's/^/       /'
        FAILURES=$((FAILURES + 1))
    fi
}

git_quiet() { git -c user.email=ci@aff3ct -c user.name=CI "$@" >/dev/null 2>&1; }

# 'git init --initial-branch' and 'init.defaultBranch' only exist since git
# 2.28, and the CI image ships 2.25. Set the branch name the portable way.
git_init_develop() { git_quiet init "$1" && git -C "$1" symbolic-ref HEAD refs/heads/develop; }

# --- a submodule repository with a merged commit and an unmerged one ---------
# 'develop' holds MERGED. UNMERGED is committed on a side branch that is then
# deleted, so it is reachable by SHA but contained in no branch - exactly the
# situation of a commit that only exists as a pull request head.
mkdir -p "$TMP/sub-a"
git_init_develop "$TMP/sub-a"
cd "$TMP/sub-a"
echo one > file.txt
git_quiet add file.txt
git_quiet commit -m "merged commit"
MERGED=$(git rev-parse HEAD)
git_quiet checkout -b side
echo two > file.txt
git_quiet commit -am "unmerged commit"
UNMERGED=$(git rev-parse HEAD)
git_quiet checkout develop
git_quiet branch -D side

# --- the superproject --------------------------------------------------------
# The submodule is declared with a relative URL, which only resolves because
# the superproject's own origin sits next to sub-a in the same directory.
mkdir -p "$TMP/super"
git_init_develop "$TMP/super"
cd "$TMP/super"
git_quiet remote add origin "$TMP/super-origin"
echo project > README
git_quiet add README
git_quiet commit -m "initial"
git -c protocol.file.allow=always submodule add -q ../sub-a sub-a >/dev/null 2>&1
git_quiet add .gitmodules sub-a
git_quiet commit -m "add submodule"

echo "# analysis-git-submodule.sh"

# --- case 1: pin on a merged commit -----------------------------------------
cd "$TMP/super/sub-a" && git_quiet checkout "$MERGED"
run_case "merged pin passes" 0 "$TMP/super"
expect_output "merged pin reported as OK" "sub-a: OK"

# --- case 2: pin on a commit contained in no branch --------------------------
cd "$TMP/super/sub-a" && git_quiet checkout "$UNMERGED"
run_case "unmerged pin fails" 1 "$TMP/super"
expect_output "unmerged pin explains the problem" "is not in the 'develop' branch"

# --- case 3: third-party submodule is skipped, not checked -------------------
# Same unmerged pin, but declared with an absolute URL: out of our control, so
# the check must ignore it and pass.
cd "$TMP/super"
git config --file .gitmodules submodule.sub-a.url "file://$TMP/sub-a"
run_case "third-party submodule is skipped" 0 "$TMP/super"
expect_output "third-party submodule reported as skipped" "sub-a: skipped"
git config --file .gitmodules submodule.sub-a.url ../sub-a

# --- case 4: unreachable remote is not reported as an invalid pin ------------
cd "$TMP/super/sub-a"
git_quiet remote set-url origin "$TMP/does-not-exist"
run_case "unreachable remote fails" 1 "$TMP/super"
expect_output "unreachable remote is distinguished from an invalid pin" "cannot reach"

echo
if [ "$FAILURES" -eq 0 ]; then
    echo "All checks passed."
else
    echo "${FAILURES} check(s) failed."
fi
exit $((FAILURES > 0))
