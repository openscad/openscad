#!/usr/bin/env bash

# Usage (in github root folder): ./scripts/github-release.sh <version>
#
# Requires release.token and releases/<version>.md

curl https://api.github.com/repos/openscad/openscad/releases -H "Authorization: token $(<release.token>)" -d "$(./scripts/makereleasejson.py $1)"
