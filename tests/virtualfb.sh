#!/bin/sh

if [ "`command -v Xvfb`" ]; then
  VFB_BINARY=Xvfb
fi

if [ "`command -v Xvnc`" ]; then
  VFB_BINARY=Xvnc
fi

if [ ! $VFB_BINARY ]; then
  echo "$0 Failed, cannot find Xvfb or Xvnc"
  exit 1
fi

DISPLAY=:98
$VFB_BINARY $DISPLAY -screen 0 800x600x24 &> virtualfblog &
echo PID=$! " "
echo DISPLAY=$DISPLAY
# trap "kill -KILL $xpid ||:" EXIT
export DISPLAY
sleep 3
