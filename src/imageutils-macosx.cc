#include <ApplicationServices/ApplicationServices.h>
#include <CGDataConsumer.h>
#include <iostream>

CGDataConsumerCallbacks callbacks;

size_t write_bytes_to_ostream (void *info,const void *buffer,size_t count)
{
	assert( info );
	std::ostream *output = (std::ostream *)info;
	output->write( (const char *) buffer, count );
	return count;
}

CGDataConsumerRef dataconsumer CGDataConsumerCreateWithOstream(std::ostream &output)
{
	callbacks.putBytes = write_bytes_to_ostream;
	callbacks.releaseConsumer = NULL;
	CGDataConsumerRef dc = CGDataConsumerCreate ( (void *)output, &callbacks );
	return dc;
}

bool write_png(const char *filename, unsigned char *pixels, int width, int height)
{
	assert( filename );
	std::stringstream dummy;
	write_png_base( filename, dummy, pixels, width, height );
}

bool write_png(std::ostream &output, unsigned char *pixels, int width, int height)
{
	write_png_base( NULL, output, pixels, width, height );
}

bool write_png_base(const char *filename, std::ostream &output, unsigned char *pixels, int width, int height)
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

	if ( filename == NULL ) {
		CGDataConsumerRef dataconsumer = CGDataConsumerCreateWithOstream(output);
	} else {
	  CFStringRef fname = CFStringCreateWithCString(kCFAllocatorDefault, filename, kCFStringEncodingUTF8);
	  CFURLRef fileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
                                                   fname, kCFURLPOSIXPathStyle, false);
	  if (!fileURL) {
	    std::cerr << "Unable to create file URL ref.";
	    return false;
	  }

		CGDataConsumerRef dataconsumer = CGDataConsumerCreateWithURL(fileURL);
	}

  CFIndex                 fileImageIndex = 1;
  CFMutableDictionaryRef  fileDict       = NULL;
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
	if ( filename ) {
	  CFRelease(fileURL);
	  CFRelease(fname);
	}
  CFRelease(imageProps);
  CGColorSpaceRelease(colorSpace);
  CGImageRelease(imageRef);
  return true;
}



