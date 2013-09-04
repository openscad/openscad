#!/bin/sh

# Toggle the Virtual Framebuffer
# If started, stop. If stopped, start.

debug=

start()
{
  VFB_BINARY=

  if [ "`command -v Xvfb`" ]; then
    VFB_BINARY=Xvfb
  fi

  if [ "`command -v Xvnc`" ]; then
    VFB_BINARY=Xvnc
  fi

  if [ ! $VFB_BINARY ]; then
    echo "$0 Failed, cannot find Xvfb or Xvnc"
    echo "$0 Failed, cannot find Xvfb or Xvnc" > ./virtualfb.log
    exit 1
  fi

  VFB_DISPLAY=`echo | awk 'BEGIN{srand();} {printf ":%.0f", rand()*1000+100};'`
  if [ $debug ]; then
    echo debug VFB_DISPLAY $VFB_DISPLAY
    echo debug VFB_BINARY $VFB_BINARY
  fi
  $VFB_BINARY $VFB_DISPLAY -screen 0 800x600x24 > ./virtualfb1.log 2> ./virtualfb2.log &
  # on some systems $! gives us VFB_BINARY's PID, on others we have to subtract 1
  VFB_PID_MINUS0=$!
  VFB_PID_MINUS1=$(($VFB_PID_MINUS0 - 1))

  if [ "`ps cax | grep $VFB_PID_MINUS0 | grep $VFB_BINARY`" ]; then
    VFB_PID=$VFB_PID_MINUS0
  elif [ "`ps cax | grep $VFB_PID_MINUS1 | grep $VFB_BINARY`" ]; then
    VFB_PID=$VFB_PID_MINUS1
  else
    echo started $VFB_BINARY but cannot find process ID in process table
    echo please stop $VFB_BINARY manually
    VFB_PID=
    return
  fi

  echo $VFB_DISPLAY > ./virtualfb.DISPLAY
  echo $VFB_PID > ./virtualfb.PID

  echo "Started virtual fb, PID=$VFB_PID , DISPLAY=$VFB_DISPLAY"
  sleep 1
}

stop()
{
  VFB_PID=`cat ./virtualfb.PID`
  VFB_DISPLAY=`cat ./virtualfb.DISPLAY`

  echo "Stopping virtual fb, PID was $VFB_PID, DISPLAY was $VFB_DISPLAY"
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
    if [ $debug ]; then echo "found PID file"; fi
    VFB_PID=`cat ./virtualfb.PID`
    if [ ! $VFB_PID ]; then
      echo ./virtualfb.PID contained invalid PID number
      return
    fi
    if [ $debug ]; then echo "PID from file:" $VFB_PID; fi
    PS_RESULT=`ps cax | awk ' { print $1 } '`
    GREP_RESULT=`echo $PS_RESULT | grep $VFB_PID`
    if [ $debug ]; then echo "grep ps result: " $GREP_RESULT; fi
    if [ "`ps cax | awk ' { print $1 } ' | grep ^$VFB_PID\$`" ]; then
      if [ $debug ]; then echo "found pid in process table."; fi
      isrunning_result=1
    else
      if [ $debug ]; then echo "did not find pid in process table."; fi
    fi
  fi
}

if [ "`echo $* | grep debug`" ]; then
  debug=1
fi

isrunning
if [ $isrunning_result ]; then
  stop
else
  start
fi

