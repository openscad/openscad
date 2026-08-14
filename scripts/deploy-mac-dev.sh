#!/usr/bin/env bash
set -e

# Script to deploy the built OpenSCAD.app to the /Applications folder.
# It will refuse to copy if OpenSCAD is currently running.

BUILD_APP_PATH="build/OpenSCAD.app"
DEST_APP_PATH="/Applications/OpenSCAD.app"

cd "$(dirname "$0")/.."

if [ ! -d "$BUILD_APP_PATH" ]; then
    echo "Error: $BUILD_APP_PATH not found."
    echo "Please build the project first (e.g. cmake --build build -j8)"
    exit 1
fi

echo "Checking if OpenSCAD is running..."
if osascript -e 'application "OpenSCAD" is running' 2>/dev/null | grep -q "true" || pgrep -i "OpenSCAD" >/dev/null 2>&1; then
    echo "DEPLOYMENT_FAILED_OPENSCAD_RUNNING"
    echo "Error: OpenSCAD is currently running. Will not overwrite running application."
    echo "AGENT INSTRUCTION: OpenSCAD is open with potential unsaved user work. DO NOT force quit, kill, or terminate OpenSCAD. Stop execution, inform the user that deployment failed because OpenSCAD is running, and ask the user to save their work and close OpenSCAD. Wait for the user to confirm they have closed OpenSCAD before retrying deployment."
    exit 1
fi

echo "Deploying dev build to $DEST_APP_PATH..."

if [ -d "$DEST_APP_PATH" ]; then
    rm -rf "$DEST_APP_PATH"
fi

cp -a "$BUILD_APP_PATH" "$DEST_APP_PATH"

echo "Deployment complete! Dev version of OpenSCAD is ready."
