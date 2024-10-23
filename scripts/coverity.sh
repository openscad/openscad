#!/usr/bin/env bash

UPLOAD_DIR=coverity
UPLOAD_HOST=openscad@files.openscad.org
UPLOAD_FILENAME="coverity-${TRAVIS_BUILD_NUMBER}-scm-log.txt"
COVERITY_LOG=/home/travis/build/openscad/openscad/cov-int/scm_log.txt

if [ -f "$COVERITY_LOG" ]; then
	scp "${COVERITY_LOG}" "${UPLOAD_HOST}:${UPLOAD_DIR}/${UPLOAD_FILENAME}"
fi
