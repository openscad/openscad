#!/bin/sh


while getopts 'v' c
do
  case $c in
    v) set -x ;;
  esac
done

shift $(($OPTIND - 1))

# FIXME: Sometimes, compare fails when comparing very small images (e.g. 40 x 10 pixels).
# It's unknown why this happens..
pixelerror=`compare -fuzz 10% -metric AE $1 $2 null: 2>&1`
if [ $? -ne 0 ]; then
    echo "General error: Ouch"
    exit 1 # Compare failed to read image
else
  # Check if $pixelerror contains an integer (it sometimes outputs things like '0 @ 0,0')
  if [ $pixelerror -eq $pixelerror 2> /dev/null ]; then
    if [ $pixelerror == 0 ]; then 
      exit 0
    fi
    echo "Pixel error: $pixelerror"
  else
    echo "Pixel error: Err"
  fi
  exit 1
fi
exit 0
