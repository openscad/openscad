#include "io/imageutils.h"

#include <iostream>
#include <cassert>
#include <cstddef>

#include <ApplicationServices/ApplicationServices.h>

static CGDataConsumerCallbacks dc_callbacks;

static size_t write_bytes_to_ostream(void *info, const void *buffer, size_t count)
{
  assert(info && buffer);
  auto *output = (std::ostream *)info;
  const size_t startpos = output->tellp();
  size_t endpos = startpos;
  try {
    output->write((const char *)buffer, count);
    endpos = output->tellp();
  } catch (const std::ios_base::failure& e) {
    std::cerr << "Error writing to ostream:" << e.what() << "\n";
  }
  return (endpos - startpos);
}

static CGDataConsumerRef CGDataConsumerCreateWithOstream(std::ostream& output)
{
  dc_callbacks.putBytes = write_bytes_to_ostream;
  dc_callbacks.releaseConsumer = nullptr;  // ostream closed by caller of write_png
  CGDataConsumerRef dc = CGDataConsumerCreate((void *)(&output), &dc_callbacks);
  return dc;
}

bool write_png(std::ostream& output, unsigned char *pixels, int width, int height)
{
  const size_t rowBytes = static_cast<size_t>(width) * 4;
  //  CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  const CGBitmapInfo bitmapInfo = kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big;  // BGRA
  const int bitsPerComponent = 8;
  CGContextRef contextRef =
    CGBitmapContextCreate(pixels, width, height, bitsPerComponent, rowBytes, colorSpace, bitmapInfo);
  if (!contextRef) {
    std::cerr << "Unable to create CGContextRef.";
    CGColorSpaceRelease(colorSpace);
    return false;
  }

  CGImageRef imageRef = CGBitmapContextCreateImage(contextRef);
  if (!imageRef) {
    std::cerr << "Unable to create CGImageRef.";
    CFRelease(contextRef);
    CGColorSpaceRelease(colorSpace);
    return false;
  }

  CGDataConsumerRef dataconsumer = CGDataConsumerCreateWithOstream(output);
  /*
     CFStringRef fname = CFStringCreateWithCString(kCFAllocatorDefault, filename, kCFStringEncodingUTF8);
     CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                   fname, kCFURLPOSIXPathStyle, false);
     if (!fileURL) {
     std::cerr << "Unable to create file URL ref.";
     return false;
     }

     CGDataConsumerRef dataconsumer = CGDataConsumerCreateWithURL(fileURL);
   */

  const CFIndex fileImageIndex = 1;
  CFMutableDictionaryRef fileDict = nullptr;
  CFStringRef fileUTType = kUTTypePNG;
  // Create an image destination opaque reference for authoring an image file
  CGImageDestinationRef imageDest =
    CGImageDestinationCreateWithDataConsumer(dataconsumer, fileUTType, fileImageIndex, fileDict);
  if (!imageDest) {
    std::cerr << "Unable to create CGImageDestinationRef.";
    CFRelease(dataconsumer);
    CGImageRelease(imageRef);
    CFRelease(contextRef);
    CGColorSpaceRelease(colorSpace);
    return false;
  }

  const CFIndex capacity = 1;
  CFMutableDictionaryRef imageProps = CFDictionaryCreateMutable(
    kCFAllocatorDefault, capacity, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CGImageDestinationAddImage(imageDest, imageRef, imageProps);
  CGImageDestinationFinalize(imageDest);

  CFRelease(imageDest);
  CFRelease(dataconsumer);
  // CFRelease(fileURL);
  // CFRelease(fname);
  CFRelease(imageProps);
  CGImageRelease(imageRef);
  CFRelease(contextRef);
  CGColorSpaceRelease(colorSpace);
  return true;
}
