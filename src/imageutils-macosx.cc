#include <ApplicationServices/ApplicationServices.h>
#include <iostream>
#include "imageutils.h"
#include <assert.h>

CGDataConsumerCallbacks dc_callbacks;

size_t write_bytes_to_ostream (void *info,const void *buffer,size_t count)
{
	assert( info && buffer );
	std::ostream *output = (std::ostream *)info;
	size_t startpos = output->tellp();
	size_t endpos = startpos;
	try {
		output->write( (const char *)buffer, count );
		endpos = output->tellp();
	} catch (const std::ios_base::failure& e) {
		std::cerr << "Error writing to ostream:" << e.what() << "\n";
	}
	return (endpos-startpos);
}

CGDataConsumerRef CGDataConsumerCreateWithOstream(std::ostream &output)
{
	dc_callbacks.putBytes = write_bytes_to_ostream;
	dc_callbacks.releaseConsumer = nullptr; // ostream closed by caller of write_png
	CGDataConsumerRef dc = CGDataConsumerCreate ( (void *)(&output), &dc_callbacks );
	return dc;
}

bool write_png(std::ostream &output, unsigned char *pixels, int width, int height)
{
  size_t rowBytes = width * 4;
//  CGColorSpaceRef colorSpace = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGBitmapInfo bitmapInfo = kCGImageAlphaNoneSkipLast | kCGBitmapByteOrder32Big; // BGRA
  int bitsPerComponent = 8;
  CGContextRef contextRef = CGBitmapContextCreate(pixels, width, height, 
																									bitsPerComponent, rowBytes, 
                                                  colorSpace, bitmapInfo);
  if (!contextRef) {
    std::cerr << "Unable to create CGContextRef.";
    return false;
  }

  CGImageRef imageRef = CGBitmapContextCreateImage(contextRef);
  if (!imageRef) {
    std::cerr <<  "Unable to create CGImageRef.";
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

  CFIndex                 fileImageIndex = 1;
  CFMutableDictionaryRef  fileDict       = nullptr;
  CFStringRef             fileUTType     = kUTTypePNG;
  // Create an image destination opaque reference for authoring an image file
  CGImageDestinationRef imageDest = CGImageDestinationCreateWithDataConsumer(dataconsumer,
                                                                             fileUTType, 
                                                                             fileImageIndex, 
                                                                             fileDict);
  if (!imageDest) {
    std::cerr <<  "Unable to create CGImageDestinationRef.";
    return false;
  }

  CFIndex capacity = 1;
  CFMutableDictionaryRef imageProps = 
    CFDictionaryCreateMutable(kCFAllocatorDefault, 
                              capacity,
                              &kCFTypeDictionaryKeyCallBacks,
                              &kCFTypeDictionaryValueCallBacks);
  CGImageDestinationAddImage(imageDest, imageRef, imageProps);
  CGImageDestinationFinalize(imageDest);

  CFRelease(imageDest);
  CFRelease(dataconsumer);
  //CFRelease(fileURL);
  //CFRelease(fname);
  CFRelease(imageProps);
  CGColorSpaceRelease(colorSpace);
  CGImageRelease(imageRef);
  return true;
}



