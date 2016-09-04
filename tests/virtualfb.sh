#!/bin/sh

# Toggle the Virtual Framebuffer
# If started, stop. If stopped, start.

debug=1

verify_pid()
{
  verify_pid_result=0
  PID0=$1
  PID1=$2
  BIN=$3
  if [ "`ps cax | grep $PID0 | grep $BIN`" ]; then
    verify_pid_result=$PID0
  elif [ "`ps cax | grep $PID1 | grep $BIN`" ]; then
    verify_pid_result=$PID1
  fi
  return
}

guess_pid_from_ps()
{
  guess_pid_from_ps_result=0
  BIN=$1
  if [ "`ps cx | grep $BIN`" ]; then
    echo guessing PID from ps cx '|' grep $BIN
    echo `ps cx | grep $BIN`
    guess_pid_from_ps_result=`ps cx | grep $BIN | awk '{ print $1 }';`
  fi
  return
}

start()
{
  VFB_BINARY=
  VFB_PID=0

  if [ "`command -v Xvnc`" ]; then
    VFB_BINARY=Xvnc
    VFB_OPTIONS='-geometry 800x600 -depth 24'
  fi

  if [ "`command -v Xvfb`" ]; then
    VFB_BINARY=Xvfb
    VFB_OPTIONS='-screen 0 800x600x24'
  fi

  if [ ! $VFB_BINARY ]; then
    echo "$0 Failed, cannot find Xvnc or Xvfb"
    echo "$0 Failed, cannot find Xvnc or Xvfb" > ./virtualfb.log
    exit 1
  fi

  VFB_DISPLAY=`echo | awk 'BEGIN{srand();} {printf ":%.0f", rand()*1000+100};'`
  if [ $debug ]; then
    echo debug VFB_DISPLAY $VFB_DISPLAY
    echo debug VFB_BINARY $VFB_BINARY
    echo debug VFB_OPTIONS $VFB_OPTIONS
  fi
  $VFB_BINARY $VFB_DISPLAY $VFB_OPTIONS > ./virtualfb1.log 2> ./virtualfb2.log &
  # on some systems $! gives us VFB_BINARY's PID, on others we have to subtract 1
  VFB_PID_MINUS0=$!
  VFB_PID_MINUS1=$(($VFB_PID_MINUS0 - 1))

  if [ $debug ]; then
    echo debug '$!' was $VFB_PID_MINUS0
  fi

  count=3
  while [ "$count" -gt 0 ]; do
    verify_pid $VFB_PID_MINUS0 $VFB_PID_MINUS1 $VFB_BINARY
    if [ $verify_pid_result -gt 0 ]; then
      VFB_PID=$verify_pid_result
      count=0
    else
      echo "failed to find PID $VFB_PID_MINUS0 or $VFB_PID_MINUS1. Retrying"
      sleep 1
      count=`expr $count - 1`
    fi
  done

  if [ $VFB_PID -eq 0 ]; then
    guess_pid_from_ps $VFB_BINARY
    VFB_PID=$guess_pid_from_ps_result
  fi

  if [ $VFB_PID -eq 0 ]; then
    echo "started $VFB_BINARY but cannot find process ID in process table ($VFB_PID_MINUS0 or $VFB_PID_MINUS1)"
    echo "please stop $VFB_BINARY manually"
    if [ $debug ]; then
        echo `ps cax | grep $VFB_BINARY`
        echo "stdout:"
        cat ./virtualfb1.log
        echo "stderr:"
        cat ./virtualfb2.log
    fi
    VFB_PID=
    return
  fi

  echo $VFB_DISPLAY > ./virtualfb.DISPLAY
  echo $VFB_PID > ./virtualfb.PID

  echo "Started $VFB_BINARY fb, PID=$VFB_PID , DISPLAY=$VFB_DISPLAY"
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
  cat virtualfb1.log
  cat virtualfb2.log
  echo 'dump ~/.xession-errors:'
  cat ~/.xsession-errors
  echo 'end  ~/.xession-errors'
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

