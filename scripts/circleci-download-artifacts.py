#!/usr/bin/env python3
#
# Script for retrieving the artifact download URLs from CircleCI via
# the REST API. This fetches all artifacts from the last build for all
# builds:
# - Windows MXE 32bit
# - Windows MXE 64bit
# - AppImage 64bit
# - WASM
# - MacOS
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

import re
import json
import urllib3

cache_file = '.circleci-last-builds.json'
circleci_base_url = 'https://circleci.com/api/v1.1/project/github/openscad/openscad'
circleci_build_url = circleci_base_url + '/tree/master'
http = urllib3.PoolManager()

def filter(x, job):
	if x["status"] != 'success':
		return False
	if x["branch"] != 'master':
		return False
	return x["workflows"]["job_name"] == job

def latest_builds():
	response = http.request('GET', circleci_build_url, headers={ 'Accept': 'application/json' })
	data = json.loads(response.data.decode('UTF-8'))
	#print(json.dumps(data, indent=4, sort_keys=True))
	builds32 = [ x["build_num"] for x in data if filter(x, 'openscad-mxe-32bit') ]
	builds64 = [ x["build_num"] for x in data if filter(x, 'openscad-mxe-64bit') ]
	appimages64 = [ x["build_num"] for x in data if filter(x, 'openscad-appimage-64bit') ]
	wasm = [ x["build_num"] for x in data if filter(x, 'openscad-wasm') ]
	macos = [ x["build_num"] for x in data if filter(x, 'openscad-macos') ]
	list = zip(['32bit', '64bit', 'appimage-64bit', 'wasm', 'macos'], [builds32, builds64, appimages64, wasm, macos])
	builds = { key : max(val) for (key, val) in list if val }
	return builds

def latest_artifacts(builds):
	result = []
	pattern = re.compile('/OpenSCAD-')
	for build in builds:
		response = http.request('GET', f'{circleci_base_url}/{build}/artifacts', headers={'Accept': 'application/json'})
		data = json.loads(response.data.decode('UTF-8'))
		urls = [ x["url"] for x in data if re.search(pattern, x["url"]) ]
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
	for key in ['32bit', '64bit', 'appimage-64bit', 'wasm', 'macos']:
		if key not in last_builds or (key in builds and last_builds[key] != builds[key]):
			if key in builds:
				new_builds.append(builds[key])
	return new_builds

def main():
	builds = new_builds()
	artifacts = latest_artifacts(builds)
	for url in artifacts:
		print(url)

main()
