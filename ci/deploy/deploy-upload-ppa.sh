#!/bin/bash
set -x

if [[ ( -z "$CI_AFF3CT_DEPLOY" ) || ( "$CI_AFF3CT_DEPLOY" != "ON" ) ]]
then
	echo "This job is disabled, try to set the CI_AFF3CT_DEPLOY environment variable to 'ON' to enable it."
	exit 1
fi

cmake --version
mkdir build
mv doc/build doc/built
cd build

if [ -z "$DISTRIBS" ]
then
	echo "Please define the 'DISTRIBS' environment variable."
	exit 1
fi


if [ -z "$CXX" ]
then
	echo "Please define the 'CXX' environment variable."
	exit 1
fi

if [ -z "$GIT_BRANCH" ]
then
	echo "Please define the 'GIT_BRANCH' environment variable."
	exit 1
fi

if [ -z "$THREADS" ]
then
	echo "The 'THREADS' environment variable is not set, default value = 1."
	THREADS=1
fi

if [ -z "$NAME" ]
then
	echo "The 'NAME' environment variable is not set, default value = 'build_deploy_upload_ppa'."
	NAME="build_deploy_upload_ppa"
fi

if [ "$GIT_BRANCH" = "develop" ]
then
	DPUT_HOST="ppa:aff3ct/aff3ct-dev"
elif [ "$GIT_BRANCH" = "master" ]
then
	DPUT_HOST="ppa:aff3ct/aff3ct-stable"
elif [ "$GIT_BRANCH" = "ppa_upload" ] || [ "$GIT_BRANCH" = "github-actions-migration" ]
then
	DPUT_HOST="ppa:aff3ct/aff3ct-dev"
else
	echo "Deploy upload ppa must be run only on master or develop branch."
	exit 1
fi

if [ -z "$LFLAGS" ]
then
	cmake .. -G"Unix Makefiles" -DCMAKE_CXX_COMPILER=$CXX \
	         -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="$CFLAGS" \
	         $CMAKE_OPT -DCMAKE_INSTALL_PREFIX="$NAME" \
	         -DAFF3CT_UPLOAD_PPA="ON" -DAFF3CT_DPUT_HOST="$DPUT_HOST" \
	         -DAFF3CT_PPA_DISTRIB="$DISTRIBS"
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
else
	cmake .. -G"Unix Makefiles" -DCMAKE_CXX_COMPILER=$CXX \
	         -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="$CFLAGS" \
	         -DCMAKE_EXE_LINKER_FLAGS="$LFLAGS" \
	         $CMAKE_OPT -DCMAKE_INSTALL_PREFIX="$NAME" \
	         -DAFF3CT_UPLOAD_PPA="ON" -DAFF3CT_DPUT_HOST="$DPUT_HOST" \
	         -DAFF3CT_PPA_DISTRIB="$DISTRIBS"
	rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
fi

if command -v apt-get >/dev/null; then
	apt-get update && apt-get install -y python3-paramiko openssh-client || true
fi

cat <<EOF > ~/.dput.cf
[ppa]
fqdn = ppa.launchpad.net
method = sftp
incoming = ~%(ppa)s/ubuntu/
login = team-aff3ct
allow_unsigned_uploads = 1
EOF

make -j $THREADS -k
rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi

for dist in $(echo $DISTRIBS | tr ';' ' '); do
	echo "Uploading distribution $dist to Launchpad PPA via SFTP..."
	n=0
	until [ "$n" -ge 5 ]
	do
		make dput_$dist && break
		n=$((n+1))
		echo "Upload busy/locked. Retrying in 30 seconds ($n/5)..."
		sleep 30
	done
	if [ "$n" -ge 5 ]; then
		echo "Failed to upload $dist after 5 attempts."
		exit 1
	fi
	echo "Waiting 60 seconds before next upload so Launchpad can process $dist..."
	sleep 60
done
