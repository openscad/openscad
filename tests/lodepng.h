/*
LodePNG version 20110908

Copyright (c) 2005-2011 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#ifndef LODEPNG_H
#define LODEPNG_H

#include <string.h> /*for size_t*/

#ifdef __cplusplus
#include <vector>
#include <string>
#endif /*__cplusplus*/

/* ////////////////////////////////////////////////////////////////////////// */
/* Code Sections                                                              */
/* ////////////////////////////////////////////////////////////////////////// */

/*
The following #defines are used to create code sections. They can be disabled
to disable code sections, which can give faster compile time and smaller binary.
*/

/*deflate&zlib encoder and deflate&zlib decoder.
If this is disabled, you need to implement the dummy LodePNG_zlib_compress and
LodePNG_zlib_decompress functions in the #else belonging to this #define in the
source file.*/
#define LODEPNG_COMPILE_ZLIB
/*png encoder and png decoder*/
#define LODEPNG_COMPILE_PNG
/*deflate&zlib decoder and png decoder*/
#define LODEPNG_COMPILE_DECODER
/*deflate&zlib encoder and png encoder*/
#define LODEPNG_COMPILE_ENCODER
/*the optional built in harddisk file loading and saving functions*/
#define LODEPNG_COMPILE_DISK
/*any code or struct datamember related to chunks other than IHDR, IDAT, PLTE, tRNS, IEND*/
#define LODEPNG_COMPILE_ANCILLARY_CHUNKS
/*handling of unknown chunks*/
#define LODEPNG_COMPILE_UNKNOWN_CHUNKS
/*ability to convert error numerical codes to English text string*/
#define LODEPNG_COMPILE_ERROR_TEXT

/* ////////////////////////////////////////////////////////////////////////// */
/* Simple Functions                                                           */
/* ////////////////////////////////////////////////////////////////////////// */

/*
This are the simple C and C++ functions, which cover basic usage.
Further on in the header file are the more advanced functions allowing custom behaviour.
*/

#ifdef LODEPNG_COMPILE_PNG

/*constants for the PNG color types. "LCT" = "LodePNG Color Type"*/
#define LCT_GREY 0 /*PNG color type greyscale*/
#define LCT_RGB 2 /*PNG color type RGB*/
#define LCT_PALETTE 3 /*PNG color type palette*/
#define LCT_GREY_ALPHA 4 /*PNG color type greyscale with alpha*/
#define LCT_RGBA 6 /*PNG color type RGB with alpha*/

#ifdef LODEPNG_COMPILE_DECODER
/*
Converts PNG data in memory to raw pixel data.
out: Output parameter. Pointer to buffer that will contain the raw pixel data.
     Its size is w * h * (bytes per pixel), bytes per pixel depends on colorType and bitDepth.
     Must be freed after usage with free(*out).
w: Output parameter. Pointer to width of pixel data.
h: Output parameter. Pointer to height of pixel data.
in: Memory buffer with the PNG file.
insize: size of the in buffer.
colorType: the desired color type for the raw output image. See explanation on PNG color types.
bitDepth: the desired bit depth for the raw output image. See explanation on PNG color types.
Return value: LodePNG error code (0 means no error).
*/
unsigned LodePNG_decode(unsigned char** out, unsigned* w, unsigned* h,
                        const unsigned char* in, size_t insize, unsigned colorType, unsigned bitDepth);

/*Same as LodePNG_decode, but uses colorType = 6 and bitDepth = 8 by default (32-bit RGBA)*/
unsigned LodePNG_decode32(unsigned char** out, unsigned* w, unsigned* h,
                          const unsigned char* in, size_t insize);

/*Same as LodePNG_decode, but uses colorType = 2 and bitDepth = 8 by default (24-bit RGB)*/
unsigned LodePNG_decode24(unsigned char** out, unsigned* w, unsigned* h,
                          const unsigned char* in, size_t insize);

#ifdef LODEPNG_COMPILE_DISK
/*
Load PNG from disk, from file with given name.
out: Output parameter. Pointer to buffer that will contain the raw pixel data.
     Its size is w * h * (bytes per pixel), bytes per pixel depends on colorType and bitDepth.
     Must be freed after usage with free(*out).
w: Output parameter. Pointer to width of pixel data.
h: Output parameter. Pointer to height of pixel data.
filename: Path on disk of the PNG file.
colorType: the desired color type for the raw output image. See explanation on PNG color types.
bitDepth: the desired bit depth for the raw output image. See explanation on PNG color types.
Return value: LodePNG error code (0 means no error).
*/
unsigned LodePNG_decode_file(unsigned char** out, unsigned* w, unsigned* h,
                             const char* filename, unsigned colorType, unsigned bitDepth);

/*Same as LodePNG_decode_file, but uses colorType = 6 and bitDepth = 8 by default (32-bit RGBA)*/
unsigned LodePNG_decode32_file(unsigned char** out, unsigned* w, unsigned* h, const char* filename);

/*Same as LodePNG_decode_file, but uses colorType = 2 and bitDepth = 8 by default (24-bit RGB)*/
unsigned LodePNG_decode24_file(unsigned char** out, unsigned* w, unsigned* h, const char* filename);


#endif /*LODEPNG_COMPILE_DISK*/
#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER
/*
Converts raw pixel data into a PNG image in memory. The colorType and bitDepth
  of the output PNG image cannot be chosen, they are automatically determined
  by the colorType, bitDepth and content of the input pixel data.
out: Output parameter. Pointer to buffer that will contain the raw pixel data.
     Must be freed after usage with free(*out).
outsize: Output parameter. Pointer to the size in bytes of the out buffer.
image: The raw pixel data to encode. The size of this buffer should be
       w * h * (bytes per pixel), bytes per pixel depends on colorType and bitDepth.
w: width of the raw pixel data in pixels.
h: height of the raw pixel data in pixels.
colorType: the color type of the raw input image. See explanation on PNG color types.
bitDepth: the bit depth of the raw input image. See explanation on PNG color types.
Return value: LodePNG error code (0 means no error).
*/
unsigned LodePNG_encode(unsigned char** out, size_t* outsize,
                        const unsigned char* image, unsigned w, unsigned h, unsigned colorType, unsigned bitDepth);

/*Same as LodePNG_encode, but uses colorType = 6 and bitDepth = 8 by default (32-bit RGBA).*/
unsigned LodePNG_encode32(unsigned char** out, size_t* outsize, const unsigned char* image, unsigned w, unsigned h);

/*Same as LodePNG_encode, but uses colorType = 2 and bitDepth = 8 by default (24-bit RGB).*/
unsigned LodePNG_encode24(unsigned char** out, size_t* outsize, const unsigned char* image, unsigned w, unsigned h);

#ifdef LODEPNG_COMPILE_DISK

/*
Converts raw pixel data into a PNG file on disk. Same as LodePNG_encode, but
outputs to disk instead of memory buffer.
filename: path to file on disk to write the PNG image to.
image: The raw pixel data to encode. The size of this buffer should be
       w * h * (bytes per pixel), bytes per pixel depends on colorType and bitDepth.
w: width of the raw pixel data in pixels.
h: height of the raw pixel data in pixels.
colorType: the color type of the raw input image. See explanation on PNG color types.
bitDepth: the bit depth of the raw input image. See explanation on PNG color types.
Return value: LodePNG error code (0 means no error).
*/
unsigned LodePNG_encode_file(const char* filename, const unsigned char* image,
                             unsigned w, unsigned h, unsigned colorType, unsigned bitDepth);

/*Same as LodePNG_encode_file, but uses colorType = 6 and bitDepth = 8 by default (32-bit RGBS).*/
unsigned LodePNG_encode32_file(const char* filename, const unsigned char* image, unsigned w, unsigned h);

/*Same as LodePNG_encode_file, but uses colorType = 2 and bitDepth = 8 by default (24-bit RGB).*/
unsigned LodePNG_encode24_file(const char* filename, const unsigned char* image, unsigned w, unsigned h);
#endif /*LODEPNG_COMPILE_DISK*/
#endif /*LODEPNG_COMPILE_ENCODER*/

#ifdef __cplusplus
namespace LodePNG
{

#ifdef LODEPNG_COMPILE_DECODER
  /*
  Converts PNG data in memory to raw pixel data.
  out: Output parameter, std::vector containing the raw pixel data. Its size
    will be w * h * (bytes per pixel), where bytes per pixel is 4 if the default
    colorType=6 and bitDepth=8 is used. The pixels are 32-bit RGBA bit in that case.
  w: Output parameter, width of the image in pixels.
  h: Output parameter, height of the image in pixels.
  in: Memory buffer with the PNG file.
  insize: size of the in buffer.
  colorType: the desired color type for the raw output image. See explanation on PNG color types.
  bitDepth: the desired bit depth for the raw output image. See explanation on PNG color types.
  Return value: LodePNG error code (0 means no error).
  */
  unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                  const unsigned char* in, size_t insize, unsigned colorType = LCT_RGBA, unsigned bitDepth = 8);

  /*
  Same as the decode function that takes a unsigned char buffer, but instead of giving
  a pointer and a size, this takes the input buffer as an std::vector.
  */
  unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                  const std::vector<unsigned char>& in, unsigned colorType = LCT_RGBA, unsigned bitDepth = 8);
#ifdef LODEPNG_COMPILE_DISK
  /*
  Converts PNG file from disk to raw pixel data in memory.
  out: Output parameter, std::vector containing the raw pixel data. Its size
    will be w * h * (bytes per pixel), where bytes per pixel is 4 if the default
    colorType=6 and bitDepth=8 is used. The pixels are 32-bit RGBA bit in that case.
  w: Output parameter, width of the image in pixels.
  h: Output parameter, height of the image in pixels.
  filename: Path to PNG file on disk.
  colorType: the desired color type for the raw output image. See explanation on PNG color types.
  bitDepth: the desired bit depth for the raw output image. See explanation on PNG color types.
  Return value: LodePNG error code (0 means no error).
  */
  unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                  const std::string& filename, unsigned colorType = LCT_RGBA, unsigned bitDepth = 8);
#endif //LODEPNG_COMPILE_DISK
#endif //LODEPNG_COMPILE_DECODER

#ifdef LODEPNG_COMPILE_ENCODER
  /*
  Converts 32-bit RGBA raw pixel data into a PNG image in memory.
  out: Output parameter, std::vector containing the PNG image data.
  in: Memory buffer with raw pixel data. The size of this buffer should be
       w * h * (bytes per pixel), With the default colorType=6 and bitDepth=8, bytes
       per pixel should be 4 and the data is a 32-bit RGBA pixel buffer.
  w: Width of the image in pixels.
  h: Height of the image in pixels.
  colorType: the color type of the raw input image. See explanation on PNG color types.
  bitDepth: the bit depth of the raw input image. See explanation on PNG color types.
  Return value: LodePNG error code (0 means no error).
  */
  unsigned encode(std::vector<unsigned char>& out, const unsigned char* in,
                  unsigned w, unsigned h, unsigned colorType = LCT_RGBA, unsigned bitDepth = 8);

  /*
  Same as the encode function that takes a unsigned char buffer, but instead of giving
  a pointer and a size, this takes the input buffer as an std::vector.
  */
  unsigned encode(std::vector<unsigned char>& out, const std::vector<unsigned char>& in,
                  unsigned w, unsigned h, unsigned colorType = LCT_RGBA, unsigned bitDepth = 8);
#ifdef LODEPNG_COMPILE_DISK
  /*
  Converts 32-bit RGBA raw pixel data into a PNG file on disk.
  filename: Path to the file to write the PNG image to.
  in: Memory buffer with raw pixel data. The size of this buffer should be
       w * h * (bytes per pixel), With the default colorType=6 and bitDepth=8, bytes
       per pixel should be 4 and the data is a 32-bit RGBA pixel buffer.
  w: Width of the image in pixels.
  h: Height of the image in pixels.
  colorType: the color type of the raw input image. See explanation on PNG color types.
  bitDepth: the bit depth of the raw input image. See explanation on PNG color types.
  Return value: LodePNG error code (0 means no error).
  */
  unsigned encode(const std::string& filename, const unsigned char* in,
                  unsigned w, unsigned h, unsigned colorType = LCT_RGBA, unsigned bitDepth = 8);

  /*
  Same as the encode function that takes a unsigned char buffer, but instead of giving
  a pointer and a size, this takes the input buffer as an std::vector.
  */
  unsigned encode(const std::string& filename, const std::vector<unsigned char>& in,
                  unsigned w, unsigned h, unsigned colorType = LCT_RGBA, unsigned bitDepth = 8);
#endif //LODEPNG_COMPILE_DISK
#endif //LODEPNG_COMPILE_ENCODER
} //namespace LodePNG
#endif /*__cplusplus*/
#endif /*LODEPNG_COMPILE_PNG*/

#ifdef LODEPNG_COMPILE_ERROR_TEXT
/*
Returns a textual description of the error code, in English. The
numerical value of the code itself is not included in this description.
*/
const char* LodePNG_error_text(unsigned code);
#endif /*LODEPNG_COMPILE_ERROR_TEXT*/

/* ////////////////////////////////////////////////////////////////////////// */
/* Inflate & Deflate Setting Structs                                          */
/* ////////////////////////////////////////////////////////////////////////// */

/*
These structs contain settings for the decompression and compression of the
PNG files. Typically you won't need these directly.
*/

#ifdef LODEPNG_COMPILE_DECODER
typedef struct LodePNG_DecompressSettings
{
  unsigned ignoreAdler32; /*if 1, continue and don't give an error message if the Adler32 checksum is corrupted*/
} LodePNG_DecompressSettings;

extern const LodePNG_DecompressSettings LodePNG_defaultDecompressSettings;
void LodePNG_DecompressSettings_init(LodePNG_DecompressSettings* settings);
#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER
/*
Compression settings. Tweaking these settings tweaks the balance between
speed and compression ratio.
*/
typedef struct LodePNG_CompressSettings /*deflate = compress*/
{
  /*LZ77 related settings*/
  unsigned btype; /*the block type for LZ (0, 1, 2 or 3, see zlib standard). Should be 2 for proper compression.*/
  unsigned useLZ77; /*whether or not to use LZ77. Should be 1 for proper compression.*/
  unsigned windowSize; /*the maximum is 32768, higher gives more compression but is slower. Typical value: 2048.*/
} LodePNG_CompressSettings;

extern const LodePNG_CompressSettings LodePNG_defaultCompressSettings;
void LodePNG_CompressSettings_init(LodePNG_CompressSettings* settings);
#endif /*LODEPNG_COMPILE_ENCODER*/

#ifdef LODEPNG_COMPILE_PNG
/* ////////////////////////////////////////////////////////////////////////// */
/* PNG and Raw Image Information Structs                                      */
/* ////////////////////////////////////////////////////////////////////////// */

/*
Info about the color type of an image.
The same LodePNG_InfoColor struct is used for both the PNG and raw image type,
even though they are two totally different things.
*/
typedef struct LodePNG_InfoColor
{
  /*header (IHDR)*/
  unsigned colorType; /*color type, see PNG standard or documentation further in this header file*/
  unsigned bitDepth;  /*bits per sample, see PNG standard or documentation further in this header file*/

  /*
  palette (PLTE and tRNS)

  This is a dynamically allocated unsigned char array with the colors of the palette, including alpha.
  The value palettesize indicates the amount of colors in the palette.
  The allocated size of the buffer is 4 * palettesize bytes, in order RGBARGBARGBA...

  When encoding a PNG, to store your colors in the palette of the LodePNG_InfoRaw, first use
  LodePNG_InfoColor_clearPalette, then for each color use LodePNG_InfoColor_addPalette.

  If you encode an image without alpha with palette, don't forget to put value 255 in each A byte of the palette.

  When decoding, by default you can ignore this palette, since LodePNG already
  fills the palette colors in the pixels of the raw RGBA output.
  */
  unsigned char* palette; /*palette in RGBARGBA... order*/
  size_t palettesize; /*palette size in number of colors (amount of bytes is 4 * palettesize)*/

  /*
  transparent color key (tRNS)
  This color uses the same bit depth as the bitDepth value in this struct, which can be 1-bit to 16-bit.
  For greyscale PNGs, r, g and b will all 3 be set to the same.

  When decoding, by default you can ignore this information, since LodePNG sets
  pixels with this key to transparent already in the raw RGBA output.
  */
  unsigned key_defined; /*is a transparent color key given? 0 = false, 1 = true*/
  unsigned key_r;       /*red/greyscale component of color key*/
  unsigned key_g;       /*green component of color key*/
  unsigned key_b;       /*blue component of color key*/
} LodePNG_InfoColor;

/*init, cleanup and copy functions to use with this struct*/
void LodePNG_InfoColor_init(LodePNG_InfoColor* info);
void LodePNG_InfoColor_cleanup(LodePNG_InfoColor* info);
/*return value is error code (0 means no error)*/
unsigned LodePNG_InfoColor_copy(LodePNG_InfoColor* dest, const LodePNG_InfoColor* source);

void LodePNG_InfoColor_clearPalette(LodePNG_InfoColor* info);
/*add 1 color to the palette*/
unsigned LodePNG_InfoColor_addPalette(LodePNG_InfoColor* info, unsigned char r,
                                      unsigned char g, unsigned char b, unsigned char a);

/*additional color info*/

/*get the total amount of bits per pixel, based on colorType and bitDepth in the struct*/
unsigned LodePNG_InfoColor_getBpp(const LodePNG_InfoColor* info);
/*get the amount of color channels used, based on colorType in the struct.
If a palette is used, it counts as 1 channel.*/
unsigned LodePNG_InfoColor_getChannels(const LodePNG_InfoColor* info);
/*is it a greyscale type? (only colorType 0 or 4)*/
unsigned LodePNG_InfoColor_isGreyscaleType(const LodePNG_InfoColor* info);
/*has it got an alpha channel? (only colorType 2 or 6)*/
unsigned LodePNG_InfoColor_isAlphaType(const LodePNG_InfoColor* info);
/*has it got a palette? (only colorType 3)*/
unsigned LodePNG_InfoColor_isPaletteType(const LodePNG_InfoColor* info);
/*only returns true if there is a palette and there is a value in the palette with alpha < 255.
Loops through the palette to check this.*/
unsigned LodePNG_InfoColor_hasPaletteAlpha(const LodePNG_InfoColor* info);

/*
Check if the given color info indicates the possibility of having non-opaque pixels in the PNG image.
Returns true if the image can have translucent or invisible pixels (it still be opaque if it doesn't use such pixels).
Returns false if the image can only have opaque pixels.
In detail, it returns true only if it's a color type with alpha, or has a palette with non-opaque values,
or if "key_defined" is true.
*/
unsigned LodePNG_InfoColor_canHaveAlpha(const LodePNG_InfoColor* info);

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
/*
The information of a Time chunk in PNG.
To make the encoder add a time chunk, set time_defined to 1 and fill in
the correct values in all the time parameters. LodePNG will not fill the current
time in these values itself, all it does is copy them over into the chunk bytes.
*/
typedef struct LodePNG_Time
{
  unsigned      year;    /*2 bytes used (0-65535)*/
  unsigned char month;   /*1-12*/
  unsigned char day;     /*1-31*/
  unsigned char hour;    /*0-23*/
  unsigned char minute;  /*0-59*/
  unsigned char second;  /*0-60 (to allow for leap seconds)*/
} LodePNG_Time;

/*
Info about text chunks in a PNG file. The arrays can contain multiple keys
and strings. The amount of keys and strings is the same. The amount of strings
ends when the pointer to the string is a null pointer.

They keyword of text chunks gives a short description what the actual text
represents. There are a few standard standard keywords recognised
by many programs: Title, Author, Description, Copyright, Creation Time,
Software, Disclaimer, Warning, Source, Comment. It's allowed to use other keys.

A keyword is minimum 1 character and maximum 79 characters long. It's
discouraged to use a single line length longer than 79 characters for texts.
*/
typedef struct LodePNG_Text /*non-international text*/
{
  /*Don't allocate these text buffers yourself. Use the init/cleanup functions
  correctly and use LodePNG_Text_add and LodePNG_Text_clear.*/
  size_t num; /*the amount of texts in these char** buffers (there may be more texts in itext)*/
  char** keys; /*the keyword of a text chunk (e.g. "Comment")*/
  char** strings; /*the actual text*/
} LodePNG_Text;

/*init, cleanup and copy functions to use with this struct*/
void LodePNG_Text_init(LodePNG_Text* text);
void LodePNG_Text_cleanup(LodePNG_Text* text);
/*return value is error code (0 means no error)*/
unsigned LodePNG_Text_copy(LodePNG_Text* dest, const LodePNG_Text* source);

/*Use these functions instead of allocating the char**s manually*/
void LodePNG_Text_clear(LodePNG_Text* text); /*use this to clear the texts again after you filled them in*/
unsigned LodePNG_Text_add(LodePNG_Text* text, const char* key, const char* str); /*push back both texts at once*/

/*
Info about international text chunks in a PNG file. The arrays can contain multiple keys
and strings. The amount of keys, lengtags, transkeys and strings is the same.
The amount of strings ends when the pointer to the string is a null pointer.

A keyword is minimum 1 character and maximum 79 characters long. It's
discouraged to use a single line length longer than 79 characters for texts.
*/
typedef struct LodePNG_IText /*international text*/
{
  /*Don't allocate these text buffers yourself. Use the init/cleanup functions
  correctly and use LodePNG_IText_add and LodePNG_IText_clear.*/
  
  /*the amount of international texts in this PNG*/
  size_t num;
  /*the English keyword of the text chunk (e.g. "Comment")*/
  char** keys;
  /*the language tag for this text's international language, ISO/IEC 646 string, e.g. ISO 639 language tag*/
  char** langtags;
  /*keyword translated to the international language - UTF-8 string*/
  char** transkeys;
  /*the actual international text - UTF-8 string*/
  char** strings;
} LodePNG_IText;

/*init, cleanup and copy functions to use with this struct*/
void LodePNG_IText_init(LodePNG_IText* text);
void LodePNG_IText_cleanup(LodePNG_IText* text);
/*return value is error code (0 means no error)*/
unsigned LodePNG_IText_copy(LodePNG_IText* dest, const LodePNG_IText* source);

/*Use these functions instead of allocating the char**s manually*/
void LodePNG_IText_clear(LodePNG_IText* text); /*use this to clear the itexts again after you filled them in*/
unsigned LodePNG_IText_add(LodePNG_IText* text, const char* key, const char* langtag,
                           const char* transkey, const char* str); /*push back the 4 texts of 1 chunk at once*/
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
/*
Unknown chunks read from the PNG, or extra chunks the user wants to have added
in the encoded PNG.
*/
typedef struct LodePNG_UnknownChunks
{
  /*There are 3 buffers, one for each position in the PNG where unknown chunks can appear
    each buffer contains all unknown chunks for that position consecutively
    The 3 buffers are the unknown chunks between certain critical chunks:
    0: IHDR-PLTE, 1: PLTE-IDAT, 2: IDAT-IEND

    Do not allocate or traverse this data yourself. Use the chunk traversing functions declared
    later, such as LodePNG_chunk_next and LodePNG_append_chunk, to read/write this struct.
    */
  unsigned char* data[3];
  size_t datasize[3]; /*size in bytes of the unknown chunks, given for protection*/

} LodePNG_UnknownChunks;

/*init, cleanup and copy functions to use with this struct*/
void LodePNG_UnknownChunks_init(LodePNG_UnknownChunks* chunks);
void LodePNG_UnknownChunks_cleanup(LodePNG_UnknownChunks* chunks);
/*return value is error code (0 means no error)*/
unsigned LodePNG_UnknownChunks_copy(LodePNG_UnknownChunks* dest, const LodePNG_UnknownChunks* src);
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/

/*
Information about the PNG image, except pixels and sometimes except width and height.
*/
typedef struct LodePNG_InfoPng
{
  /*header (IHDR), palette (PLTE) and transparency (tRNS)*/

  /*
  Note: width and height are only used as information of a decoded PNG image. When encoding one, you don't have
  to specify width and height in an LodePNG_Info struct, but you give them as parameters of the encode function.
  The rest of the LodePNG_Info struct IS used by the encoder though!
  */
  unsigned width;             /*width of the image in pixels (ignored by encoder, but filled in by decoder)*/
  unsigned height;            /*height of the image in pixels (ignored by encoder, but filled in by decoder)*/
  unsigned compressionMethod; /*compression method of the original file. Always 0.*/
  unsigned filterMethod;      /*filter method of the original file*/
  unsigned interlaceMethod;   /*interlace method of the original file*/
  LodePNG_InfoColor color;    /*color type and bits, palette and transparency of the PNG file*/

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  /*
  suggested background color (bKGD)
  This color uses the same bit depth as the bitDepth value in this struct, which can be 1-bit to 16-bit.

  For greyscale PNGs, r, g and b will all 3 be set to the same. When encoding
  the encoder writes the red one. For palette PNGs: When decoding, the RGB value
  will be stored, not a palette index. But when encoding, specify the index of
  the palette in background_r, the other two are then ignored.

  The decoder does not use this background color to edit the color of pixels.
  */
  unsigned background_defined; /*is a suggested background color given?*/
  unsigned background_r;       /*red component of suggested background color*/
  unsigned background_g;       /*green component of suggested background color*/
  unsigned background_b;       /*blue component of suggested background color*/

  /*non-international text chunks (tEXt and zTXt)*/
  LodePNG_Text text;

  /*international text chunks (iTXt)*/
  LodePNG_IText itext;

  /*time chunk (tIME)*/
  unsigned char time_defined; /*if 0, no tIME chunk was or will be generated in the PNG image*/
  LodePNG_Time time;

  /*phys chunk (pHYs)*/
  unsigned phys_defined; /*if 0, there is no pHYs chunk and the values below are undefined, if 1 else there is one*/
  unsigned phys_x; /*pixels per unit in x direction*/
  unsigned phys_y; /*pixels per unit in y direction*/
  unsigned char phys_unit; /*may be 0 (unknown unit) or 1 (metre)*/
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
  /*unknown chunks*/
  LodePNG_UnknownChunks unknown_chunks;
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
} LodePNG_InfoPng;

/*init, cleanup and copy functions to use with this struct*/
void LodePNG_InfoPng_init(LodePNG_InfoPng* info);
void LodePNG_InfoPng_cleanup(LodePNG_InfoPng* info);
/*return value is error code (0 means no error)*/
unsigned LodePNG_InfoPng_copy(LodePNG_InfoPng* dest, const LodePNG_InfoPng* source);

/*
Contains user-chosen information about the raw image data, which is independent of the PNG image
With raw images, I mean the image data in the form of the simple raw buffer to which the
compressed PNG data is decoded, or from which a PNG image can be encoded.
*/
typedef struct LodePNG_InfoRaw
{
  LodePNG_InfoColor color; /*color info of the raw image, note that the same struct as for PNG data is used.*/
} LodePNG_InfoRaw;

/*init, cleanup and copy functions to use with this struct*/
void LodePNG_InfoRaw_init(LodePNG_InfoRaw* info);
void LodePNG_InfoRaw_cleanup(LodePNG_InfoRaw* info);
/*return value is error code (0 means no error)*/
unsigned LodePNG_InfoRaw_copy(LodePNG_InfoRaw* dest, const LodePNG_InfoRaw* source);

/*
Converts raw buffer from one color type to another color type, based on
LodePNG_InfoColor structs to describe the input and output color type.
See the reference manual at the end of this header file to see which color conversions are supported.
return value = LodePNG error code (0 if all went ok, an error if the conversion isn't supported)
The out buffer must have size (w * h * bpp + 7) / 8, where bpp is the bits per pixel
of the output color type (LodePNG_InfoColor_getBpp)
*/
unsigned LodePNG_convert(unsigned char* out, const unsigned char* in, LodePNG_InfoColor* infoOut,
                         LodePNG_InfoColor* infoIn, unsigned w, unsigned h);

#ifdef LODEPNG_COMPILE_DECODER

/* ////////////////////////////////////////////////////////////////////////// */
/* LodePNG Decoder                                                            */
/* ////////////////////////////////////////////////////////////////////////// */

/*
Settings for the decoder. This contains settings for the PNG and the Zlib
decoder, but not the Info settings from the Info structs.
*/
typedef struct LodePNG_DecodeSettings
{
  LodePNG_DecompressSettings zlibsettings; /*in here is the setting to ignore Adler32 checksums*/

  unsigned ignoreCrc; /*ignore CRC checksums*/
  unsigned color_convert; /*whether to convert the PNG to the color type you want. Default: yes*/

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  unsigned readTextChunks; /*if false but rememberUnknownChunks is true, they're stored in the unknown chunks*/
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
  /*store all bytes from unknown chunks in the InfoPng (off by default, useful for a png editor)*/
  unsigned rememberUnknownChunks;
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
} LodePNG_DecodeSettings;

void LodePNG_DecodeSettings_init(LodePNG_DecodeSettings* settings);

/*
The LodePNG_Decoder struct has most input and output parameters the decoder uses,
such as the settings, the info of the PNG and the raw data, and the error. Only
the pixel buffer is not contained in this struct.
*/
typedef struct LodePNG_Decoder
{
  LodePNG_DecodeSettings settings; /*the decoding settings*/
  LodePNG_InfoRaw infoRaw; /*specifies the format in which you would like to get the raw pixel buffer*/
  LodePNG_InfoPng infoPng; /*info of the PNG image obtained after decoding*/
  unsigned error;
} LodePNG_Decoder;

/*init, cleanup and copy functions to use with this struct*/
void LodePNG_Decoder_init(LodePNG_Decoder* decoder);
void LodePNG_Decoder_cleanup(LodePNG_Decoder* decoder);
void LodePNG_Decoder_copy(LodePNG_Decoder* dest, const LodePNG_Decoder* source);

/*
Decode based on a LodePNG_Decoder.
This function allocates the out buffer and stores the size in *outsize. This buffer
needs to be freed after usage.
Other information about the PNG file, such as the size, colorType and extra chunks
are stored in the infoPng field of the LodePNG_Decoder.
*/
void LodePNG_Decoder_decode(LodePNG_Decoder* decoder, unsigned char** out, size_t* outsize,
                            const unsigned char* in, size_t insize);

/*
Read the PNG header, but not the actual data. This returns only the information
that is in the header chunk of the PNG, such as width, height and color type. The
information is placed in the infoPng field of the LodePNG_Decoder.
*/
void LodePNG_Decoder_inspect(LodePNG_Decoder* decoder, const unsigned char* in, size_t insize);
#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER
/* ////////////////////////////////////////////////////////////////////////// */
/* LodePNG Encoder                                                            */
/* ////////////////////////////////////////////////////////////////////////// */

/*Settings for the encoder.*/
typedef struct LodePNG_EncodeSettings
{
  LodePNG_CompressSettings zlibsettings; /*settings for the zlib encoder, such as window size, ...*/

  /*Brute-force-search PNG filters by compressing each filter for each scanline.
  This gives better compression, at the cost of being super slow.
  Don't enable this for normal image saving, compression can take minutes
  instead of seconds. This is experimental. If enabled, compression still isn't
  as good as some other PNG encoders and optimizers, except for some images.
  If you enable this, also consider setting zlibsettings.windowSize to 32768, and consider
  using a less than 24-bit per pixel colorType if the image has <= 256 colors, for optimal compression.
  Default: 0 (false)*/
  unsigned bruteForceFilters;

  /*automatically use color type without alpha instead of given one, if given image is opaque*/
  unsigned autoLeaveOutAlphaChannel;
  /*force creating a PLTE chunk if colortype is 2 or 6 (= a suggested palette).
  If colortype is 3, PLTE is _always_ created.*/
  unsigned force_palette;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  /*add LodePNG version as text chunk*/
  unsigned add_id;
  /*encode text chunks as zTXt chunks instead of tEXt chunks, and use compression in iTXt chunks*/
  unsigned text_compression;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
} LodePNG_EncodeSettings;

void LodePNG_EncodeSettings_init(LodePNG_EncodeSettings* settings);

/*
This struct has most input and output parameters the encoder uses,
such as the settings, the info of the PNG and the raw data, and the error. Only
the pixel buffer is not contained in this struct.
*/
typedef struct LodePNG_Encoder
{
  /*compression settings of the encoder*/
  LodePNG_EncodeSettings settings;
  /*the info specified by the user is not changed by the encoder. The encoder
  will try to generate a PNG close to the given info.*/
  LodePNG_InfoPng infoPng;
  /*put the properties of the input raw image in here*/
  LodePNG_InfoRaw infoRaw;
  /*error value filled in if error happened, or 0 if all went ok*/
  unsigned error;
} LodePNG_Encoder;

/*init, cleanup and copy functions to use with this struct*/
void LodePNG_Encoder_init(LodePNG_Encoder* encoder);
void LodePNG_Encoder_cleanup(LodePNG_Encoder* encoder);
void LodePNG_Encoder_copy(LodePNG_Encoder* dest, const LodePNG_Encoder* source);

/*This function allocates the out buffer with standard malloc and stores the size in *outsize.*/
void LodePNG_Encoder_encode(LodePNG_Encoder* encoder, unsigned char** out, size_t* outsize,
                            const unsigned char* image, unsigned w, unsigned h);
#endif /*LODEPNG_COMPILE_ENCODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* Chunk Traversing Utilities                                                 */
/* ////////////////////////////////////////////////////////////////////////// */

/*
LodePNG_chunk functions:
These functions need as input a large enough amount of allocated memory.
These functions can be used on raw PNG data, but they are exposed in the API
because they are needed if you want to traverse the unknown chunks stored
in the LodePNG_UnknownChunks struct, or add new ones to it.
*/

/*get the length of the data of the chunk. Total chunk length has 12 bytes more.*/
unsigned LodePNG_chunk_length(const unsigned char* chunk);

/*puts the 4-byte type in null terminated string*/
void LodePNG_chunk_type(char type[5], const unsigned char* chunk);

/*check if the type is the given type*/
unsigned char LodePNG_chunk_type_equals(const unsigned char* chunk, const char* type);

/*
These functions get properties of PNG chunks gotten from capitalization of chunk
type name, as defined by the PNG standard.
*/

/*
properties of PNG chunks gotten from capitalization of chunk type name, as defined by the standard
0: ancillary chunk, 1: it's one of the critical chunk types
*/
unsigned char LodePNG_chunk_critical(const unsigned char* chunk);

/*0: public, 1: private*/
unsigned char LodePNG_chunk_private(const unsigned char* chunk);

/*0: the chunk is unsafe to copy, 1: the chunk is safe to copy*/
unsigned char LodePNG_chunk_safetocopy(const unsigned char* chunk);

/*get pointer to the data of the chunk*/
unsigned char* LodePNG_chunk_data(unsigned char* chunk);

/*get pointer to the data of the chunk*/
const unsigned char* LodePNG_chunk_data_const(const unsigned char* chunk);

/*returns 0 if the crc is correct, 1 if it's incorrect*/
unsigned LodePNG_chunk_check_crc(const unsigned char* chunk);

/*generates the correct CRC from the data and puts it in the last 4 bytes of the chunk*/
void LodePNG_chunk_generate_crc(unsigned char* chunk);

/*iterate to next chunks. don't use on IEND chunk, as there is no next chunk then*/
unsigned char* LodePNG_chunk_next(unsigned char* chunk);
const unsigned char* LodePNG_chunk_next_const(const unsigned char* chunk);

/*
Appends chunk to the data in out. The given chunk should already have its chunk header.
The out variable and outlength are updated to reflect the new reallocated buffer.
Returns error code (0 if it went ok)
*/
unsigned LodePNG_append_chunk(unsigned char** out, size_t* outlength, const unsigned char* chunk);

/*
Appends new chunk to out. The chunk to append is given by giving its length, type
and data separately. The type is a 4-letter string.
The out variable and outlength are updated to reflect the new reallocated buffer.
Returne error code (0 if it went ok)
*/
unsigned LodePNG_create_chunk(unsigned char** out, size_t* outlength, unsigned length,
                              const char* type, const unsigned char* data);
#endif /*LODEPNG_COMPILE_PNG*/

#ifdef LODEPNG_COMPILE_ZLIB
/* ////////////////////////////////////////////////////////////////////////// */
/* Zlib encoder and decoder                                                   */
/* ////////////////////////////////////////////////////////////////////////// */

/*
This zlib part can be used independently to zlib compress and decompress a
buffer. It cannot be used to create gzip files however, and it only supports the
part of zlib that is required for PNG, it does not support dictionaries.
*/

#ifdef LODEPNG_COMPILE_DECODER
/*Decompresses Zlib data. Reallocates the out buffer and appends the data. The
data must be according to the zlib specification.
Either, *out must be NULL and *outsize must be 0, or, *out must be a valid
buffer and *outsize its size in bytes. out must be freed by user after usage.*/
unsigned LodePNG_zlib_decompress(unsigned char** out, size_t* outsize,
                                 const unsigned char* in, size_t insize, const LodePNG_DecompressSettings* settings);
#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER
/*Compresses data with Zlib. Reallocates the out buffer and appends the data.
The data is output in the format of the zlib specification.
Either, *out must be NULL and *outsize must be 0, or, *out must be a valid
buffer and *outsize its size in bytes. out must be freed by user after usage.*/
unsigned LodePNG_zlib_compress(unsigned char** out, size_t* outsize,
                               const unsigned char* in, size_t insize, const LodePNG_CompressSettings* settings);
#endif /*LODEPNG_COMPILE_ENCODER*/
#endif /*LODEPNG_COMPILE_ZLIB*/

#ifdef LODEPNG_COMPILE_DISK
/*
Load a file from disk into buffer. The function allocates the out buffer, and
after usage you are responsible for freeing it.
out: output parameter, contains pointer to loaded buffer.
outsize: output parameter, size of the allocated out buffer
filename: the path to the file to load
return value: error code (0 means ok)
*/
unsigned LodePNG_loadFile(unsigned char** out, size_t* outsize, const char* filename);

/*
Save a file from buffer to disk. Warning, if it exists, this function overwrites
the file without warning!
buffer: the buffer to write
buffersize: size of the buffer to write
filename: the path to the file to save to
return value: error code (0 means ok)
*/
unsigned LodePNG_saveFile(const unsigned char* buffer, size_t buffersize, const char* filename);
#endif /*LODEPNG_COMPILE_DISK*/

#ifdef __cplusplus
/* ////////////////////////////////////////////////////////////////////////// */
/* LodePNG C++ wrapper                                                        */
/* ////////////////////////////////////////////////////////////////////////// */

//The LodePNG C++ wrapper uses classes with handy constructors and destructors
//instead of manual init and cleanup functions, and uses std::vectors instead of
//manually allocated memory buffers.

namespace LodePNG
{
#ifdef LODEPNG_COMPILE_ZLIB
//The C++ wrapper for the zlib part
#ifdef LODEPNG_COMPILE_DECODER
  //Zlib-decompress an unsigned char buffer
  unsigned decompress(std::vector<unsigned char>& out, const unsigned char* in, size_t insize,
                      const LodePNG_DecompressSettings& settings = LodePNG_defaultDecompressSettings);

  //Zlib-decompress an std::vector
  unsigned decompress(std::vector<unsigned char>& out, const std::vector<unsigned char>& in,
                      const LodePNG_DecompressSettings& settings = LodePNG_defaultDecompressSettings);
#endif //LODEPNG_COMPILE_DECODER

#ifdef LODEPNG_COMPILE_ENCODER
  //Zlib-compress an unsigned char buffer
  unsigned compress(std::vector<unsigned char>& out, const unsigned char* in, size_t insize,
                    const LodePNG_CompressSettings& settings = LodePNG_defaultCompressSettings);

  //Zlib-compress an std::vector
  unsigned compress(std::vector<unsigned char>& out, const std::vector<unsigned char>& in,
                    const LodePNG_CompressSettings& settings = LodePNG_defaultCompressSettings);
#endif //LODEPNG_COMPILE_ENCODER
#endif //LODEPNG_COMPILE_ZLIB

#ifdef LODEPNG_COMPILE_PNG
#ifdef LODEPNG_COMPILE_DECODER
  /*
  Class to decode a PNG image. Before decoding, settings can be set and
  after decoding, extra information about the PNG can be retrieved.
  Extends from the C-struct LodePNG_Decoder to add constructors and destructors
  to initialize/cleanup it automatically. Beware, no virtual destructor is used.
  */
  class Decoder : public LodePNG_Decoder
  {
    public:

    Decoder();
    ~Decoder();
    void operator=(const LodePNG_Decoder& other);

    //decode PNG buffer to raw out buffer. Width and height can be retrieved with getWidth() and
    //getHeight() and error should be checked with hasError() and getError()
    void decode(std::vector<unsigned char>& out, const unsigned char* in, size_t insize);

    //decode PNG buffer to raw out buffer. Width and height can be retrieved with getWidth() and
    //getHeight() and error should be checked with hasError() and getError()
    void decode(std::vector<unsigned char>& out, const std::vector<unsigned char>& in);

    //inspect functions: get only the info from the PNG header. The info can then be retrieved with
    //the functions of this class.
    void inspect(const unsigned char* in, size_t insize);

    //inspect functions: get only the info from the PNG header. The info can then be retrieved with
    //the functions of this class.
    void inspect(const std::vector<unsigned char>& in);

    //error checking after decoding
    bool hasError() const;
    unsigned getError() const;

    //convenient access to some InfoPng parameters after decoding
    unsigned getWidth() const; //width of image in pixels
    unsigned getHeight() const; //height of image in pixels
    unsigned getBpp(); //bits per pixel
    unsigned getChannels(); //amount of channels
    unsigned isGreyscaleType(); //is it a greyscale type? (colorType 0 or 4)
    unsigned isAlphaType(); //has it an alpha channel? (colorType 2 or 6)

    //getters and setters for the decoding settings
    const LodePNG_DecodeSettings& getSettings() const;
    LodePNG_DecodeSettings& getSettings();
    void setSettings(const LodePNG_DecodeSettings& info);

    //getters and setters for the PNG image info, after decoding this describes information of the PNG image
    const LodePNG_InfoPng& getInfoPng() const;
    LodePNG_InfoPng& getInfoPng();
    void setInfoPng(const LodePNG_InfoPng& info);
    void swapInfoPng(LodePNG_InfoPng& info); //faster than copying with setInfoPng

    //getters and setters for the raw image info, this determines in what format
    //you get the pixel buffer from the decoder
    const LodePNG_InfoRaw& getInfoRaw() const;
    LodePNG_InfoRaw& getInfoRaw();
    void setInfoRaw(const LodePNG_InfoRaw& info);
  };
#endif //LODEPNG_COMPILE_DECODER

#ifdef LODEPNG_COMPILE_ENCODER
  /*
  Class to encode a PNG image. Before encoding, settings can be set.
  Extends from the C-struct LodePNG_Enoder to add constructors and destructors
  to initialize/cleanup it automatically. Beware, no virtual destructor is used.
  */
  class Encoder : public LodePNG_Encoder
  {
    public:

    Encoder();
    ~Encoder();
    void operator=(const LodePNG_Encoder& other);

    //encoding image to PNG buffer
    void encode(std::vector<unsigned char>& out, const unsigned char* image, unsigned w, unsigned h);

    //encoding image to PNG buffer
    void encode(std::vector<unsigned char>& out, const std::vector<unsigned char>& image, unsigned w, unsigned h);

    //error checking after decoding
    bool hasError() const;
    unsigned getError() const;

    //convenient direct access to some parameters of the InfoPng
    void clearPalette();
    void addPalette(unsigned char r, unsigned char g, unsigned char b, unsigned char a); //add 1 color to the palette
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    void clearText();
    void addText(const std::string& key, const std::string& str); //push back both texts at once
    void clearIText();
    void addIText(const std::string& key, const std::string& langtag,
                  const std::string& transkey, const std::string& str);
#endif //LODEPNG_COMPILE_ANCILLARY_CHUNKS

    //getters and setters for the encoding settings
    const LodePNG_EncodeSettings& getSettings() const;
    LodePNG_EncodeSettings& getSettings();
    void setSettings(const LodePNG_EncodeSettings& info);

    //getters and setters for the PNG image info, this describes what color type
    //and other settings the resulting PNG should have
    const LodePNG_InfoPng& getInfoPng() const;
    LodePNG_InfoPng& getInfoPng();
    void setInfoPng(const LodePNG_InfoPng& info);
    void swapInfoPng(LodePNG_InfoPng& info); //faster than copying with setInfoPng

    //getters and setters for the raw image info, this describes how the encoder
    //should interpret the input pixel buffer
    const LodePNG_InfoRaw& getInfoRaw() const;
    LodePNG_InfoRaw& getInfoRaw();
    void setInfoRaw(const LodePNG_InfoRaw& info);
  };
#endif //LODEPNG_COMPILE_ENCODER

#ifdef LODEPNG_COMPILE_DISK
  /*
  Load a file from disk into an std::vector. If the vector is empty, then either
  the file doesn't exist or is an empty file.
  */
  void loadFile(std::vector<unsigned char>& buffer, const std::string& filename);

  /*
  Save the binary data in an std::vector to a file on disk. The file is overwritten
  without warning.
  */
  void saveFile(const std::vector<unsigned char>& buffer, const std::string& filename);
#endif //LODEPNG_COMPILE_DISK
#endif //LODEPNG_COMPILE_PNG
} //namespace LodePNG
#endif /*end of __cplusplus wrapper*/

/*
TODO:
[.] test if there are no memory leaks or security exploits - done a lot but needs to be checked often
[.] check compatibility with vareous compilers  - done but needs to be redone for every newer version
[ ] LZ77 encoder more like the one described in zlib - to make sure it's patentfree
[X] converting color to 16-bit per channel types
[ ] read all public PNG chunk types (but never let the color profile and gamma ones touch RGB values)
[ ] make sure encoder generates no chunks with size > (2^31)-1
[ ] partial decoding (stream processing)
[ ] let the "isFullyOpaque" function check color keys and transparent palettes too
[X] better name for the variables "codes", "codesD", "codelengthcodes", "clcl" and "lldl"
[ ] don't stop decoding on errors like 69, 57, 58 (make warnings)
[ ] make option to choose if the raw image with non multiple of 8 bits per scanline should have padding bits or not
[ ] let the C++ wrapper catch exceptions coming from the standard library and return LodePNG error codes
*/

#endif /*LODEPNG_H inclusion guard*/

/*
LodePNG Documentation
---------------------

This documentations contains background information and examples. For the function
and class documentation, see the comments in the declarations above.

0. table of contents
--------------------

  1. about
   1.1. supported features
   1.2. features not supported
  2. C and C++ version
  3. security
  4. decoding
  5. encoding
  6. color conversions
    6.1. PNG color types
    6.2. Default Behaviour of LodePNG
    6.3. Color Conversions
    6.4. More Notes
  7. error values
  8. chunks and PNG editing
  9. compiler support
  10. examples
   10.1. decoder C++ example
   10.2. encoder C++ example
   10.3. decoder C example
  11. changes
  12. contact information


1. about
--------

PNG is a file format to store raster images losslessly with good compression,
supporting different color types. It can be implemented in a patent-free way.

LodePNG is a PNG codec according to the Portable Network Graphics (PNG)
Specification (Second Edition) - W3C Recommendation 10 November 2003.

The specifications used are:

*) Portable Network Graphics (PNG) Specification (Second Edition):
     http://www.w3.org/TR/2003/REC-PNG-20031110
*) RFC 1950 ZLIB Compressed Data Format version 3.3:
     http://www.gzip.org/zlib/rfc-zlib.html
*) RFC 1951 DEFLATE Compressed Data Format Specification ver 1.3:
     http://www.gzip.org/zlib/rfc-deflate.html

The most recent version of LodePNG can currently be found at
http://members.gamedev.net/lode/projects/LodePNG/

LodePNG works both in C (ISO C90) and C++, with a C++ wrapper that adds
extra functionality.

LodePNG exists out of two files:
-lodepng.h: the header file for both C and C++
-lodepng.c(pp): give it the name lodepng.c or lodepng.cpp (or .cc) depending on your usage

If you want to start using LodePNG right away without reading this doc, get the
files lodepng_examples.c or lodepng_examples.cpp to see how to use it in code,
or check the (smaller) examples in chapter 13 here.

LodePNG is simple but only supports the basic requirements. To achieve
simplicity, the following design choices were made: There are no dependencies
on any external library. To decode PNGs, there's a Decoder struct or class that
can convert any PNG file data into an RGBA image buffer with a single function
call. To encode PNGs, there's an Encoder struct or class that can convert image
data into PNG file data with a single function call. To read and write files,
there are simple functions to convert the files to/from buffers in memory.

This all makes LodePNG suitable for loading textures in games, demoscene
productions, saving a screenshot, images in programs that require them for simple
usage, ... It's less suitable for full fledged image editors, loading PNGs
over network (it requires all the image data to be available before decoding can
begin), life-critical systems, ...
LodePNG has a standards conformant decoder and encoder, and supports the ability
to make a somewhat conformant editor.

1.1. supported features
-----------------------

The following features are supported by the decoder:

*) decoding of PNGs with any color type, bit depth and interlace mode, to a 24- or 32-bit color raw image,
   or the same color type as the PNG
*) encoding of PNGs, from any raw image to 24- or 32-bit color, or the same color type as the raw image
*) Adam7 interlace and deinterlace for any color type
*) loading the image from harddisk or decoding it from a buffer from other sources than harddisk
*) support for alpha channels, including RGBA color model, translucent palettes and color keying
*) zlib decompression (inflate)
*) zlib compression (deflate)
*) CRC32 and ADLER32 checksums
*) handling of unknown chunks, allowing making a PNG editor that stores custom and unknown chunks.
*) the following chunks are supported (generated/interpreted) by both encoder and decoder:
    IHDR: header information
    PLTE: color palette
    IDAT: pixel data
    IEND: the final chunk
    tRNS: transparency for palettized images
    tEXt: textual information
    zTXt: compressed textual information
    iTXt: international textual information
    bKGD: suggested background color
    pHYs: physical dimensions
    tIME: modification time

1.2. features not supported
---------------------------

The following features are _not_ supported:

*) some features needed to make a conformant PNG-Editor might be still missing.
*) partial loading/stream processing. All data must be available and is processed in one call.
*) The following public chunks are not supported but treated as unknown chunks by LodePNG
    cHRM, gAMA, iCCP, sRGB, sBIT, hIST, sPLT


2. C and C++ version
--------------------

The C version uses buffers allocated with alloc that you need to free()
yourself. On top of that, you need to use init and cleanup functions for each
struct whenever using a struct from the C version to avoid exploits and memory leaks.

The C++ version has constructors and destructors that take care of these things,
and uses std::vectors in the interface for storing data.

Both the C and the C++ version are contained in this file! The C++ code depends on
the C code, the C code works on its own.

These files work without modification for both C and C++ compilers because all the
additional C++ code is in "#ifdef __cplusplus" blocks that make C-compilers ignore
it, and the C code is made to compile both with strict ISO C90 and C++.

To use the C++ version, you need to rename the source file to lodepng.cpp (instead
of lodepng.c), and compile it with a C++ compiler.

To use the C version, you need to rename the source file to lodepng.c (instead
of lodepng.cpp), and compile it with a C compiler.


3. Security
-----------

As with most software, even if carefully designed, it's always possible that
LodePNG may contain possible exploits.

If you discover a possible exploit, please let me know, and it will be fixed.

When using LodePNG, care has to be taken with the C version of LodePNG, as well as the C-style
structs when working with C++. The following conventions are used for all C-style structs:

-if a struct has a corresponding init function, always call the init function when making a new one, to avoid exploits
-if a struct has a corresponding cleanup function, call it before the struct disappears to avoid memory leaks
-if a struct has a corresponding copy function, use the copy function instead of "=".
 The destination must also be inited already!

4. Decoding
-----------

Decoding converts a PNG compressed image to a raw pixel buffer.

Most documentation on using the decoder is at its declarations in the header
above. For C, simple decoding can be done with functions such as LodePNG_decode32,
and more advanced decoding can be done with the struct LodePNG_Decoder and its
functions. For C++, simple decoding can be done with the LodePNG::decode functions
and advanced decoding with the LodePNG::Decoder class.

The Decoder contains 3 components:
*) LodePNG_InfoPng: it stores information about the PNG (the input) in an LodePNG_InfoPng struct,
   don't modify this one yourself
*) Settings: you can specify a few other settings for the decoder to use
*) LodePNG_InfoRaw: here you can say what type of raw image (the output) you want to get

Some of the parameters described below may be inside the sub-struct "LodePNG_InfoColor color".
In the C and C++ version, when using Info structs outside of the decoder or encoder, you need to use their
init and cleanup functions, but normally you use the ones in the decoder that are already handled
in the init and cleanup functions of the decoder itself.

=LodePNG_InfoPng=

This contains information such as the original color type of the PNG image, text
comments, suggested background color, etc... More details about the LodePNG_InfoPng struct
are at its declaration documentation.

Because the dimensions of the image are important, there are shortcuts to get them in the
C++ version: use decoder.getWidth() and decoder.getHeight().
In the C version, use decoder.infoPng.width and decoder.infoPng.height.

=LodePNG_InfoRaw=

In the LodePNG_InfoRaw struct of the Decoder, you can specify which color type you want
the resulting raw image to be. If this is different from the colorType of the
PNG, then the decoder will automatically convert the result to your LodePNG_InfoRaw
settings. Not all combinations of color conversions are supported though, see
a different section for information about the color modes and supported conversions.

Palette of LodePNG_InfoRaw isn't used by the Decoder, when converting from palette color
to palette color, the values of the pixels are left untouched so that the colors
will change if the palette is different. Color key of LodePNG_InfoRaw is not used by the
Decoder. If setting color_convert is false then LodePNG_InfoRaw is completely ignored,
but it will be modified to match the color type of the PNG so will be overwritten.

By default, 32-bit color is used for the result.

=Settings=

The Settings can be used to ignore the errors created by invalid CRC and Adler32
chunks, and to disable the decoding of tEXt chunks.

There's also a setting color_convert, true by default. If false, no conversion
is done, the resulting data will be as it was in the PNG (after decompression)
and you'll have to puzzle the colors of the pixels together yourself using the
color type information in the LodePNG_InfoPng.


5. Encoding
-----------

Encoding converts a raw pixel buffer to a PNG compressed image.

Most documentation on using the encoder is at its declarations in the header
above. For C, simple encoding can be done with functions such as LodePNG_encode32,
and more advanced decoding can be done with the struct LodePNG_Encoder and its
functions. For C++, simple encoding can be done with the LodePNG::encode functions
and advanced decoding with the LodePNG::Encoder class.

Like the decoder, the encoder can also give errors. However it gives less errors
since the encoder input is trusted, the decoder input (a PNG image that could
be forged by anyone) is not trusted.

Like the Decoder, the Encoder has 3 components:
*) LodePNG_InfoRaw: here you say what color type of the raw image (the input) has
*) Settings: you can specify a few settings for the encoder to use
*) LodePNG_InfoPng: the same LodePNG_InfoPng struct as created by the Decoder. For the encoder,
with this you specify how you want the PNG (the output) to be.

Some of the parameters described below may be inside the sub-struct "LodePNG_InfoColor color".
In the C and C++ version, when using Info structs outside of the decoder or encoder, you need to use their
init and cleanup functions, but normally you use the ones in the encoder that are already handled
in the init and cleanup functions of the decoder itself.

=LodePNG_InfoPng=

The Decoder class stores information about the PNG image in an LodePNG_InfoPng object. With
the Encoder you can do the opposite: you give it an LodePNG_InfoPng object, and it'll try
to match the LodePNG_InfoPng you give as close as possible in the PNG it encodes. For
example in the LodePNG_InfoPng you can specify the color type you want to use, possible
tEXt chunks you want the PNG to contain, etc... For an explanation of all the
values in LodePNG_InfoPng see a further section. Not all PNG color types are supported
by the Encoder.

The encoder will not always exactly match the LodePNG_InfoPng struct you give,
it tries as close as possible. Some things are ignored by the encoder. The width
and height of LodePNG_InfoPng are ignored as well, because instead the width and
height of the raw image you give in the input are used. In fact the encoder
currently uses only the following settings from it:
-colorType and bitDepth: the ones it supports
-text chunks, that you can add to the LodePNG_InfoPng with "addText"
-the color key, if applicable for the given color type
-the palette, if you encode to a PNG with colorType 3
-the background color: it'll add a bKGD chunk to the PNG if one is given
-the interlaceMethod: None (0) or Adam7 (1)

When encoding to a PNG with colorType 3, the encoder will generate a PLTE chunk.
If the palette contains any colors for which the alpha channel is not 255 (so
there are translucent colors in the palette), it'll add a tRNS chunk.

=LodePNG_InfoRaw=

You specify the color type of the raw image that you give to the input here,
including a possible transparent color key and palette you happen to be using in
your raw image data.

By default, 32-bit color is assumed, meaning your input has to be in RGBA
format with 4 bytes (unsigned chars) per pixel.

=Settings=

The following settings are supported (some are in sub-structs):
*) autoLeaveOutAlphaChannel: when this option is enabled, when you specify a PNG
color type with alpha channel (not to be confused with the color type of the raw
image you specify!!), but the encoder detects that all pixels of the given image
are opaque, then it'll automatically use the corresponding type without alpha
channel, resulting in a smaller PNG image.
*) btype: the block type for LZ77.
   0 = uncompressed, 1 = fixed huffman tree, 2 = dynamic huffman tree (best compression)
*) useLZ77: whether or not to use LZ77 for compressed block types
*) windowSize: the window size used by the LZ77 encoder (1 - 32768)
*) force_palette: if colorType is 2 or 6, you can make the encoder write a PLTE
   chunk if force_palette is true. This can used as suggested palette to convert
   to by viewers that don't support more than 256 colors (if those still exist)
*) add_id: add text chunk "Encoder: LodePNG <version>" to the image.
*) text_compression: default 0. If 1, it'll store texts as zTXt instead of tEXt chunks.
  zTXt chunks use zlib compression on the text. This gives a smaller result on
  large texts but a larger result on small texts (such as a single program name).
  It's all tEXt or all zTXt though, there's no separate setting per text yet.


6. color conversions
--------------------

In LodePNG, the color mode (bits, channels and palette) used in the PNG image,
and the color mode used in the raw data, are separate and independently
configurable. Therefore, LodePNG needs to do conversions from one color mode to
another. Not all possible conversions are supported (e.g. converting to a palette
or a lower bit depth isn't supported). This section will explain which conversions
are supported and how to configure this. This explains for example when LodePNG
uses the settings in LodePNG_InfoPng, LodePNG_InfoRaw and Settings.

6.1. PNG color types
--------------------

A PNG image can have many color types, ranging from 1-bit color to 64-bit color,
as well as palettized color modes. After the zlib decompression and unfiltering
in the PNG image is done, the raw pixel data will have that color type and thus
a certain amount of bits per pixel. If you want the output raw image after
decoding to have another color type, a conversion is done by LodePNG.

The PNG specification mentions the following color types:

0: greyscale, bit depths 1, 2, 4, 8, 16
2: RGB, bit depths 8 and 16
3: palette, bit depths 1, 2, 4 and 8
4: greyscale with alpha, bit depths 8 and 16
6: RGBA, bit depths 8 and 16

Bit depth is the amount of bits per pixel per color channel. So the total amount
of bits per pixel = amount of channels * bitDepth.

6.2. Default Behaviour of LodePNG
---------------------------------

By default, the Decoder will convert the data from the PNG to 32-bit RGBA color,
no matter what color type the PNG has, so that the result can be used directly
as a texture in OpenGL etc... without worries about what color type the original
image has.

The Encoder assumes by default that the raw input you give it is a 32-bit RGBA
buffer and will store the PNG as either 32 bit or 24 bit depending on whether
or not any translucent pixels were detected in it.

To get the default behaviour, don't change the values of LodePNG_InfoRaw and
LodePNG_InfoPng of the encoder, and don't change the values of LodePNG_InfoRaw
of the decoder.

6.3. Color Conversions
----------------------

As explained in the sections about the Encoder and Decoder, you can specify
color types and bit depths in LodePNG_InfoPng and LodePNG_InfoRaw, to change the default behaviour
explained above. (for the Decoder you can only specify the LodePNG_InfoRaw, because the
LodePNG_InfoPng contains what the PNG file has).

To avoid some confusion:
-the Decoder converts from PNG to raw image
-the Encoder converts from raw image to PNG
-the color type and bit depth in LodePNG_InfoRaw, are those of the raw image
-the color type and bit depth in LodePNG_InfoPng, are those of the PNG
-if the color type of the LodePNG_InfoRaw and PNG image aren't the same, a conversion
between the color types is done if the color types are supported. If it is not
supported, an error is returned. If the types are the same, no conversion is done
and this is supported for any color type.

Supported color conversions:
-It's possible to load PNGs from any colortype and to save PNGs of any colorType.
-Both encoder and decoder use the same converter. So both encoder and decoder
suport the same color types at the input and the output. So the decoder supports
any type of PNG image and can convert it to certain types of raw image, while the
encoder supports any type of raw data but only certain color types for the output PNG.
-The converter can convert from _any_ input color type, to 24-bit RGB or 32-bit RGBA (8 bits per channel)
-The converter can convert from _any_ input color type, to 48-bit RGB or 64-bit RGBA (16 bits per channel)
-The converter can convert from greyscale input color type, to 8-bit greyscale or greyscale with alpha
-The converter can convert from greyscale input color type, to 16-bit greyscale or greyscale with alpha
-If both color types are the same, conversion from anything to anything is possible
-Color types that are invalid according to the PNG specification are not allowed
-When converting from a type with alpha channel to one without, the alpha channel information is discarded
-When converting from a type without alpha channel to one with, the result will be opaque except
 pixels that have the same color as the color key of the input if one was given
-When converting from 16-bit bitDepth to 8-bit bitDepth, the 16-bit precision information is lost,
 only the most significant byte is kept
-When converting from 8-bit bitDepth to 16-bit bitDepth, the 8-bit byte is duplicated, so that 0 stays 0, and
 the 8-bit maximum 255 is converted to the 16-bit maximum 65535. Something similar happens at bit level for
 converting very low bit depths to 8 or 16 bit.
-Converting from color to greyscale or to palette is not supported on purpose: there are multiple possible
 algorithms to do this color reduction, LodePNG does not want to pick one and leaves this choice to the
 user instead, because it's beyond the scope of PNG encoding.
-Converting from a palette to a palette, only keeps the indices, it ignores the colors defined in the
 palette without giving an error

No conversion needed...:
-If the color type of the PNG image and raw image are the same, then no
conversion is done, and all color types are supported.
-In the encoder, you can make it save a PNG with any color by giving the
LodePNG_InfoRaw and LodePNG_InfoPng the same color type.
-In the decoder, you can make it store the pixel data in the same color type
as the PNG has, by setting the color_convert setting to false. Settings in
infoRaw are then ignored.

The function LodePNG_convert does this, which is available in the interface but
normally isn't needed since the encoder and decoder already call it.

6.4. More Notes
---------------

In the PNG file format, if a less than 8-bit per pixel color type is used and the scanlines
have a bit amount that isn't a multiple of 8, then padding bits are used so that each
scanline starts at a fresh byte. But that is NOT true for the LodePNG input and output!
The raw input image you give to the encoder, and the raw output image you get from the decoder
will NOT have these padding bits, e.g. in the case of a 1-bit image with a width
of 7 pixels, the first pixel of the second scanline will the the 8th bit of the first byte,
not the first bit of a new byte.

7. error values
---------------

All functions in LodePNG that return an error code, return 0 if everything went
OK, or one of the code defined by LodePNG if there was an error.

The meaning of the LodePNG error values can be retrieved with the function
LodePNG_error_text: given the numerical error code, it returns a description
of the error in English as a string.

Check the implementation of LodePNG_error_text to see the meaning of each error code.


8. chunks and PNG editing
-------------------------

If you want to add extra chunks to a PNG you encode, or use LodePNG for a PNG
editor that should follow the rules about handling of unknown chunks, or if you
program is able to read other types of chunks than the ones handled by LodePNG,
then that's possible with the chunk functions of LodePNG.

A PNG chunk has the following layout:

4 bytes length
4 bytes type name
length bytes data
4 bytes CRC


8.1. iterating through chunks
-----------------------------

If you have a buffer containing the PNG image data, then the first chunk (the
IHDR chunk) starts at byte number 8 of that buffer. The first 8 bytes are the
signature of the PNG and are not part of a chunk. But if you start at byte 8
then you have a chunk, and can check the following things of it.

NOTE: none of these functions check for memory buffer boundaries. To avoid
exploits, always make sure the buffer contains all the data of the chunks.
When using LodePNG_chunk_next, make sure the returned value is within the
allocated memory.

unsigned LodePNG_chunk_length(const unsigned char* chunk):

Get the length of the chunk's data. The total chunk length is this length + 12.

void LodePNG_chunk_type(char type[5], const unsigned char* chunk):
unsigned char LodePNG_chunk_type_equals(const unsigned char* chunk, const char* type):

Get the type of the chunk or compare if it's a certain type

unsigned char LodePNG_chunk_critical(const unsigned char* chunk):
unsigned char LodePNG_chunk_private(const unsigned char* chunk):
unsigned char LodePNG_chunk_safetocopy(const unsigned char* chunk):

Check if the chunk is critical in the PNG standard (only IHDR, PLTE, IDAT and IEND are).
Check if the chunk is private (public chunks are part of the standard, private ones not).
Check if the chunk is safe to copy. If it's not, then, when modifying data in a critical
chunk, unsafe to copy chunks of the old image may NOT be saved in the new one if your
program doesn't handle that type of unknown chunk.

unsigned char* LodePNG_chunk_data(unsigned char* chunk):
const unsigned char* LodePNG_chunk_data_const(const unsigned char* chunk):

Get a pointer to the start of the data of the chunk.

unsigned LodePNG_chunk_check_crc(const unsigned char* chunk):
void LodePNG_chunk_generate_crc(unsigned char* chunk):

Check if the crc is correct or generate a correct one.

unsigned char* LodePNG_chunk_next(unsigned char* chunk):
const unsigned char* LodePNG_chunk_next_const(const unsigned char* chunk):

Iterate to the next chunk. This works if you have a buffer with consecutive chunks. Note that these
functions do no boundary checking of the allocated data whatsoever, so make sure there is enough
data available in the buffer to be able to go to the next chunk.

unsigned LodePNG_append_chunk(unsigned char** out, size_t* outlength, const unsigned char* chunk):
unsigned LodePNG_create_chunk(unsigned char** out, size_t* outlength, unsigned length,
                              const char* type, const unsigned char* data):

These functions are used to create new chunks that are appended to the data in *out that has
length *outlength. The append function appends an existing chunk to the new data. The create
function creates a new chunk with the given parameters and appends it. Type is the 4-letter
name of the chunk.


8.2. chunks in infoPng
----------------------

The LodePNG_InfoPng struct contains a struct LodePNG_UnknownChunks in it. This
struct has 3 buffers (each with size) to contain 3 types of unknown chunks:
the ones that come before the PLTE chunk, the ones that come between the PLTE
and the IDAT chunks, and the ones that come after the IDAT chunks.
It's necessary to make the distionction between these 3 cases because the PNG
standard forces to keep the ordering of unknown chunks compared to the critical
chunks, but does not force any other ordering rules.

infoPng.unknown_chunks.data[0] is the chunks before PLTE
infoPng.unknown_chunks.data[1] is the chunks after PLTE, before IDAT
infoPng.unknown_chunks.data[2] is the chunks after IDAT

The chunks in these 3 buffers can be iterated through and read by using the same
way described in the previous subchapter.

When using the decoder to decode a PNG, you can make it store all unknown chunks
if you set the option settings.rememberUnknownChunks to 1. By default, this option
is off and is 0.

The encoder will always encode unknown chunks that are stored in the infoPng. If
you need it to add a particular chunk that isn't known by LodePNG, you can use
LodePNG_append_chunk or LodePNG_create_chunk to the chunk data in
infoPng.unknown_chunks.data[x].

Chunks that are known by LodePNG should not be added in that way. E.g. to make
LodePNG add a bKGD chunk, set background_defined to true and add the correct
parameters there and LodePNG will generate the chunk.


9. compiler support
-------------------

No libraries other than the current standard C library are needed to compile
LodePNG. For the C++ version, only the standard C++ library is needed on top.
Add the files lodepng.c(pp) and lodepng.h to your project, include
lodepng.h where needed, and your program can read/write PNG files.

Use optimization! For both the encoder and decoder, compiling with the best
optimizations makes a large difference.

Make sure that LodePNG is compiled with the same compiler of the same version
and with the same settings as the rest of the program, or the interfaces with
std::vectors and std::strings in C++ can be incompatible resulting in bad things.

CHAR_BITS must be 8 or higher, because LodePNG uses unsigned chars for octets.

*) gcc and g++

LodePNG is developed in gcc so this compiler is natively supported. It gives no
warnings with compiler options "-Wall -Wextra -pedantic -ansi", with gcc and g++
version 4.5.1 on Linux.

*) Mingw and Bloodshed DevC++

The Mingw compiler (a port of gcc) used by Bloodshed DevC++ for Windows is fully
supported by LodePNG.

*) Visual Studio 2005 and Visual C++ 2005 Express Edition

Versions 20070604 up to 20080107 have been tested on VS2005 and work. Visual
studio may give warnings about 'fopen' being deprecated. A multiplatform library
can't support the proposed Visual Studio alternative however.

If you're using LodePNG in VS2005 and don't want to see the deprecated warnings,
put this on top of lodepng.h before the inclusions: #define _CRT_SECURE_NO_DEPRECATE

*) Visual Studio 6.0

The C++ version of LodePNG was not supported by Visual Studio 6.0 because Visual
Studio 6.0 doesn't follow the C++ standard and implements it incorrectly.
The current C version of LodePNG has not been tested in VS6 but may work now.

*) Comeau C/C++

Vesion 20070107 compiles without problems on the Comeau C/C++ Online Test Drive
at http://www.comeaucomputing.com/tryitout in both C90 and C++ mode.

*) Compilers on Macintosh

LodePNG has been reported to work both with the gcc and LLVM for Macintosh, both
for C and C++.

*) Other Compilers

If you encounter problems on other compilers, I'm happy to help out make LodePNG
support the compiler if it supports the ISO C90 and C++ standard well enough and
the required modification doesn't require using non standard or less good C/C++
code or headers.


10. examples
------------

This decoder and encoder example show the most basic usage of LodePNG (using the
classes, not the simple functions, which would be trivial)

More complex examples can be found in:
-lodepng_examples.c: 9 different examples in C, such as showing the image with SDL, ...
-lodepng_examples.cpp: the same examples in C++ using the C++ wrapper of LodePNG

These files can be found on the LodePNG website or searched for on the internet.

10.1. decoder C++ example
-------------------------

////////////////////////////////////////////////////////////////////////////////
#include "lodepng.h"
#include <iostream>

int main(int argc, char *argv[])
{
  const char* filename = argc > 1 ? argv[1] : "test.png";

  //load and decode
  std::vector<unsigned char> buffer, image; //buffer will contain the PNG file, image will contain the raw pixels
  LodePNG::loadFile(buffer, filename); //load the image file with given filename
  LodePNG::Decoder decoder;
  decoder.decode(image, buffer.size() ? &buffer[0] : 0, (unsigned)buffer.size()); //decode the png

  //if there's an error, display it
  if(decoder.hasError())
    std::cout << "error " << decoder.getError() << ": "<< LodePNG_error_text(decoder.getError()) << std::endl;

  int width = decoder.getWidth(); //get the width in pixels
  int height = decoder.getHeight(); //get the height in pixels
  //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
}

//alternative version using the "simple" function
int main(int argc, char *argv[])
{
  const char* filename = argc > 1 ? argv[1] : "test.png";

  //load and decode
  std::vector<unsigned char> image;
  unsigned width, height;
  unsigned error = LodePNG::decode(image, width, height, filename);

  //if there's an error, display it
  if(error != 0) std::cout << "error " << error << ": " << LodePNG_error_text(error) << std::endl;

  //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
}
////////////////////////////////////////////////////////////////////////////////


10.2. encoder C++ example
-------------------------

////////////////////////////////////////////////////////////////////////////////
#include "lodepng.h"
#include <iostream>

//saves image to filename given as argument. Warning, this overwrites the file without warning!
int main(int argc, char *argv[])
{
  //check if user gave a filename
  if(argc <= 1)
  {
    std::cout << "please provide a filename to save to\n";
    return 0;
  }

  //generate some image
  std::vector<unsigned char> image;
  image.resize(512 * 512 * 4);
  for(unsigned y = 0; y < 512; y++)
  for(unsigned x = 0; x < 512; x++)
  {
    image[4 * 512 * y + 4 * x + 0] = 255 * !(x & y);
    image[4 * 512 * y + 4 * x + 1] = x ^ y;
    image[4 * 512 * y + 4 * x + 2] = x | y;
    image[4 * 512 * y + 4 * x + 3] = 255;
  }

  //encode and save, using the Encoder class
  std::vector<unsigned char> buffer;
  LodePNG::Encoder encoder;
  encoder.encode(buffer, image, 512, 512);
  LodePNG::saveFile(buffer, argv[1]);

  //the same as the 4 lines of code above, but in 1 call without the class:
  //LodePNG::encode(argv[1], image, 512, 512);
}
////////////////////////////////////////////////////////////////////////////////


10.3. Decoder C example
-----------------------

This example loads the PNG from a file into a pixel buffer in 1 function call

#include "lodepng.h"

int main(int argc, char *argv[])
{
  unsigned error;
  unsigned char* image;
  size_t width, height;

  if(argc <= 1) return 0;

  error = LodePNG_decode32_file(&image, &width, &height, filename);

  if(error != 0) printf("error %u: %s\n", error, LodePNG_error_text(error));

  //use image here

  free(image);
}


11. changes
-----------

The version number of LodePNG is the date of the change given in the format
yyyymmdd.

Some changes aren't backwards compatible. Those are indicated with a (!)
symbol.

*) 8 sep 2011: lz77 encoder lazy matching instead of greedy matching.
*) 23 aug 2011: tweaked the zlib compression parameters after benchmarking.
    A bug with the PNG filtertype heuristic was fixed, so that it chooses much
    better ones (it's quite significant). A setting to do an experimental, slow,
    brute force search for PNG filter types is added.
*) 17 aug 2011 (!): changed some C zlib related function names.
*) 16 aug 2011: made the code less wide (max 120 characters per line).
*) 17 apr 2011: code cleanup. Bugfixes. Convert low to 16-bit per sample colors.
*) 21 feb 2011: fixed compiling for C90. Fixed compiling with sections disabled.
*) 11 dec 2010: encoding is made faster, based on suggestion by Peter Eastman
    to optimize long sequences of zeros.
*) 13 nov 2010: added LodePNG_InfoColor_hasPaletteAlpha and
    LodePNG_InfoColor_canHaveAlpha functions for convenience.
*) 7 nov 2010: added LodePNG_error_text function to get error code description.
*) 30 okt 2010: made decoding slightly faster
*) 26 okt 2010: (!) changed some C function and struct names (more consistent).
     Reorganized the documentation and the declaration order in the header.
*) 08 aug 2010: only changed some comments and external samples.
*) 05 jul 2010: fixed bug thanks to warnings in the new gcc version.
*) 14 mar 2010: fixed bug where too much memory was allocated for char buffers.
*) 02 sep 2008: fixed bug where it could create empty tree that linux apps could
    read by ignoring the problem but windows apps couldn't.
*) 06 jun 2008: added more error checks for out of memory cases.
*) 26 apr 2008: added a few more checks here and there to ensure more safety.
*) 06 mar 2008: crash with encoding of strings fixed
*) 02 feb 2008: support for international text chunks added (iTXt)
*) 23 jan 2008: small cleanups, and #defines to divide code in sections
*) 20 jan 2008: support for unknown chunks allowing using LodePNG for an editor.
*) 18 jan 2008: support for tIME and pHYs chunks added to encoder and decoder.
*) 17 jan 2008: ability to encode and decode compressed zTXt chunks added
    Also vareous fixes, such as in the deflate and the padding bits code.
*) 13 jan 2008: Added ability to encode Adam7-interlaced images. Improved
    filtering code of encoder.
*) 07 jan 2008: (!) changed LodePNG to use ISO C90 instead of C++. A
    C++ wrapper around this provides an interface almost identical to before.
    Having LodePNG be pure ISO C90 makes it more portable. The C and C++ code
    are together in these files but it works both for C and C++ compilers.
*) 29 dec 2007: (!) changed most integer types to unsigned int + other tweaks
*) 30 aug 2007: bug fixed which makes this Borland C++ compatible
*) 09 aug 2007: some VS2005 warnings removed again
*) 21 jul 2007: deflate code placed in new namespace separate from zlib code
*) 08 jun 2007: fixed bug with 2- and 4-bit color, and small interlaced images
*) 04 jun 2007: improved support for Visual Studio 2005: crash with accessing
    invalid std::vector element [0] fixed, and level 3 and 4 warnings removed
*) 02 jun 2007: made the encoder add a tag with version by default
*) 27 may 2007: zlib and png code separated (but still in the same file),
    simple encoder/decoder functions added for more simple usage cases
*) 19 may 2007: minor fixes, some code cleaning, new error added (error 69),
    moved some examples from here to lodepng_examples.cpp
*) 12 may 2007: palette decoding bug fixed
*) 24 apr 2007: changed the license from BSD to the zlib license
*) 11 mar 2007: very simple addition: ability to encode bKGD chunks.
*) 04 mar 2007: (!) tEXt chunk related fixes, and support for encoding
    palettized PNG images. Plus little interface change with palette and texts.
*) 03 mar 2007: Made it encode dynamic Huffman shorter  with repeat codes.
    Fixed a bug where the end code of a block had length 0 in the Huffman tree.
*) 26 feb 2007: Huffman compression with dynamic trees (BTYPE 2) now implemented
    and supported by the encoder, resulting in smaller PNGs at the output.
*) 27 jan 2007: Made the Adler-32 test faster so that a timewaste is gone.
*) 24 jan 2007: gave encoder an error interface. Added color conversion from any
    greyscale type to 8-bit greyscale with or without alpha.
*) 21 jan 2007: (!) Totally changed the interface. It allows more color types
    to convert to and is more uniform. See the manual for how it works now.
*) 07 jan 2007: Some cleanup & fixes, and a few changes over the last days:
    encode/decode custom tEXt chunks, separate classes for zlib & deflate, and
    at last made the decoder give errors for incorrect Adler32 or Crc.
*) 01 jan 2007: Fixed bug with encoding PNGs with less than 8 bits per channel.
*) 29 dec 2006: Added support for encoding images without alpha channel, and
    cleaned out code as well as making certain parts faster.
*) 28 dec 2006: Added "Settings" to the encoder.
*) 26 dec 2006: The encoder now does LZ77 encoding and produces much smaller files now.
    Removed some code duplication in the decoder. Fixed little bug in an example.
*) 09 dec 2006: (!) Placed output parameters of public functions as first parameter.
    Fixed a bug of the decoder with 16-bit per color.
*) 15 okt 2006: Changed documentation structure
*) 09 okt 2006: Encoder class added. It encodes a valid PNG image from the
    given image buffer, however for now it's not compressed.
*) 08 sep 2006: (!) Changed to interface with a Decoder class
*) 30 jul 2006: (!) LodePNG_InfoPng , width and height are now retrieved in different
    way. Renamed decodePNG to decodePNGGeneric.
*) 29 jul 2006: (!) Changed the interface: image info is now returned as a
    struct of type LodePNG::LodePNG_Info, instead of a vector, which was a bit clumsy.
*) 28 jul 2006: Cleaned the code and added new error checks.
    Corrected terminology "deflate" into "inflate".
*) 23 jun 2006: Added SDL example in the documentation in the header, this
    example allows easy debugging by displaying the PNG and its transparency.
*) 22 jun 2006: (!) Changed way to obtain error value. Added
    loadFile function for convenience. Made decodePNG32 faster.
*) 21 jun 2006: (!) Changed type of info vector to unsigned.
    Changed position of palette in info vector. Fixed an important bug that
    happened on PNGs with an uncompressed block.
*) 16 jun 2006: Internally changed unsigned into unsigned where
    needed, and performed some optimizations.
*) 07 jun 2006: (!) Renamed functions to decodePNG and placed them
    in LodePNG namespace. Changed the order of the parameters. Rewrote the
    documentation in the header. Renamed files to lodepng.cpp and lodepng.h
*) 22 apr 2006: Optimized and improved some code
*) 07 sep 2005: (!) Changed to std::vector interface
*) 12 aug 2005: Initial release (C++, decoder only)


12. contact information
-----------------------

Feel free to contact me with suggestions, problems, comments, ... concerning
LodePNG. If you encounter a PNG image that doesn't work properly with this
decoder, feel free to send it and I'll use it to find and fix the problem.

My email address is (puzzle the account and domain together with an @ symbol):
Domain: gmail dot com.
Account: lode dot vandevenne.


Copyright (c) 2005-2011 Lode Vandevenne
*/
