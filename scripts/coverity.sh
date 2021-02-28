#!/bin/bash

UPLOAD_DIR=dev/coverity
UPLOAD_PROFILE=openscad-files
UPLOAD_BUCKET=openscad-files
UPLOAD_FILENAME="coverity-${TRAVIS_BUILD_NUMBER}-scm-log.txt"
COVERITY_LOG=/home/travis/build/openscad/openscad/cov-int/scm_log.txt

if [ -f "$COVERITY_LOG" ]; then
	aws --profile $UPLOAD_PROFILE s3 cp "${COVERITY_LOG}" "s3://${UPLOAD_BUCKET}/${UPLOAD_DIR}/${UPLOAD_FILENAME}"
fi
