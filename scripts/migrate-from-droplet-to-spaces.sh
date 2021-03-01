#!/usr/bin/env bash

# Run this on files.openscad.or

PROFILE=openscad-files
BUCKET=openscad-files

aws --profile $PROFILE s3 cp ~/www/ s3://$BUCKET --recursive
