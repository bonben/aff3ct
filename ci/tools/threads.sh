#!/bin/bash
# set -x

# THREADS: how many threads a CI job may use. On a self-hosted machine it is a
# budget set by its administrator, in <runner>/_work/.ci-threads - the runner
# mounts _work as /__w, the only file of the host a job in a container can read -
# or in /etc/aff3ct/ci-threads. Otherwise, every CPU we see. See ci/README.md.

if [ -z "$THREADS" ]; then
	for f in "${RUNNER_WORKSPACE:+${RUNNER_WORKSPACE%/*}/.ci-threads}" /etc/aff3ct/ci-threads; do
		[ -n "$f" ] && [ -r "$f" ] || continue
		THREADS=$(sed -n 's/^[[:space:]]*\([1-9][0-9]*\).*/\1/p' "$f" | head -1)
		if [ -n "$THREADS" ]; then
			echo "THREADS=$THREADS (from $f)"
			break
		fi
	done
fi

if [ -z "$THREADS" ]; then
	if [ -f /proc/cpuinfo ]; then
		THREADS=$(grep -c ^processor /proc/cpuinfo)
	elif command -v sysctl >/dev/null 2>&1; then
		THREADS=$(sysctl -n hw.logicalcpu)
	else
		THREADS=1
	fi

	# A container sees every CPU of its host, however many runners share it.
	if [ "$RUNNER_ENVIRONMENT" = "self-hosted" ]; then
		echo "::warning::No thread budget on this machine, using its $THREADS CPUs (see ci/README.md)."
	else
		echo "THREADS=$THREADS (every visible CPU)"
	fi
fi

export THREADS
