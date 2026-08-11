#!/bin/bash
# Check that every aff3ct-owned submodule is pinned to a commit that is really
# merged in its own repository.
#
# The CI can fetch a commit that only exists as a pull request head, so a build
# can be perfectly green while the pinned revision lives nowhere durable. Once
# such a revision is merged into aff3ct, a force-push or a branch deletion in
# the submodule repository makes this repository impossible to clone. This
# check is what turns that silent trap into an explicit failure, before the
# merge rather than after it.
#
# Only submodules declared with a relative URL in .gitmodules are checked:
# those resolve against the aff3ct organization. Third-party submodules (e.g.
# lib/date) are skipped, we do not control their branches.
#
# Each submodule is compared against its *own* default branch, as advertised by
# its remote: streampu, conf and refs use 'develop', MIPP and cli use 'master'.
# Do not assume the branch name matches the one being built here.
#
# Usage: analysis-git-submodule.sh [superproject path]
# The optional argument exists so the test suite can point the check at a
# throwaway superproject; the CI calls it without arguments.

set -u

cd "${1:-.}" || exit 1
WD=$(pwd)
FAILED=0

while read -r KEY PATH_SUB; do
    NAME=${KEY#submodule.}
    NAME=${NAME%.path}
    URL=$(git config --file .gitmodules --get "submodule.${NAME}.url")

    case "$URL" in
        ../*) ;;
        *)
            echo "== ${PATH_SUB}: skipped (third-party submodule: ${URL})"
            continue
            ;;
    esac

    cd "${WD}/${PATH_SUB}" 2>/dev/null || {
        echo "== ${PATH_SUB}: FAILED, submodule is not initialized"
        FAILED=1
        cd "$WD" || exit 1
        continue
    }

    SHA=$(git rev-parse HEAD)
    BRANCH=$(git ls-remote --symref origin HEAD 2>/dev/null | awk '/^ref:/ {sub("refs/heads/", "", $2); print $2; exit}')

    # A network or permission problem must not be reported as an invalid pin:
    # the contributor would be blamed for something entirely on our side.
    if [ -z "$BRANCH" ]; then
        echo "== ${PATH_SUB}: FAILED, cannot reach '${URL}' to read its default branch"
        FAILED=1
        cd "$WD" || exit 1
        continue
    fi

    if ! git fetch --quiet origin "$BRANCH"; then
        echo "== ${PATH_SUB}: FAILED, cannot fetch '${BRANCH}' from '${URL}'"
        FAILED=1
        cd "$WD" || exit 1
        continue
    fi

    if git merge-base --is-ancestor "$SHA" FETCH_HEAD; then
        echo "== ${PATH_SUB}: OK (${SHA:0:10} is in '${BRANCH}')"
    else
        echo "== ${PATH_SUB}: FAILED"
        echo "   The pinned commit ${SHA} is not in the '${BRANCH}' branch of"
        echo "   the '${NAME}' submodule. It may only exist as a pull request"
        echo "   head or in a personal fork, in which case it can disappear at"
        echo "   any time and leave this repository unclonable."
        echo "   Get that commit merged in the submodule repository first, then"
        echo "   re-point the submodule at the merged revision."
        FAILED=1
    fi

    cd "$WD" || exit 1
done < <(git config --file .gitmodules --get-regexp '^submodule\..*\.path$')

exit $FAILED
