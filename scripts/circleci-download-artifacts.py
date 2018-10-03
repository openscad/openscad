#!/usr/bin/env python3
#
# Script for retrieving the artifact download URLs from CircleCI via the
# REST API. This fetches all artifacts from the last build for both the
# 32bit (mxe-i686-openscad) and 64bit (mxe-x86_64-openscad) docker builds
#
# The script does not actually download the binaries, that can be done
# using wget or similar tools, e.g.:
#
#   wget $(./circleci-download-artifacts.py)
#
# A cache file in the current folder is used to remember the build
# numbers from the last run. If there are no new builds on CircleCI,
# artifacts attached to those builds will not be reported again. To
# force full download, remove the file (.circleci-last-builds.json).
#
# Currently there is no check for successful build as the latest
# setup still reports docker push failures even though the actual
# build was successful.
#

import json
import urllib3

cache_file = '.circleci-last-builds.json'
http = urllib3.PoolManager()

def latest_builds():
	response = http.request('GET', 'https://circleci.com/api/v1.1/project/github/openscad/docker-openscad', headers={ 'Accept': 'application/json' })
	data = json.loads(response.data.decode('UTF-8'))
	builds32 = [ x["build_num"] for x in data if x["build_parameters"]["CIRCLE_JOB"] == 'mxe-i686-openscad' ]
	builds64 = [ x["build_num"] for x in data if x["build_parameters"]["CIRCLE_JOB"] == 'mxe-x86_64-openscad' ]
	builds = { '32bit': max(builds32), '64bit': max(builds64) }
	return builds

def latest_artifacts(builds):
	result = []
	for build in builds:
		response = http.request('GET', 'https://circleci.com/api/v1.1/project/github/openscad/docker-openscad/{0}/artifacts'.format(build), headers={ 'Accept': 'application/json' })
		data = json.loads(response.data.decode('UTF-8'))
		urls = [ x["url"] for x in data ]
		result.extend(urls)
	return result

def new_builds():
	try:
		with open(cache_file) as infile:
			last_builds = json.load(infile)
	except:
		last_builds = {}

	builds = latest_builds()
	with open(cache_file, 'w') as outfile:
	    json.dump(builds, outfile)

	new_builds = []
	if '32bit' not in last_builds or last_builds['32bit'] != builds['32bit']:
		new_builds.append(builds['32bit'])
	if '64bit' not in last_builds or last_builds['64bit'] != builds['64bit']:
		new_builds.append(builds['64bit'])

	return new_builds

def main():
	builds = new_builds()
	artifacts = latest_artifacts(builds)
	for url in artifacts:
		print(url)

main()
