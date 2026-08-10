#!/bin/bash
# Initialize the submodules, with a fallback to the upstream aff3ct organization.
#
# .gitmodules uses relative URLs (../error_rate_references, ../streampu, ...),
# so submodules are resolved against the origin of the checked-out repository.
# When CI runs inside a contributor fork, that means the fork's own submodules
# are used, which lets a contributor iterate locally on e.g. their own error
# rate references. If the fork does not host a given submodule, we fall back to
# the upstream aff3ct organization.
#
# That fallback is deliberately a no-op when the checkout already belongs to
# aff3ct - which is the case for every pull request targeting aff3ct/aff3ct,
# since the workflow runs in the context of the base repository. A submodule
# bump can therefore only be validated against a commit that exists in
# aff3ct/<submodule>. This is on purpose: a commit that lives only in a fork
# would make develop unclonable for everyone else as soon as the PR is merged.

set -e

OWNER="${GITHUB_REPOSITORY_OWNER:-aff3ct}"

git config --global --add safe.directory '*'

if git submodule update --init --recursive; then
    exit 0
fi

if [ "$OWNER" != "aff3ct" ]; then
    echo "Failed to find submodules on '$OWNER'. Falling back to upstream aff3ct..."
    git config --global url."https://github.com/aff3ct/".insteadOf "https://github.com/$OWNER/"
    git submodule update --init --recursive
    exit 0
fi

cat >&2 <<'EOF'

================================================================================
Submodule initialization failed against the aff3ct organization.

If the error above reads "upload-pack: not our ref <sha>", this commit bumps a
submodule to a revision that does not exist in the corresponding aff3ct
repository - typically because that revision was only pushed to a personal
fork. Retrying will not help.

To fix it:
  1. Open a pull request on the submodule repository (e.g.
     aff3ct/error_rate_references) with your changes, and get it merged.
  2. Back in this repository, point the submodule at the merged commit:
       cd <submodule path> && git fetch origin && git checkout <merged sha>
       cd - && git add <submodule path> && git commit
  3. Push again.

A submodule revision that only exists in a fork cannot be merged here: it would
make this repository unclonable for everyone else the moment the fork changes.
================================================================================
EOF
exit 1
