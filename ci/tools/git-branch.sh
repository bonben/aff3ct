#!/bin/bash
# set -x

# GIT_BRANCH=$(git show -s --pretty=%d HEAD | sed -rne 's/.*origin\/(.*),.*/\1/p')
# if [ -z "$GIT_BRANCH" ]
# then
# 	GIT_BRANCH=$(git show -s --pretty=%d HEAD | sed -rne 's/.*origin\/(.*)\)$/\1/p')
# fi

# if [ -z "$GIT_BRANCH" ]
# then
# 	GIT_BRANCH=$(git branch --remote --verbose --no-abbrev --contains | sed -rne 's/^[^\/]*\/([^\ ]+).*$/\1/p')
# fi

# if [ ! -z "$GIT_BRANCH" ]
# then
# 	export GIT_BRANCH
# fi

# GITHUB_REF_NAME is set by GitHub Actions, CI_COMMIT_REF_NAME by GitLab
# CI. Support both since the two CIs run in parallel.
if [ ! -z "$GITHUB_REF_NAME" ]
then
	GIT_BRANCH=$GITHUB_REF_NAME
elif [ ! -z "$CI_COMMIT_REF_NAME" ]
then
	GIT_BRANCH=$CI_COMMIT_REF_NAME
fi

if [ -z "$GIT_BRANCH" ]
then
	echo "Git branch can't be found, exit."
	exit 1;
fi

export GIT_BRANCH
