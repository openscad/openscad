#!/usr/bin/env python3

from PIL import Image
import numpy as np
import sys


def Compare3x3(img1, img2):
  a1 = np.array(img1, dtype=float)
  a2 = np.array(img2, dtype=float)

  d = a1-a2
  d[np.abs(d)<3] = 0

  s = np.sign(d)
  s = s[0:-2] + s[1:-1] + s[2:]
  s = s[:,0:-2] + s[:,1:-1] + s[:,2:]
  s = np.abs(s)==9

  a = d[0:-2] * d[1:-1] * d[2:]
  a = a[:,0:-2] * a[:,1:-1] * a[:,2:]
  a = s * np.abs(a)**(1/9)

  return a

def CompareImageFiles(path1, path2):
  img1 = Image.open(path1)
  img2 = Image.open(path2)

  pixel_diffs = np.sort(Compare3x3(img1, img2).ravel())[::-1]
  pixel_cnt = pixel_diffs.shape[0]

  diff_cnt = np.sum(pixel_diffs!=0)
  perc_diff = 100.0*diff_cnt / pixel_cnt

  if perc_diff == 0:
    print('3x3 image block comparison successfully passed.')
    return True
  else:
    med_diff = np.median(pixel_diffs[:diff_cnt])
    print(f'{perc_diff:0.8f}% of 3x3 blocks differ with median block diff: {med_diff:0.2f}')
    return False

if __name__ == '__main__':
  if len(sys.argv) < 3:
    if len(sys.argv) == 2 and sys.argv[1] == '--status':
      print(f'{sys.argv[0]} library check successful')
      sys.exit(0)
    print(f'{sys.argv[0]} <image1> <image2>')
    sys.exit(-1)
  else:
    outcome = CompareImageFiles(sys.argv[1], sys.argv[2])
    sys.exit(0 if outcome else 1)

