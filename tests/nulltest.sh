#!/bin/bash
rm -f render.png cgal.png preview.png
./openscad_nogui $1 -o preview.png
./openscad_nogui $1 -o render.png --render
./openscad_nogui $1 -o cgal.png --render=cgal

for type in preview render cgal; do
  echo -n "$type: "
  if [ -f $type.png ]; then
    if [ -s $type.png ]; then
      diff $type.png empty.png > /dev/null
      if [ $? -eq 0 ]; then
        echo "Empty"
      else
        echo "X"
      fi
    else
      echo "Zero"
    fi
  else
    echo "None"
  fi
done
