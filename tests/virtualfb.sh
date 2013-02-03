#!/bin/sh

# Toggle the Virtual Framebuffer
# If started, stop. If stopped, start.

start()
{
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

  VFB_DISPLAY=`echo | awk 'BEGIN{srand();} {printf ":%.0f", rand()*1000+100};'`
  $VFB_BINARY $VFB_DISPLAY -screen 0 800x600x24 &> ./virtualfb.log &
  VFB_PID=$!

  echo $VFB_DISPLAY > ./virtualfb.DISPLAY
  echo $VFB_PID > ./virtualfb.PID

  echo "Started virtual fb, PID=$VFB_PID , DISPLAY=$VFB_DISPLAY"
  sleep 1
}

stop()
{
  VFB_PID=`cat ./virtualfb.PID`
  VFB_DISPLAY=`cat ./virtualfb.DISPLAY`

  echo "Stopping virtual fb, PID=$VFB_PID, DISPLAY=$VFB_DISPLAY"
  kill $VFB_PID
  LOCKFILE=`echo "/tmp/.X"$VFB_DISPLAY"-lock"`
  if [ -e $LOCKFILE ]; then
    rm $LOCKFILE
  fi
  rm ./virtualfb.PID
  rm ./virtualfb.DISPLAY
}

isrunning()
{
  isrunning_result=
  if [ -e ./virtualfb.PID ]; then
    VFB_PID=`cat ./virtualfb.PID`
    if [ "`ps cax | awk ' { print $1 } ' | grep ^$VFB_PID\$`" ]; then
      isrunning_result=1
    fi
  fi
}

isrunning
if [ $isrunning_result ]; then
  stop
else
  start
fi

