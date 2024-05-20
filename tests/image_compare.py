#!/usr/bin/env python3

from PIL import Image
import numpy as np
import os
import sys

PIXEL_TOLERANCE = 8

def Compare3x3(img1, img2):
  '''Informally, this identifies image differences that are consistently
     different in the same direction for each possible 3x3 pixel block.  It
     zeros out all other differences.

     More precisely, this checks all 3x3 pixel blocks (with overlap, meaning
     012 123 234) for difference between the two images, and returns an array
     of the geometric mean difference for any blocks that have like-signed
     differences of magnitude 3 or greater in all 9 pixels.

     A mask image is generated highlighting in bright red (#FF2828) the
     pixels of the second image which contributed to the failed equality
     test.  For "actual.png" the mask is saved to "actual_mask.png".
  '''
  a1 = np.array(img1, dtype=float)
  a2 = np.array(img2, dtype=float)

  d = a1-a2
  # Truncate pixel-to-pixel differences less than 3 to 0.
  d[np.abs(d)<PIXEL_TOLERANCE] = 0

  # Prepare a boolean mask of all 3x3 blocks that have all 9 pixels having
  # a non-zero difference with the same sign.  i.e., the whole block is
  # more, or the whole block is less.
  s = np.sign(d)
  s = s[0:-2] + s[1:-1] + s[2:]
  s = s[:,0:-2] + s[:,1:-1] + s[:,2:]
  s = np.abs(s)==9

  # Calculate the geometric mean difference between the two images for each
  # 3x3 block, and apply the boolean sign-consistency mask s.
  a = d[0:-2] * d[1:-1] * d[2:]
  a = a[:,0:-2] * a[:,1:-1] * a[:,2:]
  a = s * np.abs(a)**(1/9)

  mask_a = np.array(img2)
  if len(s.shape)==3:
    gray_s = np.any(s, axis=2)
  else:
    mask_a = np.stack((mask_a,)*3, axis=2)
    gray_s = s
  for i in range(3):
    for j in range(3):
      mask_a[i:mask_a.shape[0]-2+i, j:mask_a.shape[1]-2+j][gray_s] = [255,40,40]

  return a, mask_a

def CompareImageFiles(path1, path2):
  img1 = Image.open(path1)
  img2 = Image.open(path2)
  split = os.path.splitext(path2)
  maskpath = f'{split[0]}_mask{split[1]}'

  # Obtains the 3x3 pixel block comparisons, flattens both image dimensions
  # and color channels into a 1D array, and sorts in descending order.
  diff_arr, mask = Compare3x3(img1, img2)
  pixel_diffs = np.sort(diff_arr.ravel())[::-1]
  pixel_cnt = pixel_diffs.shape[0]

  diff_cnt = np.sum(pixel_diffs!=0)
  perc_diff = 100.0*diff_cnt / pixel_cnt

  if perc_diff == 0:
    print('3x3 image block comparison successfully passed.')
    return True
  else:
    # Obtain the median reported geometric difference of the pixel blocks
    # that are actually differing.
    med_diff = np.median(pixel_diffs[:diff_cnt])
    print(f'{perc_diff:0.8f}% of 3x3 blocks differ with median block diff: {med_diff:0.2f}')
    Image.fromarray(mask).save(maskpath)
    print(f'Highlight mask saved to: {maskpath}')
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
    # Return 0 if images compared equivalent, or 1 if a difference was
    # identified.
    sys.exit(0 if outcome else 1)

