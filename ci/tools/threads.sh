#!/bin/bash
# set -x

if [ -z "$THREADS" ]; then
	if [ -f /proc/cpuinfo ]; then
		THREADS=$(grep -c ^processor /proc/cpuinfo)
	elif command -v sysctl >/dev/null 2>&1; then
		THREADS=$(sysctl -n hw.logicalcpu)
	else
		THREADS=1
	fi
	export THREADS
fi
