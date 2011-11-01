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

/*
The manual and changelog are in the header file "lodepng.h"
Rename this file to lodepng.cpp to use it for C++, or to lodepng.c to use it for C.
*/

#include "lodepng.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
#include <fstream>
#endif /*__cplusplus*/

#define VERSION_STRING "20110908"

/*
This source file is built up in the following large parts. The code sections
with the "LODEPNG_COMPILE_" #defines divide this up further in an intermixed way.
-Tools for C and common code for PNG and Zlib
-C Code for Zlib (huffman, deflate, ...)
-C Code for PNG (file format chunks, adam7, PNG filters, color conversions, ...)
-The C++ wrapper around all of the above
*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // Tools for C, and common code for PNG and Zlib.                       // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

/*
Often in case of an error a value is assigned to a variable and then it breaks
out of a loop (to go to the cleanup phase of a function). This macro does that.
It makes the error handling code shorter and more readable.

Example: if(!uivector_resizev(&frequencies_ll, 286, 0)) ERROR_BREAK(9924);
*/
#define CERROR_BREAK(errorvar, code)\
{\
  errorvar = code;\
  break;\
}

/*version of CERROR_BREAK that assumes the common case where the error variable is named "error"*/
#define ERROR_BREAK(code) CERROR_BREAK(error, code)

/*
About vector, uivector, ucvector and string:
-All of them wrap dynamic arrays or text strings in a similar way.
-LodePNG was originally written in C++. The vectors replace the std::vectors that were used in the C++ version.
-The string tools are made to avoid problems with compilers that declare things like strncat as deprecated.
-They're not used in the interface, only internally in this file as static functions.
-As with many other structs in this file, the init and cleanup functions serve as ctor and dtor.
*/

#ifdef LODEPNG_COMPILE_ZLIB
#ifdef LODEPNG_COMPILE_ENCODER

typedef struct vector /*dynamic vector of void* pointers. This one is used only by the deflate compressor*/
{
  void* data;
  size_t size; /*in groups of bytes depending on type*/
  size_t allocsize; /*in bytes*/
  unsigned typesize; /*sizeof the type you store in data*/
} vector;

static unsigned vector_resize(vector* p, size_t size) /*returns 1 if success, 0 if failure ==> nothing done*/
{
  if(size * p->typesize > p->allocsize)
  {
    size_t newsize = size * p->typesize * 2;
    void* data = realloc(p->data, newsize);
    if(data)
    {
      p->allocsize = newsize;
      p->data = data;
      p->size = size;
    }
    else return 0;
  }
  else p->size = size;
  return 1;
}

/*resize and use destructor on elements if it gets smaller*/
static unsigned vector_resized(vector* p, size_t size, void dtor(void*))
{
  size_t i;
  if(size < p->size)
  {
    for(i = size; i < p->size; i++)
    {
      dtor(&((char*)(p->data))[i * p->typesize]);
    }
  }
  return vector_resize(p, size);
}

static void vector_cleanup(void* p)
{
  ((vector*)p)->size = ((vector*)p)->allocsize = 0;
  free(((vector*)p)->data);
  ((vector*)p)->data = NULL;
}

static void vector_cleanupd(vector* p, void dtor(void*)) /*clear and use destructor on elements*/
{
  vector_resized(p, 0, dtor);
  vector_cleanup(p);
}

static void vector_init(vector* p, unsigned typesize)
{
  p->data = NULL;
  p->size = p->allocsize = 0;
  p->typesize = typesize;
}

static void vector_swap(vector* p, vector* q) /*they're supposed to have the same typesize*/
{
  size_t tmp;
  void* tmpp;
  tmp = p->size; p->size = q->size; q->size = tmp;
  tmp = p->allocsize; p->allocsize = q->allocsize; q->allocsize = tmp;
  tmpp = p->data; p->data = q->data; q->data = tmpp;
}

static void* vector_get(vector* p, size_t index)
{
  return &((char*)p->data)[index * p->typesize];
}

#endif /*LODEPNG_COMPILE_ENCODER*/
#endif /*LODEPNG_COMPILE_ZLIB*/

/* /////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_ZLIB
/*dynamic vector of unsigned ints*/
typedef struct uivector
{
  unsigned* data;
  size_t size; /*size in number of unsigned longs*/
  size_t allocsize; /*allocated size in bytes*/
} uivector;

static void uivector_cleanup(void* p)
{
  ((uivector*)p)->size = ((uivector*)p)->allocsize = 0;
  free(((uivector*)p)->data);
  ((uivector*)p)->data = NULL;
}

/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned uivector_resize(uivector* p, size_t size)
{
  if(size * sizeof(unsigned) > p->allocsize)
  {
    size_t newsize = size * sizeof(unsigned) * 2;
    void* data = realloc(p->data, newsize);
    if(data)
    {
      p->allocsize = newsize;
      p->data = (unsigned*)data;
      p->size = size;
    }
    else return 0;
  }
  else p->size = size;
  return 1;
}

/*resize and give all new elements the value*/
static unsigned uivector_resizev(uivector* p, size_t size, unsigned value)
{
  size_t oldsize = p->size, i;
  if(!uivector_resize(p, size)) return 0;
  for(i = oldsize; i < size; i++) p->data[i] = value;
  return 1;
}

static void uivector_init(uivector* p)
{
  p->data = NULL;
  p->size = p->allocsize = 0;
}

#ifdef LODEPNG_COMPILE_ENCODER
/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned uivector_push_back(uivector* p, unsigned c)
{
  if(!uivector_resize(p, p->size + 1)) return 0;
  p->data[p->size - 1] = c;
  return 1;
}

/*copy q to p, returns 1 if success, 0 if failure ==> nothing done*/
static unsigned uivector_copy(uivector* p, const uivector* q)
{
  size_t i;
  if(!uivector_resize(p, q->size)) return 0;
  for(i = 0; i < q->size; i++) p->data[i] = q->data[i];
  return 1;
}

static void uivector_swap(uivector* p, uivector* q)
{
  size_t tmp;
  unsigned* tmpp;
  tmp = p->size; p->size = q->size; q->size = tmp;
  tmp = p->allocsize; p->allocsize = q->allocsize; q->allocsize = tmp;
  tmpp = p->data; p->data = q->data; q->data = tmpp;
}
#endif /*LODEPNG_COMPILE_ENCODER*/
#endif /*LODEPNG_COMPILE_ZLIB*/

/* /////////////////////////////////////////////////////////////////////////// */

/*dynamic vector of unsigned chars*/
typedef struct ucvector
{
  unsigned char* data;
  size_t size; /*used size*/
  size_t allocsize; /*allocated size*/
} ucvector;

static void ucvector_cleanup(void* p)
{
  ((ucvector*)p)->size = ((ucvector*)p)->allocsize = 0;
  free(((ucvector*)p)->data);
  ((ucvector*)p)->data = NULL;
}

/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned ucvector_resize(ucvector* p, size_t size)
{
  if(size * sizeof(unsigned char) > p->allocsize)
  {
    size_t newsize = size * sizeof(unsigned char) * 2;
    void* data = realloc(p->data, newsize);
    if(data)
    {
      p->allocsize = newsize;
      p->data = (unsigned char*)data;
      p->size = size;
    }
    else return 0; /*error: not enough memory*/
  }
  else p->size = size;
  return 1;
}

#ifdef LODEPNG_COMPILE_DECODER
#ifdef LODEPNG_COMPILE_PNG
/*resize and give all new elements the value*/
static unsigned ucvector_resizev(ucvector* p, size_t size, unsigned char value)
{
  size_t oldsize = p->size, i;
  if(!ucvector_resize(p, size)) return 0;
  for(i = oldsize; i < size; i++) p->data[i] = value;
  return 1;
}
#endif /*LODEPNG_COMPILE_PNG*/
#endif /*LODEPNG_COMPILE_DECODER*/

static void ucvector_init(ucvector* p)
{
  p->data = NULL;
  p->size = p->allocsize = 0;
}

#ifdef LODEPNG_COMPILE_ZLIB
/*you can both convert from vector to buffer&size and vica versa. If you use
init_buffer to take over a buffer and size, it is not needed to use cleanup*/
static void ucvector_init_buffer(ucvector* p, unsigned char* buffer, size_t size)
{
  p->data = buffer;
  p->allocsize = p->size = size;
}
#endif /*LODEPNG_COMPILE_ZLIB*/

#ifdef LODEPNG_COMPILE_ENCODER
/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned ucvector_push_back(ucvector* p, unsigned char c)
{
  if(!ucvector_resize(p, p->size + 1)) return 0;
  p->data[p->size - 1] = c;
  return 1;
}
#endif /*LODEPNG_COMPILE_ENCODER*/


/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_PNG
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned string_resize(char** out, size_t size)
{
  char* data = (char*)realloc(*out, size + 1);
  if(data)
  {
    data[size] = 0; /*null termination char*/
    *out = data;
  }
  return data != 0;
}

/*init a {char*, size_t} pair for use as string*/
static void string_init(char** out)
{
  *out = NULL;
  string_resize(out, 0);
}

/*free the above pair again*/
static void string_cleanup(char** out)
{
  free(*out);
  *out = NULL;
}

static void string_set(char** out, const char* in)
{
  size_t insize = strlen(in), i = 0;
  if(string_resize(out, insize))
  {
    for(i = 0; i < insize; i++)
    {
      (*out)[i] = in[i];
    }
  }
}
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
#endif /*LODEPNG_COMPILE_PNG*/

/* ////////////////////////////////////////////////////////////////////////// */

unsigned LodePNG_read32bitInt(const unsigned char* buffer)
{
  return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

/*buffer must have at least 4 allocated bytes available*/
static void LodePNG_set32bitInt(unsigned char* buffer, unsigned value)
{
  buffer[0] = (unsigned char)((value >> 24) & 0xff);
  buffer[1] = (unsigned char)((value >> 16) & 0xff);
  buffer[2] = (unsigned char)((value >>  8) & 0xff);
  buffer[3] = (unsigned char)((value      ) & 0xff);
}

#ifdef LODEPNG_COMPILE_ENCODER
static void LodePNG_add32bitInt(ucvector* buffer, unsigned value)
{
  ucvector_resize(buffer, buffer->size + 4); /*todo: give error if resize failed*/
  LodePNG_set32bitInt(&buffer->data[buffer->size - 4], value);
}
#endif /*LODEPNG_COMPILE_ENCODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* / File IO                                                                / */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_DISK

unsigned LodePNG_loadFile(unsigned char** out, size_t* outsize, const char* filename)
{
  FILE* file;
  long size;

  /*provide some proper output values if error will happen*/
  *out = 0;
  *outsize = 0;

  file = fopen(filename, "rb");
  if(!file) return 78;

  /*get filesize:*/
  fseek(file , 0 , SEEK_END);
  size = ftell(file);
  rewind(file);

  /*read contents of the file into the vector*/
  *outsize = 0;
  *out = (unsigned char*)malloc((size_t)size);
  if(size && (*out)) (*outsize) = fread(*out, 1, (size_t)size, file);

  fclose(file);
  if(!(*out) && size) return 9900; /*the above malloc failed*/
  return 0;
}

/*write given buffer to the file, overwriting the file, it doesn't append to it.*/
unsigned LodePNG_saveFile(const unsigned char* buffer, size_t buffersize, const char* filename)
{
  FILE* file;
  file = fopen(filename, "wb" );
  if(!file) return 79;
  fwrite((char*)buffer , 1 , buffersize, file);
  fclose(file);
  return 0;
}

#endif /*LODEPNG_COMPILE_DISK*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // End of common code and tools. Begin of Zlib related code.            // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_ZLIB

/* ////////////////////////////////////////////////////////////////////////// */
/* / Reading and writing single bits and bytes from/to stream for Deflate   / */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_ENCODER
/*TODO: this ignores potential out of memory errors*/
static void addBitToStream(size_t* bitpointer, ucvector* bitstream, unsigned char bit)
{
  /*add a new byte at the end*/
  if((*bitpointer) % 8 == 0) ucvector_push_back(bitstream, (unsigned char)0);
  /*earlier bit of huffman code is in a lesser significant bit of an earlier byte*/
  (bitstream->data[bitstream->size - 1]) |= (bit << ((*bitpointer) & 0x7));
  (*bitpointer)++;
}

static void addBitsToStream(size_t* bitpointer, ucvector* bitstream, unsigned value, size_t nbits)
{
  size_t i;
  for(i = 0; i < nbits; i++) addBitToStream(bitpointer, bitstream, (unsigned char)((value >> i) & 1));
}

static void addBitsToStreamReversed(size_t* bitpointer, ucvector* bitstream, unsigned value, size_t nbits)
{
  size_t i;
  for(i = 0; i < nbits; i++) addBitToStream(bitpointer, bitstream, (unsigned char)((value >> (nbits - 1 - i)) & 1));
}
#endif /*LODEPNG_COMPILE_ENCODER*/

#ifdef LODEPNG_COMPILE_DECODER

#define READBIT(bitpointer, bitstream) ((bitstream[bitpointer >> 3] >> (bitpointer & 0x7)) & (unsigned char)1)

static unsigned char readBitFromStream(size_t* bitpointer, const unsigned char* bitstream)
{
  unsigned char result = (unsigned char)(READBIT(*bitpointer, bitstream));
  (*bitpointer)++;
  return result;
}

static unsigned readBitsFromStream(size_t* bitpointer, const unsigned char* bitstream, size_t nbits)
{
  unsigned result = 0, i;
  for(i = 0; i < nbits; i++)
  {
    result += ((unsigned)READBIT(*bitpointer, bitstream)) << i;
    (*bitpointer)++;
  }
  return result;
}
#endif /*LODEPNG_COMPILE_DECODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* / Deflate - Huffman                                                      / */
/* ////////////////////////////////////////////////////////////////////////// */

#define FIRST_LENGTH_CODE_INDEX 257
#define LAST_LENGTH_CODE_INDEX 285
/*256 literals, the end code, some length codes, and 2 unused codes*/
#define NUM_DEFLATE_CODE_SYMBOLS 288
/*the distance codes have their own symbols, 30 used, 2 unused*/
#define NUM_DISTANCE_SYMBOLS 32
/*the code length codes. 0-15: code lengths, 16: copy previous 3-6 times, 17: 3-10 zeros, 18: 11-138 zeros*/
#define NUM_CODE_LENGTH_CODES 19

/*the base lengths represented by codes 257-285*/
static const unsigned LENGTHBASE[29]
  = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
     67, 83, 99, 115, 131, 163, 195, 227, 258};

/*the extra bits used by codes 257-285 (added to base length)*/
static const unsigned LENGTHEXTRA[29]
  = {0, 0, 0, 0, 0, 0, 0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,
      4,  4,  4,   4,   5,   5,   5,   5,   0};

/*the base backwards distances (the bits of distance codes appear after length codes and use their own huffman tree)*/
static const unsigned DISTANCEBASE[30]
  = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
     769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

/*the extra bits of backwards distances (added to base)*/
static const unsigned DISTANCEEXTRA[30]
  = {0, 0, 0, 0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5,   6,   6,   7,   7,   8,
       8,    9,    9,   10,   10,   11,   11,   12,    12,    13,    13};

/*the order in which "code length alphabet code lengths" are stored, out of this
the huffman tree of the dynamic huffman tree lengths is generated*/
static const unsigned CLCL_ORDER[NUM_CODE_LENGTH_CODES]
  = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/* ////////////////////////////////////////////////////////////////////////// */

/*
Huffman tree struct, containing multiple representations of the tree
*/
typedef struct HuffmanTree
{
  uivector tree2d;
  uivector tree1d;
  uivector lengths; /*the lengths of the codes of the 1d-tree*/
  unsigned maxbitlen; /*maximum number of bits a single code can get*/
  unsigned numcodes; /*number of symbols in the alphabet = number of codes*/
} HuffmanTree;

/*function used for debug purposes to draw the tree in ascii art with C++*/
/*#include <iostream>
static void HuffmanTree_draw(HuffmanTree* tree)
{
  std::cout << "tree. length: " << tree->numcodes << " maxbitlen: " << tree->maxbitlen << std::endl;
  for(size_t i = 0; i < tree->tree1d.size; i++)
  {
    if(tree->lengths.data[i])
      std::cout << i << " " << tree->tree1d.data[i] << " " << tree->lengths.data[i] << std::endl;
  }
  std::cout << std::endl;
}*/

static void HuffmanTree_init(HuffmanTree* tree)
{
  uivector_init(&tree->tree2d);
  uivector_init(&tree->tree1d);
  uivector_init(&tree->lengths);
}

static void HuffmanTree_cleanup(HuffmanTree* tree)
{
  uivector_cleanup(&tree->tree2d);
  uivector_cleanup(&tree->tree1d);
  uivector_cleanup(&tree->lengths);
}

/*the tree representation used by the decoder. return value is error*/
static unsigned HuffmanTree_make2DTree(HuffmanTree* tree)
{
  unsigned nodefilled = 0; /*up to which node it is filled*/
  unsigned treepos = 0; /*position in the tree (1 of the numcodes columns)*/
  unsigned n, i;

  if(!uivector_resize(&tree->tree2d, tree->numcodes * 2)) return 9901; /*alloc fail*/

  /*
  convert tree1d[] to tree2d[][]. In the 2D array, a value of 32767 means
  uninited, a value >= numcodes is an address to another bit, a value < numcodes
  is a code. The 2 rows are the 2 possible bit values (0 or 1), there are as
  many columns as codes - 1.
  A good huffmann tree has N * 2 - 1 nodes, of which N - 1 are internal nodes.
  Here, the internal nodes are stored (what their 0 and 1 option point to).
  There is only memory for such good tree currently, if there are more nodes
  (due to too long length codes), error 55 will happen
  */
  for(n = 0;  n < tree->numcodes * 2; n++)
  {
    tree->tree2d.data[n] = 32767; /*32767 here means the tree2d isn't filled there yet*/
  }

  for(n = 0; n < tree->numcodes; n++) /*the codes*/
  {
    for(i = 0; i < tree->lengths.data[n]; i++) /*the bits for this code*/
    {
      unsigned char bit = (unsigned char)((tree->tree1d.data[n] >> (tree->lengths.data[n] - i - 1)) & 1);
      if(treepos > tree->numcodes - 2) return 55; /*error 55: oversubscribed; see description in header*/
      if(tree->tree2d.data[2 * treepos + bit] == 32767) /*not yet filled in*/
      {
        if(i + 1 == tree->lengths.data[n]) /*last bit*/
        {
          tree->tree2d.data[2 * treepos + bit] = n; /*put the current code in it*/
          treepos = 0;
        }
        else
        {
          /*put address of the next step in here, first that address has to be found of course
          (it's just nodefilled + 1)...*/
          nodefilled++;
          /*addresses encoded with numcodes added to it*/
          tree->tree2d.data[2 * treepos + bit] = nodefilled + tree->numcodes;
          treepos = nodefilled;
        }
      }
      else treepos = tree->tree2d.data[2 * treepos + bit] - tree->numcodes;
    }
  }

  for(n = 0;  n < tree->numcodes * 2; n++)
  {
    if(tree->tree2d.data[n] == 32767) tree->tree2d.data[n] = 0; /*remove possible remaining 32767's*/
  }

  return 0;
}

/*
Second step for the ...makeFromLengths and ...makeFromFrequencies functions.
numcodes, lengths and maxbitlen must already be filled in correctly. return
value is error.
*/
static unsigned HuffmanTree_makeFromLengths2(HuffmanTree* tree)
{
  uivector blcount;
  uivector nextcode;
  unsigned bits, n, error = 0;

  uivector_init(&blcount);
  uivector_init(&nextcode);
  if(!uivector_resize(&tree->tree1d, tree->numcodes)
  || !uivector_resizev(&blcount, tree->maxbitlen + 1, 0)
  || !uivector_resizev(&nextcode, tree->maxbitlen + 1, 0))
    error = 9902; /*alloc fail*/

  if(!error)
  {
    /*step 1: count number of instances of each code length*/
    for(bits = 0; bits < tree->numcodes; bits++) blcount.data[tree->lengths.data[bits]]++;
    /*step 2: generate the nextcode values*/
    for(bits = 1; bits <= tree->maxbitlen; bits++)
    {
      nextcode.data[bits] = (nextcode.data[bits - 1] + blcount.data[bits - 1]) << 1;
    }
    /*step 3: generate all the codes*/
    for(n = 0; n < tree->numcodes; n++)
    {
      if(tree->lengths.data[n] != 0) tree->tree1d.data[n] = nextcode.data[tree->lengths.data[n]]++;
    }
  }

  uivector_cleanup(&blcount);
  uivector_cleanup(&nextcode);

  if(!error) return HuffmanTree_make2DTree(tree);
  else return error;
}

/*
given the code lengths (as stored in the PNG file), generate the tree as defined
by Deflate. maxbitlen is the maximum bits that a code in the tree can have.
return value is error.
*/
static unsigned HuffmanTree_makeFromLengths(HuffmanTree* tree, const unsigned* bitlen,
                                            size_t numcodes, unsigned maxbitlen)
{
  unsigned i;
  if(!uivector_resize(&tree->lengths, numcodes)) return 9903; /*alloc fail*/
  for(i = 0; i < numcodes; i++) tree->lengths.data[i] = bitlen[i];
  tree->numcodes = (unsigned)numcodes; /*number of symbols*/
  tree->maxbitlen = maxbitlen;
  return HuffmanTree_makeFromLengths2(tree);
}

#ifdef LODEPNG_COMPILE_ENCODER

/*
A coin, this is the terminology used for the package-merge algorithm and the
coin collector's problem. This is used to generate the huffman tree.
A coin can be multiple coins (when they're merged)
*/
typedef struct Coin
{
  uivector symbols;
  float weight; /*the sum of all weights in this coin*/
} Coin;

static void Coin_init(Coin* c)
{
  uivector_init(&c->symbols);
}

/*argument c is void* so that this dtor can be given as function pointer to the vector resize function*/
static void Coin_cleanup(void* c)
{
  uivector_cleanup(&((Coin*)c)->symbols);
}

static void Coin_copy(Coin* c1, const Coin* c2)
{
  c1->weight = c2->weight;
  uivector_copy(&c1->symbols, &c2->symbols);
}

static void addCoins(Coin* c1, const Coin* c2)
{
  size_t i;
  for(i = 0; i < c2->symbols.size; i++) uivector_push_back(&c1->symbols, c2->symbols.data[i]);
  c1->weight += c2->weight;
}

/*
Coin_sort: This uses a simple combsort to sort the data. This function is not critical for
overall encoding speed and the data amount isn't that large.
*/
static void Coin_sort(Coin* data, size_t amount)
{
  size_t gap = amount;
  unsigned char swapped = 0;
  while((gap > 1) || swapped)
  {
    size_t i;
    gap = (gap * 10) / 13; /*shrink factor 1.3*/
    if(gap == 9 || gap == 10) gap = 11; /*combsort11*/
    if(gap < 1) gap = 1;
    swapped = 0;
    for(i = 0; i < amount - gap; i++)
    {
      size_t j = i + gap;
      if(data[j].weight < data[i].weight)
      {
        float temp = data[j].weight; data[j].weight = data[i].weight; data[i].weight = temp;
        uivector_swap(&data[i].symbols, &data[j].symbols);
        swapped = 1;
      }
    }
  }
}

static unsigned HuffmanTree_fillInCoins(vector* coins, const unsigned* frequencies, unsigned numcodes, size_t sum)
{
  unsigned i;
  for(i = 0; i < numcodes; i++)
  {
    Coin* coin;
    if(frequencies[i] == 0) continue; /*it's important to exclude symbols that aren't present*/
    if(!vector_resize(coins, coins->size + 1))
    {
      vector_cleanup(coins);
      return 9904; /*alloc fail*/
    }
    coin = (Coin*)(vector_get(coins, coins->size - 1));
    Coin_init(coin);
    coin->weight = frequencies[i] / (float)sum;
    uivector_push_back(&coin->symbols, i);
  }
  if(coins->size) Coin_sort((Coin*)coins->data, coins->size);
  return 0;
}

/*Create the Huffman tree given the symbol frequencies*/
static unsigned HuffmanTree_makeFromFrequencies(HuffmanTree* tree, const unsigned* frequencies,
                                                size_t numcodes, unsigned maxbitlen)
{
  unsigned i, j;
  size_t sum = 0, numpresent = 0;
  unsigned error = 0;

  vector prev_row; /*type Coin, the previous row of coins*/
  vector coins; /*type Coin, the coins of the currently calculated row*/

  tree->maxbitlen = maxbitlen;

  for(i = 0; i < numcodes; i++)
  {
    if(frequencies[i] > 0)
    {
      numpresent++;
      sum += frequencies[i];
    }
  }

  if(numcodes == 0) return 80; /*error: a tree of 0 symbols is not supposed to be made*/
  tree->numcodes = (unsigned)numcodes; /*number of symbols*/
  uivector_resize(&tree->lengths, 0);
  if(!uivector_resizev(&tree->lengths, tree->numcodes, 0)) return 9905; /*alloc fail*/

  /*there are no symbols at all, in that case add one symbol of value 0 to the tree (see RFC 1951 section 3.2.7) */
  if(numpresent == 0)
  {
    tree->lengths.data[0] = 1;
    return HuffmanTree_makeFromLengths2(tree);
  }
  /*the package merge algorithm gives wrong results if there's only one symbol
  (theoretically 0 bits would then suffice, but we need a proper symbol for zlib)*/
  else if(numpresent == 1)
  {
    for(i = 0; i < numcodes; i++) if(frequencies[i]) tree->lengths.data[i] = 1;
    return HuffmanTree_makeFromLengths2(tree);
  }

  vector_init(&coins, sizeof(Coin));
  vector_init(&prev_row, sizeof(Coin));

  /*Package-Merge algorithm represented by coin collector's problem
  For every symbol, maxbitlen coins will be created*/

  /*first row, lowest denominator*/
  error = HuffmanTree_fillInCoins(&coins, frequencies, tree->numcodes, sum);
  if(!error)
  {
    for(j = 1; j <= maxbitlen && !error; j++) /*each of the remaining rows*/
    {
      vector_swap(&coins, &prev_row); /*swap instead of copying*/
      if(!vector_resized(&coins, 0, Coin_cleanup)) ERROR_BREAK(9906 /*alloc fail*/);
      for(i = 0; i + 1 < prev_row.size; i += 2)
      {
        if(!vector_resize(&coins, coins.size + 1)) ERROR_BREAK(9907 /*alloc fail*/);
        Coin_init((Coin*)vector_get(&coins, coins.size - 1));
        Coin_copy((Coin*)vector_get(&coins, coins.size - 1), (Coin*)vector_get(&prev_row, i));
        /*merge the coins into packages*/
        addCoins((Coin*)vector_get(&coins, coins.size - 1), (Coin*)vector_get(&prev_row, i + 1));
      }
      if(j < maxbitlen)
      {
        error = HuffmanTree_fillInCoins(&coins, frequencies, tree->numcodes, sum);
      }
    }
  }

  if(!error)
  {
    /*keep the coins with lowest weight, so that they add up to the amount of symbols - 1*/
    vector_resized(&coins, numpresent - 1, Coin_cleanup);

    /*calculate the lenghts of each symbol, as the amount of times a coin of each symbol is used*/
    for(i = 0; i < coins.size; i++)
    {
      Coin* coin = (Coin*)vector_get(&coins, i);
      for(j = 0; j < coin->symbols.size; j++) tree->lengths.data[coin->symbols.data[j]]++;
    }

    error = HuffmanTree_makeFromLengths2(tree);
  }

  vector_cleanupd(&coins, Coin_cleanup);
  vector_cleanupd(&prev_row, Coin_cleanup);

  return error;
}

static unsigned HuffmanTree_getCode(const HuffmanTree* tree, unsigned index)
{
  return tree->tree1d.data[index];
}

static unsigned HuffmanTree_getLength(const HuffmanTree* tree, unsigned index)
{
  return tree->lengths.data[index];
}
#endif /*LODEPNG_COMPILE_ENCODER*/

/*get the literal and length code tree of a deflated block with fixed tree, as per the deflate specification*/
static unsigned generateFixedLitLenTree(HuffmanTree* tree)
{
  unsigned i, error = 0;
  uivector bitlen;
  uivector_init(&bitlen);
  if(!uivector_resize(&bitlen, NUM_DEFLATE_CODE_SYMBOLS)) error = 9909; /*alloc fail*/

  if(!error)
  {
    /*288 possible codes: 0-255=literals, 256=endcode, 257-285=lengthcodes, 286-287=unused*/
    for(i =   0; i <= 143; i++) bitlen.data[i] = 8;
    for(i = 144; i <= 255; i++) bitlen.data[i] = 9;
    for(i = 256; i <= 279; i++) bitlen.data[i] = 7;
    for(i = 280; i <= 287; i++) bitlen.data[i] = 8;

    error = HuffmanTree_makeFromLengths(tree, bitlen.data, NUM_DEFLATE_CODE_SYMBOLS, 15);
  }

  uivector_cleanup(&bitlen);
  return error;
}

/*get the distance code tree of a deflated block with fixed tree, as specified in the deflate specification*/
static unsigned generateFixedDistanceTree(HuffmanTree* tree)
{
  unsigned i, error = 0;
  uivector bitlen;
  uivector_init(&bitlen);
  if(!uivector_resize(&bitlen, NUM_DISTANCE_SYMBOLS)) error = 9910; /*alloc fail*/

  /*there are 32 distance codes, but 30-31 are unused*/
  if(!error)
  {
    for(i = 0; i < NUM_DISTANCE_SYMBOLS; i++) bitlen.data[i] = 5;
    error = HuffmanTree_makeFromLengths(tree, bitlen.data, NUM_DISTANCE_SYMBOLS, 15);
  }
  uivector_cleanup(&bitlen);
  return error;
}

#ifdef LODEPNG_COMPILE_DECODER

/*
returns the code, or (unsigned)(-1) if error happened
inbitlength is the length of the complete buffer, in bits (so its byte length times 8)
*/
static unsigned huffmanDecodeSymbol(const unsigned char* in, size_t* bp,
                                    const HuffmanTree* codetree, size_t inbitlength)
{
  unsigned treepos = 0, ct;
  for(;;)
  {
    if(*bp > inbitlength) return (unsigned)(-1); /*error: end of input memory reached without endcode*/

    /*
    decode the symbol from the tree. The "readBitFromStream" code is inlined in
    the expression below because this is the biggest bottleneck while decoding
    */
    ct = codetree->tree2d.data[(treepos << 1) + READBIT(*bp, in)];
    (*bp)++;
    if(ct < codetree->numcodes) return ct; /*the symbol is decoded, return it*/
    else treepos = ct - codetree->numcodes; /*symbol not yet decoded, instead move tree position*/

    if(treepos >= codetree->numcodes) return (unsigned)(-1); /*error: it appeared outside the codetree*/
  }
}
#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_DECODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / Inflator (Decompressor)                                                / */
/* ////////////////////////////////////////////////////////////////////////// */

/*get the tree of a deflated block with fixed tree, as specified in the deflate specification*/
static void getTreeInflateFixed(HuffmanTree* tree_ll, HuffmanTree* tree_d)
{
  /*error checking not done, this is fixed stuff, it works, it doesn't depend on the image*/
  /*TODO: out of memory errors could still happen...*/
  generateFixedLitLenTree(tree_ll);
  generateFixedDistanceTree(tree_d);
}

/*get the tree of a deflated block with dynamic tree, the tree itself is also Huffman compressed with a known tree*/
static unsigned getTreeInflateDynamic(HuffmanTree* tree_ll, HuffmanTree* tree_d,
                                      const unsigned char* in, size_t* bp, size_t inlength)
{
  /*make sure that length values that aren't filled in will be 0, or a wrong tree will be generated*/
  unsigned error = 0;
  unsigned n, HLIT, HDIST, HCLEN, i;
  size_t inbitlength = inlength * 8;

  /*see comments in deflateDynamic for explanation of the context and these variables, it is analogous*/
  uivector bitlen_ll; /*lit,len code lengths*/
  uivector bitlen_d; /*dist code lengths*/
  /*code length code lengths ("clcl"), the bit lengths of the huffman tree used to compress bitlen_ll and bitlen_d*/
  uivector bitlen_cl;
  HuffmanTree tree_cl; /*the code tree for code length codes (the huffman tree for compressed huffman trees)*/

  if((*bp) >> 3 >= inlength - 2) return 49; /*the bit pointer is or will go past the memory*/

  /*number of literal/length codes + 257. Unlike the spec, the value 257 is added to it here already*/
  HLIT =  readBitsFromStream(bp, in, 5) + 257;
  /*number of distance codes. Unlike the spec, the value 1 is added to it here already*/
  HDIST = readBitsFromStream(bp, in, 5) + 1;
  /*number of code length codes. Unlike the spec, the value 4 is added to it here already*/
  HCLEN = readBitsFromStream(bp, in, 4) + 4;

  HuffmanTree_init(&tree_cl);
  uivector_init(&bitlen_ll);
  uivector_init(&bitlen_d);
  uivector_init(&bitlen_cl);

  while(!error)
  {
    /*read the code length codes out of 3 * (amount of code length codes) bits*/
    
    if(!uivector_resize(&bitlen_cl, NUM_CODE_LENGTH_CODES)) ERROR_BREAK(9911);

    for(i = 0; i < NUM_CODE_LENGTH_CODES; i++)
    {
      if(i < HCLEN) bitlen_cl.data[CLCL_ORDER[i]] = readBitsFromStream(bp, in, 3);
      else bitlen_cl.data[CLCL_ORDER[i]] = 0; /*if not, it must stay 0*/
    }

    error = HuffmanTree_makeFromLengths(&tree_cl, bitlen_cl.data, bitlen_cl.size, 7);
    if(error) break;

    /*now we can use this tree to read the lengths for the tree that this function will return*/
    uivector_resizev(&bitlen_ll, NUM_DEFLATE_CODE_SYMBOLS, 0);
    uivector_resizev(&bitlen_d, NUM_DISTANCE_SYMBOLS, 0);
    i = 0;
    if(!bitlen_ll.data || !bitlen_d.data) ERROR_BREAK(9912); /*alloc fail*/

    /*i is the current symbol we're reading in the part that contains the code lengths of lit/len and dist codes*/
    while(i < HLIT + HDIST)
    {
      unsigned code = huffmanDecodeSymbol(in, bp, &tree_cl, inbitlength);
      if(code <= 15) /*a length code*/
      {
        if(i < HLIT) bitlen_ll.data[i] = code;
        else bitlen_d.data[i - HLIT] = code;
        i++;
      }
      else if(code == 16) /*repeat previous*/
      {
        unsigned replength = 3; /*read in the 2 bits that indicate repeat length (3-6)*/
        unsigned value; /*set value to the previous code*/

        if((*bp) >> 3 >= inlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/

        replength += readBitsFromStream(bp, in, 2);

        if((i - 1) < HLIT) value = bitlen_ll.data[i - 1];
        else value = bitlen_d.data[i - HLIT - 1];
        /*repeat this value in the next lengths*/
        for(n = 0; n < replength; n++)
        {
          if(i >= HLIT + HDIST) ERROR_BREAK(13); /*error: i is larger than the amount of codes*/
          if(i < HLIT) bitlen_ll.data[i] = value;
          else bitlen_d.data[i - HLIT] = value;
          i++;
        }
      }
      else if(code == 17) /*repeat "0" 3-10 times*/
      {
        unsigned replength = 3; /*read in the bits that indicate repeat length*/
        if((*bp) >> 3 >= inlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/

        replength += readBitsFromStream(bp, in, 3);

        /*repeat this value in the next lengths*/
        for(n = 0; n < replength; n++)
        {
          if(i >= HLIT + HDIST) ERROR_BREAK(14); /*error: i is larger than the amount of codes*/

          if(i < HLIT) bitlen_ll.data[i] = 0;
          else bitlen_d.data[i - HLIT] = 0;
          i++;
        }
      }
      else if(code == 18) /*repeat "0" 11-138 times*/
      {
        unsigned replength = 11; /*read in the bits that indicate repeat length*/
        if((*bp) >> 3 >= inlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/

        replength += readBitsFromStream(bp, in, 7);

        /*repeat this value in the next lengths*/
        for(n = 0; n < replength; n++)
        {
          if(i >= HLIT + HDIST) ERROR_BREAK(15); /*error: i is larger than the amount of codes*/

          if(i < HLIT) bitlen_ll.data[i] = 0;
          else bitlen_d.data[i - HLIT] = 0;
          i++;
        }
      }
      else /*if(code == (unsigned)(-1))*/ /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
      {
        if(code == (unsigned)(-1))
        {
          /*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
          (10=no endcode, 11=wrong jump outside of tree)*/
          error = (*bp) > inlength * 8 ? 10 : 11;
        }
        else error = 16; /*unexisting code, this can never happen*/
        break;
      }
    }
    if(error) break;

    if(bitlen_ll.data[256] == 0) ERROR_BREAK(64); /*the length of the end code 256 must be larger than 0*/

    /*now we've finally got HLIT and HDIST, so generate the code trees, and the function is done*/
    error = HuffmanTree_makeFromLengths(tree_ll, &bitlen_ll.data[0], bitlen_ll.size, 15);
    if(error) break;
    error = HuffmanTree_makeFromLengths(tree_d, &bitlen_d.data[0], bitlen_d.size, 15);

    break; /*end of error-while*/
  }

  uivector_cleanup(&bitlen_cl);
  uivector_cleanup(&bitlen_ll);
  uivector_cleanup(&bitlen_d);
  HuffmanTree_cleanup(&tree_cl);

  return error;
}

/*inflate a block with dynamic of fixed Huffman tree*/
static unsigned inflateHuffmanBlock(ucvector* out, const unsigned char* in, size_t* bp,
                                    size_t* pos, size_t inlength, unsigned btype)
{
  unsigned error = 0;
  HuffmanTree tree_ll; /*the huffman tree for literal and length codes*/
  HuffmanTree tree_d; /*the huffman tree for distance codes*/
  size_t inbitlength = inlength * 8;

  HuffmanTree_init(&tree_ll);
  HuffmanTree_init(&tree_d);

  if(btype == 1) getTreeInflateFixed(&tree_ll, &tree_d);
  else if(btype == 2)
  {
    error = getTreeInflateDynamic(&tree_ll, &tree_d, in, bp, inlength);
  }

  for(;;) /*decode all symbols until end reached*/
  {
    /*code_ll is literal, length or end code*/
    unsigned code_ll = huffmanDecodeSymbol(in, bp, &tree_ll, inbitlength);
    if(code_ll <= 255) /*literal symbol*/
    {
      if((*pos) >= out->size)
      {
        /*reserve more room at once*/
        if(!ucvector_resize(out, ((*pos) + 1) * 2)) ERROR_BREAK(9913 /*alloc fail*/);
      }
      out->data[(*pos)] = (unsigned char)(code_ll);
      (*pos)++;
    }
    else if(code_ll >= FIRST_LENGTH_CODE_INDEX && code_ll <= LAST_LENGTH_CODE_INDEX) /*length code*/
    {
      unsigned code_d, distance;
      unsigned numextrabits_l, numextrabits_d; /*extra bits for length and distance*/
      size_t start, forward, backward, length;
      
      /*part 1: get length base*/
      length = LENGTHBASE[code_ll - FIRST_LENGTH_CODE_INDEX];

      /*part 2: get extra bits and add the value of that to length*/
      numextrabits_l = LENGTHEXTRA[code_ll - FIRST_LENGTH_CODE_INDEX];
      if(((*bp) >> 3) >= inlength) ERROR_BREAK(51); /*error, bit pointer will jump past memory*/
      length += readBitsFromStream(bp, in, numextrabits_l);

      /*part 3: get distance code*/
      code_d = huffmanDecodeSymbol(in, bp, &tree_d, inbitlength);
      if(code_d > 29)
      {
        if(code_ll == (unsigned)(-1)) /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
        {
          /*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
          (10=no endcode, 11=wrong jump outside of tree)*/
          error = (*bp) > inlength * 8 ? 10 : 11;
        }
        else error = 18; /*error: invalid distance code (30-31 are never used)*/
        break;
      }
      distance = DISTANCEBASE[code_d];

      /*part 4: get extra bits from distance*/
      numextrabits_d = DISTANCEEXTRA[code_d];
      if(((*bp) >> 3) >= inlength) ERROR_BREAK(51); /*error, bit pointer will jump past memory*/

      distance += readBitsFromStream(bp, in, numextrabits_d);

      /*part 5: fill in all the out[n] values based on the length and dist*/
      start = (*pos);
      backward = start - distance;
      if((*pos) + length >= out->size)
      {
        /*reserve more room at once*/
        if(!ucvector_resize(out, ((*pos) + length) * 2)) ERROR_BREAK(9914 /*alloc fail*/);
      }

      for(forward = 0; forward < length; forward++)
      {
        out->data[(*pos)] = out->data[backward];
        (*pos)++;
        backward++;
        if(backward >= start) backward = start - distance;
      }
    }
    else if(code_ll == 256)
    {
      break; /*end code, break the loop*/
    }
    else /*if(code == (unsigned)(-1))*/ /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
    {
      /*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
      (10=no endcode, 11=wrong jump outside of tree)*/
      error = (*bp) > inlength * 8 ? 10 : 11;
      break;
    }
  }

  HuffmanTree_cleanup(&tree_ll);
  HuffmanTree_cleanup(&tree_d);

  return error;
}

static unsigned inflateNoCompression(ucvector* out, const unsigned char* in, size_t* bp, size_t* pos, size_t inlength)
{
  /*go to first boundary of byte*/
  size_t p;
  unsigned LEN, NLEN, n, error = 0;
  while(((*bp) & 0x7) != 0) (*bp)++;
  p = (*bp) / 8; /*byte position*/

  /*read LEN (2 bytes) and NLEN (2 bytes)*/
  if(p >= inlength - 4) return 52; /*error, bit pointer will jump past memory*/
  LEN = in[p] + 256 * in[p + 1]; p += 2;
  NLEN = in[p] + 256 * in[p + 1]; p += 2;

  /*check if 16-bit NLEN is really the one's complement of LEN*/
  if(LEN + NLEN != 65535) return 21; /*error: NLEN is not one's complement of LEN*/

  if((*pos) + LEN >= out->size)
  {
    if(!ucvector_resize(out, (*pos) + LEN)) return 9915; /*alloc fail*/
  }

  /*read the literal data: LEN bytes are now stored in the out buffer*/
  if(p + LEN > inlength) return 23; /*error: reading outside of in buffer*/
  for(n = 0; n < LEN; n++) out->data[(*pos)++] = in[p++];

  (*bp) = p * 8;

  return error;
}

/*inflate the deflated data (cfr. deflate spec); return value is the error*/
static unsigned LodePNG_inflate(ucvector* out, const unsigned char* in, size_t insize, size_t inpos)
{
  /*bit pointer in the "in" data, current byte is bp >> 3, current bit is bp & 0x7 (from lsb to msb of the byte)*/
  size_t bp = 0;
  unsigned BFINAL = 0;
  size_t pos = 0; /*byte position in the out buffer*/

  unsigned error = 0;

  while(!BFINAL)
  {
    unsigned BTYPE;
    if(bp + 2 >= insize * 8) return 52; /*error, bit pointer will jump past memory*/
    BFINAL = readBitFromStream(&bp, &in[inpos]);
    BTYPE = 1 * readBitFromStream(&bp, &in[inpos]);
    BTYPE += 2 * readBitFromStream(&bp, &in[inpos]);

    if(BTYPE == 3) return 20; /*error: invalid BTYPE*/
    else if(BTYPE == 0) error = inflateNoCompression(out, &in[inpos], &bp, &pos, insize); /*no compression*/
    else error = inflateHuffmanBlock(out, &in[inpos], &bp, &pos, insize, BTYPE); /*compression, BTYPE 01 or 10*/

    if(error) return error;
  }
  
  /*Only now we know the true size of out, resize it to that*/
  if(!ucvector_resize(out, pos)) error = 9916; /*alloc fail*/

  return error;
}

#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / Deflator (Compressor)                                                  / */
/* ////////////////////////////////////////////////////////////////////////// */

static const size_t MAX_SUPPORTED_DEFLATE_LENGTH = 258;

/*bitlen is the size in bits of the code*/
static void addHuffmanSymbol(size_t* bp, ucvector* compressed, unsigned code, unsigned bitlen)
{
  addBitsToStreamReversed(bp, compressed, code, bitlen);
}

/*search the index in the array, that has the largest value smaller than or equal to the given value,
given array must be sorted (if no value is smaller, it returns the size of the given array)*/
static size_t searchCodeIndex(const unsigned* array, size_t array_size, size_t value)
{
  /*linear search implementation*/
  /*for(size_t i = 1; i < array_size; i++) if(array[i] > value) return i - 1;
  return array_size - 1;*/

  /*binary search implementation (not that much faster) (precondition: array_size > 0)*/
  size_t left  = 1;
  size_t right = array_size - 1;
  while(left <= right)
  {
    size_t mid = (left + right) / 2;
    if(array[mid] <= value) left = mid + 1; /*the value to find is more to the right*/
    else if(array[mid - 1] > value) right = mid - 1; /*the value to find is more to the left*/
    else return mid - 1;
  }
  return array_size - 1;
}

static void addLengthDistance(uivector* values, size_t length, size_t distance)
{
  /*values in encoded vector are those used by deflate:
  0-255: literal bytes
  256: end
  257-285: length/distance pair (length code, followed by extra length bits, distance code, extra distance bits)
  286-287: invalid*/

  unsigned length_code = (unsigned)searchCodeIndex(LENGTHBASE, 29, length);
  unsigned extra_length = (unsigned)(length - LENGTHBASE[length_code]);
  unsigned dist_code = (unsigned)searchCodeIndex(DISTANCEBASE, 30, distance);
  unsigned extra_distance = (unsigned)(distance - DISTANCEBASE[dist_code]);

  uivector_push_back(values, length_code + FIRST_LENGTH_CODE_INDEX);
  uivector_push_back(values, extra_length);
  uivector_push_back(values, dist_code);
  uivector_push_back(values, extra_distance);
}

#if 0
/*the "brute force" version of the encodeLZ7 algorithm, not used anymore, kept here for reference*/
static unsigned encodeLZ77_brute(uivector* out, const unsigned char* in, size_t insize, unsigned windowSize)
{
  size_t pos;
  for(pos = 0; pos < insize; pos++)
  {
    size_t length = 0, offset = 0; /*the length and offset found for the current position*/
    size_t max_offset = pos < windowSize ? pos : windowSize; /*how far back to test*/
    size_t current_offset;

    /**search for the longest string, backwards through all possible distances (=offsets)**/
    for(current_offset = 1; current_offset < max_offset; current_offset++)
    {
      size_t backpos = pos - current_offset;
      if(in[backpos] == in[pos])
      {
        /*test the next characters*/
        size_t current_length = 1;
        size_t backtest = backpos + 1;
        size_t foretest = pos + 1;
        /*maximum supporte length by deflate is max length*/
        while(foretest < insize && in[backtest] == in[foretest] && current_length < MAX_SUPPORTED_DEFLATE_LENGTH)
        {
          if(backpos >= pos)
          {
            /*continue as if we work on the decoded bytes after pos by jumping back before pos*/
            backpos -= current_offset;
          }
          current_length++;
          backtest++;
          foretest++;
        }
        if(current_length > length)
        {
          length = current_length; /*the longest length*/
          offset = current_offset; /*the offset that is related to this longest length*/
          /*you can jump out of this for loop once a length of max length is found (gives significant speed gain)*/
          if(current_length == MAX_SUPPORTED_DEFLATE_LENGTH) break;
        }
      }
    }

    /**encode it as length/distance pair or literal value**/
    if(length < 3) /*only lengths of 3 or higher are supported as length/distance pair*/
    {
      uivector_push_back(out, in[pos]);
    }
    else
    {
      addLengthDistance(out, length, offset);
      pos += (length - 1);
    }
  } /*end of the loop through each character of input*/
  return 0;
}
#endif

static const unsigned HASH_NUM_VALUES = 2048;
static const unsigned HASH_NUM_CHARACTERS = 3;
static const unsigned HASH_SHIFT = 2;
/*
The HASH_NUM_CHARACTERS value is used to make encoding faster by using longer
sequences to generate a hash value from the stream bytes. Setting it to 3
gives exactly the same compression as the brute force method, since deflate's
run length encoding starts with lengths of 3. Setting it to higher values,
like 6, can make the encoding faster (not always though!), but will cause the
encoding to miss any length between 3 and this value, so that the compression
may be worse (but this can vary too depending on the image, sometimes it is
even a bit better instead).
The HASH_NUM_VALUES is the amount of unique possible hash values that
combinations of bytes can give, the higher it is the more memory is needed, but
if it's too low the advantage of hashing is gone.
*/

static unsigned getHash(const unsigned char* data, size_t size, size_t pos, size_t num)
{
  unsigned result = 0;
  size_t amount, i;
  if(pos >= size) return 0;
  amount = num;
  if(pos + amount >= size) amount = size - pos;
  for(i = 0; i < amount; i++) result ^= (data[pos + i] << (i * HASH_SHIFT));
  return result % HASH_NUM_VALUES;
}

static unsigned countInitialZeros(const unsigned char* data, size_t size, size_t pos)
{
  size_t max_count = MAX_SUPPORTED_DEFLATE_LENGTH;
  size_t i;
  if(max_count > size - pos) max_count = size - pos;
  for(i = 0; i < max_count; i++)
  {
    if(data[pos + i] != 0)
      return i;
  }
  return max_count;
}

/*push a value to the vector in a circular way. This is to do as if we're extending
the vector's size forever, but instead the size is limited to maxsize and it wraps
around, to avoid too large memory size. The pos pointer gets updated to the current
end (unless updatepos is false, in that case pos is only used to know the current
value). returns 1 on success, 0 if fail*/
static unsigned push_circular(uivector* v, unsigned* pos, unsigned value, size_t maxsize, unsigned updatepos)
{
  if(v->size < maxsize)
  {
    if(!uivector_push_back(v, value)) return 0;
    if(updatepos) (*pos)++;
  }
  else
  {
    if(updatepos)
    {
      (*pos)++;
      if((*pos) > maxsize) (*pos) = 1;
    }
    v->data[(*pos) - 1] = value;
  }
  return 1;
}

/*Enable to use lazy instead of greedy matghing. It looks one byte further
to see if that one gives a longer distance. This gives slightly better compression, at the cost
of a speed loss.*/
#define LAZY_MATCHING

/*
LZ77-encode the data. Return value is error code. The input are raw bytes, the output
is in the form of unsigned integers with codes representing for example literal bytes, or
length/distance pairs.
It uses a hash table technique to let it encode faster. When doing LZ77 encoding, a
sliding window (of windowSize) is used, and all past bytes in that window can be used as
the "dictionary". A brute force search through all possible distances would be slow, and
this hash technique is one out of several ways to speed this up.
*/
static unsigned encodeLZ77(uivector* out, const unsigned char* in, size_t insize, unsigned windowSize)
{
  /**generate hash table**/
  /*
  The hash table is 2-dimensional. For every possible hash value, it contains a list of positions
  in the data where this hash occured.
  tablepos1 and tablepos2 remember the last used start and end index in the hash table for each hash value.
  */
  vector table; /*HASH_NUM_VALUES uivectors; this is what would be an std::vector<std::vector<unsigned> > in C++*/
  uivector tablepos1, tablepos2;
  /*hash 0 indicates a possible common case of a long sequence of zeros, store and use the amount here for a speedup*/
  uivector initialZerosTable;
  unsigned pos, i, error = 0;
  unsigned hash_num_characters = HASH_NUM_CHARACTERS;

  vector_init(&table, sizeof(uivector));
  if(!vector_resize(&table, HASH_NUM_VALUES)) return 9917; /*alloc fail*/
  for(i = 0; i < HASH_NUM_VALUES; i++)
  {
    uivector* v = (uivector*)vector_get(&table, i);
    uivector_init(v);
  }

  /*remember start and end positions in the tables to search in*/
  uivector_init(&tablepos1);
  uivector_init(&tablepos2);
  uivector_init(&initialZerosTable);

  if(!uivector_resizev(&tablepos1, HASH_NUM_VALUES, 0)) error = 9918; /*alloc fail*/
  if(!uivector_resizev(&tablepos2, HASH_NUM_VALUES, 0)) error = 9919; /*alloc fail*/

  if(!error)
  {
    unsigned offset, max_offset; /*the offset represents the distance in LZ77 terminology*/
    unsigned length, tablepos;
#ifdef LAZY_MATCHING
    unsigned lazy = 0;
    unsigned lazylength, lazyoffset;
#endif /*LAZY_MATCHING*/
    unsigned hash, initialZeros = 0;
    unsigned backpos, current_offset, t1, t2, t11, current_length;
    const unsigned char *lastptr, *foreptr, *backptr;
    uivector* v; /*vector from the hash table we're currently working on*/
    unsigned hashWindow = windowSize;
    unsigned numones = 0;

    for(pos = 0; pos < insize; pos++)
    {
      length = 0, offset = 0; /*the length and offset found for the current position*/
      max_offset = pos < windowSize ? pos : windowSize; /*how far back to test*/

      /*search for the longest string. First find out where in the table to start
      (the first value that is in the range from "pos - max_offset" to "pos")*/
      hash = getHash(in, insize, pos, hash_num_characters);
      v = (uivector*)vector_get(&table, hash);
      if(!push_circular(v, &tablepos2.data[hash], pos, hashWindow, 1)) ERROR_BREAK(9920 /*alloc fail*/);
      
      if(hash == 0)
      {
        initialZeros = countInitialZeros(in, insize, pos);
        if(!push_circular(&initialZerosTable, &tablepos2.data[hash], initialZeros, hashWindow, 0))
          ERROR_BREAK(9920 /*alloc fail*/);
      }
      
      while(v->data[tablepos1.data[hash]] < pos - max_offset)
      {
        /*it now points to the first value in the table for which the index is
        larger than or equal to pos - max_offset*/
        tablepos1.data[hash]++;
        if(tablepos1.data[hash] >= hashWindow) tablepos1.data[hash] = 0;
      }

      t1 = tablepos1.data[hash];
      t2 = tablepos2.data[hash] - 1;
      if(tablepos2.data[hash] == 0) t2 = hashWindow - 1;

      lastptr = &in[insize < pos + MAX_SUPPORTED_DEFLATE_LENGTH ? insize : pos + MAX_SUPPORTED_DEFLATE_LENGTH];

      t11 = t1 == 0 ? hashWindow - 1 : t1 - 1;
      for(tablepos = t2 == 0 ? hashWindow - 1 : t2 - 1;
          tablepos != t2 && tablepos != t11 && tablepos < v->size;
          tablepos = tablepos == 0 ? hashWindow - 1 : tablepos - 1)
      {
        backpos = v->data[tablepos];
        current_offset = pos - backpos;
        /*test the next characters*/
        foreptr = &in[pos];
        backptr = &in[backpos];

        if(hash == 0)
        {
          unsigned skip = initialZerosTable.data[tablepos];
          if(skip > initialZeros) skip = initialZeros;
          if(skip > insize - pos) skip = insize - pos;
          backptr += skip;
          foreptr += skip;
        }
        while(foreptr != lastptr && *backptr == *foreptr) /*maximum supported length by deflate is max length*/
        {
          ++backptr;
          ++foreptr;
        }
        current_length = (unsigned)(foreptr - &in[pos]);
        if(current_length > length)
        {
          length = current_length; /*the longest length*/
          offset = current_offset; /*the offset that is related to this longest length*/
          /*you can jump out of this for loop once a length of max length is found (gives significant speed gain)*/
          if(current_length == MAX_SUPPORTED_DEFLATE_LENGTH) break;
        }
      }

#ifdef LAZY_MATCHING
      if(!lazy && length >= 3 && length < MAX_SUPPORTED_DEFLATE_LENGTH)
      {
        lazylength = length;
        lazyoffset = offset;
        lazy = 1;
        continue;
      }
      if(lazy)
      {
        if(pos == 0) ERROR_BREAK(81);
        lazy = 0;
        if(length > lazylength + 1)
        {
          /*push the previous character as literal*/
          if(!uivector_push_back(out, in[pos - 1])) ERROR_BREAK(9921 /*alloc fail*/);
        }
        else
        {
          length = lazylength;
          offset = lazyoffset;
          pos--;
        }
      }
#endif /*LAZY_MATCHING*/
      
      /**encode it as length/distance pair or literal value**/
      if(length < 3) /*only lengths of 3 or higher are supported as length/distance pair*/
      {
        if(!uivector_push_back(out, in[pos])) ERROR_BREAK(9921 /*alloc fail*/);
      }
      else
      {
        unsigned j, local_hash;
        addLengthDistance(out, length, offset);
        for(j = 0; j < length - 1; j++)
        {
          unsigned* t2p; /*pointer to current tablepos2 element*/
          pos++;
          local_hash = getHash(in, insize, pos, hash_num_characters);
          t2p = &tablepos2.data[local_hash];
          v = (uivector*)vector_get(&table, local_hash);
          if(!push_circular(v, t2p, pos, hashWindow, 1)) ERROR_BREAK(9920 /*alloc fail*/);
          
          if(local_hash == 0)
          {
            initialZeros = countInitialZeros(in, insize, pos);
            if(!push_circular(&initialZerosTable, t2p, initialZeros, hashWindow, 0))
              ERROR_BREAK(9922 /*alloc fail*/);
          }
          if(local_hash == 1 && hash_num_characters == 3)
          {
            /*
            If many hash values are getting grouped together in hash value 1, 4, 16, 20, ...,
            it indicates there are many near-zero values. This is not zero enough to benefit from a speed
            increase from the initialZerosTable, and makes it very slow. For that case only, switch to
            hash_num_characters = 6. Value 6 is experimentally found to be fastest. For this particular type
            of file, the compression isn't even worse, despite the fact that lengths < 6 are now no longer
            found, and that by changing hash_num_characters not all previously found hash values are still valid.
            Almost all images compress fast enough and smaller with hash_num_characters = 3, except sine plasma
            images. Those benefit a lot from this heuristic.
            */
            if(numones > 8192 && numones > pos / 16) hash_num_characters = 6;
            else numones++;
          }
        }
      }
    } /*end of the loop through each character of input*/
  } /*end of "if(!error)"*/

  /*cleanup*/
  for(i = 0; i < table.size; i++)
  {
    uivector* v = (uivector*)vector_get(&table, i);
    uivector_cleanup(v);
  }
  vector_cleanup(&table);
  uivector_cleanup(&tablepos1);
  uivector_cleanup(&tablepos2);
  uivector_cleanup(&initialZerosTable);
  return error;
}

/* /////////////////////////////////////////////////////////////////////////// */

static unsigned deflateNoCompression(ucvector* out, const unsigned char* data, size_t datasize)
{
  /*non compressed deflate block data: 1 bit BFINAL,2 bits BTYPE,(5 bits): it jumps to start of next byte,
  2 bytes LEN, 2 bytes NLEN, LEN bytes literal DATA*/

  size_t i, j, numdeflateblocks = datasize / 65536 + 1;
  unsigned datapos = 0;
  for(i = 0; i < numdeflateblocks; i++)
  {
    unsigned BFINAL, BTYPE, LEN, NLEN;
    unsigned char firstbyte;

    BFINAL = (i == numdeflateblocks - 1);
    BTYPE = 0;

    firstbyte = (unsigned char)(BFINAL + ((BTYPE & 1) << 1) + ((BTYPE & 2) << 1));
    ucvector_push_back(out, firstbyte);

    LEN = 65535;
    if(datasize - datapos < 65535) LEN = (unsigned)datasize - datapos;
    NLEN = 65535 - LEN;

    ucvector_push_back(out, (unsigned char)(LEN % 256));
    ucvector_push_back(out, (unsigned char)(LEN / 256));
    ucvector_push_back(out, (unsigned char)(NLEN % 256));
    ucvector_push_back(out, (unsigned char)(NLEN / 256));

    /*Decompressed data*/
    for(j = 0; j < 65535 && datapos < datasize; j++)
    {
      ucvector_push_back(out, data[datapos++]);
    }
  }

  return 0;
}

/*
write the lz77-encoded data, which has lit, len and dist codes, to compressed stream using huffman trees.
tree_ll: the tree for lit and len codes.
tree_d: the tree for distance codes.
*/
static void writeLZ77data(size_t* bp, ucvector* out, const uivector* lz77_encoded,
                          const HuffmanTree* tree_ll, const HuffmanTree* tree_d)
{
  size_t i = 0;
  for(i = 0; i < lz77_encoded->size; i++)
  {
    unsigned val = lz77_encoded->data[i];
    addHuffmanSymbol(bp, out, HuffmanTree_getCode(tree_ll, val), HuffmanTree_getLength(tree_ll, val));
    if(val > 256) /*for a length code, 3 more things have to be added*/
    {
      unsigned length_index = val - FIRST_LENGTH_CODE_INDEX;
      unsigned n_length_extra_bits = LENGTHEXTRA[length_index];
      unsigned length_extra_bits = lz77_encoded->data[++i];

      unsigned distance_code = lz77_encoded->data[++i];

      unsigned distance_index = distance_code;
      unsigned n_distance_extra_bits = DISTANCEEXTRA[distance_index];
      unsigned distance_extra_bits = lz77_encoded->data[++i];

      addBitsToStream(bp, out, length_extra_bits, n_length_extra_bits);
      addHuffmanSymbol(bp, out, HuffmanTree_getCode(tree_d, distance_code),
                       HuffmanTree_getLength(tree_d, distance_code));
      addBitsToStream(bp, out, distance_extra_bits, n_distance_extra_bits);
    }
  }
}

/*Deflate for a block of type "dynamic", that is, with freely, optimally, created huffman trees*/
static unsigned deflateDynamic(ucvector* out, const unsigned char* data, size_t datasize,
                               const LodePNG_CompressSettings* settings)
{
  unsigned error = 0;

  /*
  A block is compressed as follows: The PNG data is lz77 encoded, resulting in
  literal bytes and length/distance pairs. This is then huffman compressed with
  two huffman trees. One huffman tree is used for the lit and len values ("ll"),
  another huffman tree is used for the dist values ("d"). These two trees are
  stored using their code lengths, and to compress even more these code lengths
  are also run-length encoded and huffman compressed. This gives a huffman tree
  of code lengths "cl". The code lenghts used to describe this third tree are
  the code length code lengths ("clcl").
  */
  
  /*The lz77 encoded data, represented with integers since there will also be length and distance codes in it*/
  uivector lz77_encoded;
  HuffmanTree tree_ll; /*tree for lit,len values*/
  HuffmanTree tree_d; /*tree for distance codes*/
  HuffmanTree tree_cl; /*tree for encoding the code lengths representing tree_ll and tree_d*/
  uivector frequencies_ll; /*frequency of lit,len codes*/
  uivector frequencies_d; /*frequency of dist codes*/
  uivector frequencies_cl; /*frequency of code length codes*/
  uivector bitlen_lld; /*lit,len,dist code lenghts (int bits), literally (without repeat codes).*/
  uivector bitlen_lld_e; /*bitlen_lld encoded with repeat codes (this is a rudemtary run length compression)*/
  /*bitlen_cl is the code length code lengths ("clcl"). The bit lengths of codes to represent tree_cl
  (these are written as is in the file, it would be crazy to compress these using yet another huffman
  tree that needs to be represented by yet another set of code lengths)*/
  uivector bitlen_cl;

  /*
  Due to the huffman compression of huffman tree representations ("two levels"), there are some anologies:
  bitlen_lld is to tree_cl what data is to tree_ll and tree_d. 
  bitlen_lld_e is to bitlen_lld what lz77_encoded is to data.
  bitlen_cl is to bitlen_lld_e what bitlen_lld is to lz77_encoded.
  */

  unsigned BFINAL = 1; /*make only one block... the first and final one*/
  size_t numcodes_ll, numcodes_d, i;
  size_t bp = 0; /*the bit pointer*/
  unsigned HLIT, HDIST, HCLEN;

  uivector_init(&lz77_encoded);
  HuffmanTree_init(&tree_ll);
  HuffmanTree_init(&tree_d);
  HuffmanTree_init(&tree_cl);
  uivector_init(&frequencies_ll);
  uivector_init(&frequencies_d);
  uivector_init(&frequencies_cl);
  uivector_init(&bitlen_lld);
  uivector_init(&bitlen_lld_e);
  uivector_init(&bitlen_cl);

  /*This while loop is never loops due to a break at the end, it is here to
  allow breaking out of it to the cleanup phase on error conditions.*/
  while(!error)
  {
    if(settings->useLZ77)
    {
      error = encodeLZ77(&lz77_encoded, data, datasize, settings->windowSize); /*LZ77 encoded*/
      if(error) break;
    }
    else
    {
      if(!uivector_resize(&lz77_encoded, datasize)) ERROR_BREAK(9923 /*alloc fail*/);
      for(i = 0; i < datasize; i++) lz77_encoded.data[i] = data[i]; /*no LZ77, but still will be Huffman compressed*/
    }

    if(!uivector_resizev(&frequencies_ll, 286, 0)) ERROR_BREAK(9924 /*alloc fail*/);
    if(!uivector_resizev(&frequencies_d, 30, 0)) ERROR_BREAK(9925 /*alloc fail*/);

    /*Count the frequencies of lit, len and dist codes*/
    for(i = 0; i < lz77_encoded.size; i++)
    {
      unsigned symbol = lz77_encoded.data[i];
      frequencies_ll.data[symbol]++;
      if(symbol > 256)
      {
        unsigned dist = lz77_encoded.data[i + 2];
        frequencies_d.data[dist]++;
        i += 3;
      }
    }
    frequencies_ll.data[256] = 1; /*there will be exactly 1 end code, at the end of the block*/

    /*Make both huffman trees, one for the lit and len codes, one for the dist codes*/
    error = HuffmanTree_makeFromFrequencies(&tree_ll, frequencies_ll.data, frequencies_ll.size, 15);
    if(error) break;
    error = HuffmanTree_makeFromFrequencies(&tree_d, frequencies_d.data, frequencies_d.size, 15);
    if(error) break;

    numcodes_ll = tree_ll.numcodes; if(numcodes_ll > 286) numcodes_ll = 286;
    numcodes_d = tree_d.numcodes; if(numcodes_d > 30) numcodes_d = 30;
    /*store the code lengths of both generated trees in bitlen_lld*/
    for(i = 0; i < numcodes_ll; i++) uivector_push_back(&bitlen_lld, HuffmanTree_getLength(&tree_ll, (unsigned)i));
    for(i = 0; i < numcodes_d; i++) uivector_push_back(&bitlen_lld, HuffmanTree_getLength(&tree_d, (unsigned)i));

    /*run-length compress bitlen_ldd into bitlen_lld_e by using repeat codes 16 (copy length 3-6 times),
    17 (3-10 zeroes), 18 (11-138 zeroes)*/
    for(i = 0; i < (unsigned)bitlen_lld.size; i++)
    {
      unsigned j = 0; /*amount of repititions*/
      while(i + j + 1 < (unsigned)bitlen_lld.size && bitlen_lld.data[i + j + 1] == bitlen_lld.data[i]) j++;

      if(bitlen_lld.data[i] == 0 && j >= 2) /*repeat code for zeroes*/
      {
        j++; /*include the first zero*/
        if(j <= 10) /*repeat code 17 supports max 10 zeroes*/
        {
          uivector_push_back(&bitlen_lld_e, 17);
          uivector_push_back(&bitlen_lld_e, j - 3);
        }
        else /*repeat code 18 supports max 138 zeroes*/
        {
          if(j > 138) j = 138;
          uivector_push_back(&bitlen_lld_e, 18);
          uivector_push_back(&bitlen_lld_e, j - 11);
        }
        i += (j - 1);
      }
      else if(j >= 3) /*repeat code for value other than zero*/
      {
        size_t k;
        unsigned num = j / 6, rest = j % 6;
        uivector_push_back(&bitlen_lld_e, bitlen_lld.data[i]);
        for(k = 0; k < num; k++)
        {
          uivector_push_back(&bitlen_lld_e, 16);
          uivector_push_back(&bitlen_lld_e, 6 - 3);
        }
        if(rest >= 3)
        {
          uivector_push_back(&bitlen_lld_e, 16);
          uivector_push_back(&bitlen_lld_e, rest - 3);
        }
        else j -= rest;
        i += j;
      }
      else /*too short to benefit from repeat code*/
      {
        uivector_push_back(&bitlen_lld_e, bitlen_lld.data[i]);
      }
    }

    /*generate tree_cl, the huffmantree of huffmantrees*/

    if(!uivector_resizev(&frequencies_cl, NUM_CODE_LENGTH_CODES, 0)) ERROR_BREAK(9926 /*alloc fail*/);
    for(i = 0; i < bitlen_lld_e.size; i++)
    {
      frequencies_cl.data[bitlen_lld_e.data[i]]++;
      /*after a repeat code come the bits that specify the number of repetitions,
      those don't need to be in the frequencies_cl calculation*/
      if(bitlen_lld_e.data[i] >= 16) i++;
    }

    error = HuffmanTree_makeFromFrequencies(&tree_cl, frequencies_cl.data, frequencies_cl.size, 7);
    if(error) break;

    if(!uivector_resize(&bitlen_cl, NUM_CODE_LENGTH_CODES)) ERROR_BREAK(9927 /*alloc fail*/);
    for(i = 0; i < NUM_CODE_LENGTH_CODES; i++)
    {
      /*lenghts of code length tree is in the order as specified by deflate*/
      bitlen_cl.data[i] = HuffmanTree_getLength(&tree_cl, CLCL_ORDER[i]);
    }
    while(bitlen_cl.data[bitlen_cl.size - 1] == 0 && bitlen_cl.size > 4)
    {
      /*remove zeros at the end, but minimum size must be 4*/
      if(!uivector_resize(&bitlen_cl, bitlen_cl.size - 1)) ERROR_BREAK(9928 /*alloc fail*/);
    }
    if(error) break;

    /*
    Write everything into the output
    
    After the BFINAL and BTYPE, the dynamic block consists out of the following:
    - 5 bits HLIT, 5 bits HDIST, 4 bits HCLEN
    - (HCLEN+4)*3 bits code lengths of code length alphabet
    - HLIT + 257 code lenghts of lit/length alphabet (encoded using the code length
      alphabet, + possible repetition codes 16, 17, 18)
    - HDIST + 1 code lengths of distance alphabet (encoded using the code length
      alphabet, + possible repetition codes 16, 17, 18)
    - compressed data
    - 256 (end code)
    */

    /*Write block type*/
    addBitToStream(&bp, out, BFINAL);
    addBitToStream(&bp, out, 0); /*first bit of BTYPE "dynamic"*/
    addBitToStream(&bp, out, 1); /*second bit of BTYPE "dynamic"*/

    /*write the HLIT, HDIST and HCLEN values*/
    HLIT = (unsigned)(numcodes_ll - 257);
    HDIST = (unsigned)(numcodes_d - 1);
    HCLEN = (unsigned)bitlen_cl.size - 4;
    addBitsToStream(&bp, out, HLIT, 5);
    addBitsToStream(&bp, out, HDIST, 5);
    addBitsToStream(&bp, out, HCLEN, 4);

    /*write the code lenghts of the code length alphabet*/
    for(i = 0; i < HCLEN + 4; i++) addBitsToStream(&bp, out, bitlen_cl.data[i], 3);

    /*write the lenghts of the lit/len AND the dist alphabet*/
    for(i = 0; i < bitlen_lld_e.size; i++)
    {
      addHuffmanSymbol(&bp, out, HuffmanTree_getCode(&tree_cl, bitlen_lld_e.data[i]),
                       HuffmanTree_getLength(&tree_cl, bitlen_lld_e.data[i]));
      /*extra bits of repeat codes*/
      if(bitlen_lld_e.data[i] == 16) addBitsToStream(&bp, out, bitlen_lld_e.data[++i], 2);
      else if(bitlen_lld_e.data[i] == 17) addBitsToStream(&bp, out, bitlen_lld_e.data[++i], 3);
      else if(bitlen_lld_e.data[i] == 18) addBitsToStream(&bp, out, bitlen_lld_e.data[++i], 7);
    }

    /*write the compressed data symbols*/
    writeLZ77data(&bp, out, &lz77_encoded, &tree_ll, &tree_d);
    /*error: the length of the end code 256 must be larger than 0*/
    if(HuffmanTree_getLength(&tree_ll, 256) == 0) ERROR_BREAK(64);

    /*write the end code*/
    addHuffmanSymbol(&bp, out, HuffmanTree_getCode(&tree_ll, 256), HuffmanTree_getLength(&tree_ll, 256));

    break; /*end of error-while*/
  }

  /*cleanup*/
  uivector_cleanup(&lz77_encoded);
  HuffmanTree_cleanup(&tree_ll);
  HuffmanTree_cleanup(&tree_d);
  HuffmanTree_cleanup(&tree_cl);
  uivector_cleanup(&frequencies_ll);
  uivector_cleanup(&frequencies_d);
  uivector_cleanup(&frequencies_cl);
  uivector_cleanup(&bitlen_lld_e);
  uivector_cleanup(&bitlen_lld);
  uivector_cleanup(&bitlen_cl);

  return error;
}

static unsigned deflateFixed(ucvector* out, const unsigned char* data,
                             size_t datasize, const LodePNG_CompressSettings* settings)
{
  HuffmanTree tree_ll; /*tree for literal values and length codes*/
  HuffmanTree tree_d; /*tree for distance codes*/

  unsigned BFINAL = 1; /*make only one block... the first and final one*/
  unsigned error = 0;
  size_t i, bp = 0; /*the bit pointer*/

  HuffmanTree_init(&tree_ll);
  HuffmanTree_init(&tree_d);

  generateFixedLitLenTree(&tree_ll);
  generateFixedDistanceTree(&tree_d);

  addBitToStream(&bp, out, BFINAL);
  addBitToStream(&bp, out, 1); /*first bit of BTYPE*/
  addBitToStream(&bp, out, 0); /*second bit of BTYPE*/

  if(settings->useLZ77) /*LZ77 encoded*/
  {
    uivector lz77_encoded;
    uivector_init(&lz77_encoded);
    error = encodeLZ77(&lz77_encoded, data, datasize, settings->windowSize);
    if(!error) writeLZ77data(&bp, out, &lz77_encoded, &tree_ll, &tree_d);
    uivector_cleanup(&lz77_encoded);
  }
  else /*no LZ77, but still will be Huffman compressed*/
  {
    for(i = 0; i < datasize; i++)
    {
      addHuffmanSymbol(&bp, out, HuffmanTree_getCode(&tree_ll, data[i]), HuffmanTree_getLength(&tree_ll, data[i]));
    }
  }
  /*add END code*/
  if(!error) addHuffmanSymbol(&bp, out, HuffmanTree_getCode(&tree_ll, 256), HuffmanTree_getLength(&tree_ll, 256));

  /*cleanup*/
  HuffmanTree_cleanup(&tree_ll);
  HuffmanTree_cleanup(&tree_d);

  return error;
}

static unsigned LodePNG_deflate(ucvector* out, const unsigned char* data, size_t datasize,
                                const LodePNG_CompressSettings* settings)
{
  unsigned error = 0;
  if(settings->btype == 0) error = deflateNoCompression(out, data, datasize);
  else if(settings->btype == 1) error = deflateFixed(out, data, datasize, settings);
  else if(settings->btype == 2) error = deflateDynamic(out, data, datasize, settings);
  else error = 61;
  return error;
}

#endif /*LODEPNG_COMPILE_DECODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* / Adler32                                                                  */
/* ////////////////////////////////////////////////////////////////////////// */

static unsigned update_adler32(unsigned adler, const unsigned char* data, unsigned len)
{
   unsigned s1 = adler & 0xffff;
   unsigned s2 = (adler >> 16) & 0xffff;

  while(len > 0)
  {
    /*at least 5550 sums can be done before the sums overflow, saving a lot of module divisions*/
    unsigned amount = len > 5550 ? 5550 : len;
    len -= amount;
    while(amount > 0)
    {
      s1 = (s1 + *data++);
      s2 = (s2 + s1);
      amount--;
    }
    s1 %= 65521;
    s2 %= 65521;
  }

  return (s2 << 16) | s1;
}

/*Return the adler32 of the bytes data[0..len-1]*/
static unsigned adler32(const unsigned char* data, unsigned len)
{
  return update_adler32(1L, data, len);
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Zlib                                                                   / */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_DECODER

unsigned LodePNG_zlib_decompress(unsigned char** out, size_t* outsize, const unsigned char* in,
                                 size_t insize, const LodePNG_DecompressSettings* settings)
{
  unsigned error = 0;
  unsigned CM, CINFO, FDICT;
  ucvector outv;

  if(insize < 2) return 53; /*error, size of zlib data too small*/
  /*read information from zlib header*/
  if((in[0] * 256 + in[1]) % 31 != 0)
  {
    /*error: 256 * in[0] + in[1] must be a multiple of 31, the FCHECK value is supposed to be made that way*/
    return 24;
  }

  CM = in[0] & 15;
  CINFO = (in[0] >> 4) & 15;
  /*FCHECK = in[1] & 31;*/ /*FCHECK is already tested above*/
  FDICT = (in[1] >> 5) & 1;
  /*FLEVEL = (in[1] >> 6) & 3;*/ /*FLEVEL is not used here*/

  if(CM != 8 || CINFO > 7)
  {
    /*error: only compression method 8: inflate with sliding window of 32k is supported by the PNG spec*/
    return 25;
  }
  if(FDICT != 0)
  {
    /*error: the specification of PNG says about the zlib stream:
      "The additional flags shall not specify a preset dictionary."*/
    return 26;
  }

  /*ucvector-controlled version of the output buffer, for dynamic array*/
  ucvector_init_buffer(&outv, *out, *outsize);
  error = LodePNG_inflate(&outv, in, insize, 2);
  *out = outv.data;
  *outsize = outv.size;
  if(error) return error;

  if(!settings->ignoreAdler32)
  {
    unsigned ADLER32 = LodePNG_read32bitInt(&in[insize - 4]);
    unsigned checksum = adler32(outv.data, (unsigned)outv.size);
    if(checksum != ADLER32) return 58; /*error, adler checksum not correct, data must be corrupted*/
  }

  return 0; /*no error*/
}

#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER

unsigned LodePNG_zlib_compress(unsigned char** out, size_t* outsize, const unsigned char* in,
                               size_t insize, const LodePNG_CompressSettings* settings)
{
  /*initially, *out must be NULL and outsize 0, if you just give some random *out
  that's pointing to a non allocated buffer, this'll crash*/
  ucvector deflatedata, outv;
  size_t i;
  unsigned error;

  unsigned ADLER32;
  /*zlib data: 1 byte CMF (CM+CINFO), 1 byte FLG, deflate data, 4 byte ADLER32 checksum of the Decompressed data*/
  unsigned CMF = 120; /*0b01111000: CM 8, CINFO 7. With CINFO 7, any window size up to 32768 can be used.*/
  unsigned FLEVEL = 0;
  unsigned FDICT = 0;
  unsigned CMFFLG = 256 * CMF + FDICT * 32 + FLEVEL * 64;
  unsigned FCHECK = 31 - CMFFLG % 31;
  CMFFLG += FCHECK;

  /*ucvector-controlled version of the output buffer, for dynamic array*/
  ucvector_init_buffer(&outv, *out, *outsize);

  ucvector_push_back(&outv, (unsigned char)(CMFFLG / 256));
  ucvector_push_back(&outv, (unsigned char)(CMFFLG % 256));

  ucvector_init(&deflatedata);
  error = LodePNG_deflate(&deflatedata, in, insize, settings);

  if(!error)
  {
    ADLER32 = adler32(in, (unsigned)insize);
    for(i = 0; i < deflatedata.size; i++) ucvector_push_back(&outv, deflatedata.data[i]);
    ucvector_cleanup(&deflatedata);
    LodePNG_add32bitInt(&outv, ADLER32);
  }

  *out = outv.data;
  *outsize = outv.size;

  return error;
}

#endif /*LODEPNG_COMPILE_ENCODER*/

#else /*no LODEPNG_COMPILE_ZLIB*/

/*
Dummy functions used if LODEPNG_COMPILE_ZLIB isn't defined. You need to implement
these yourself when disabling the LodePNG Zlib part, e.g. by calling another
library from here.

*out must be NULL and *outsize must be 0 initially, and after the function is done,
*out must point to the decompressed data, *outsize must be the size of it, and must
be the size of the useful data in bytes, not the alloc size.
*/

#ifdef LODEPNG_COMPILE_DECODER
static unsigned LodePNG_zlib_decompress(unsigned char** out, size_t* outsize, const unsigned char* in,
                                        size_t insize, const LodePNG_DecompressSettings* settings)
{
  return 0; //Placeholder to be implemented if LODEPNG_COMPILE_ZLIB is disabled.
}
#endif /*LODEPNG_COMPILE_DECODER*/
#ifdef LODEPNG_COMPILE_ENCODER
static unsigned LodePNG_zlib_compress(unsigned char** out, size_t* outsize, const unsigned char* in,
                                      size_t insize, const LodePNG_CompressSettings* settings)
{
  return 0; //Placeholder to be implemented if LODEPNG_COMPILE_ZLIB is disabled.
}
#endif /*LODEPNG_COMPILE_ENCODER*/

#endif /*LODEPNG_COMPILE_ZLIB*/

/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_ENCODER

/*this is a good tradeoff between speed and compression ratio*/
#define DEFAULT_WINDOWSIZE 2048

void LodePNG_CompressSettings_init(LodePNG_CompressSettings* settings)
{
  /*compress with dynamic huffman tree (not in the mathematical sense, just not the predefined one)*/
  settings->btype = 2;
  settings->useLZ77 = 1;
  settings->windowSize = DEFAULT_WINDOWSIZE;
}

const LodePNG_CompressSettings LodePNG_defaultCompressSettings = {2, 1, DEFAULT_WINDOWSIZE};

#endif /*LODEPNG_COMPILE_ENCODER*/

#ifdef LODEPNG_COMPILE_DECODER

void LodePNG_DecompressSettings_init(LodePNG_DecompressSettings* settings)
{
  settings->ignoreAdler32 = 0;
}

const LodePNG_DecompressSettings LodePNG_defaultDecompressSettings = {0};

#endif /*LODEPNG_COMPILE_DECODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // End of Zlib related code. Begin of PNG related code.                 // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_PNG

/* ////////////////////////////////////////////////////////////////////////// */
/* / CRC32                                                                  / */
/* ////////////////////////////////////////////////////////////////////////// */

static unsigned Crc32_crc_table_computed = 0;
static unsigned Crc32_crc_table[256];

/*Make the table for a fast CRC.*/
static void Crc32_make_crc_table(void)
{
  unsigned c, k, n;
  for(n = 0; n < 256; n++)
  {
    c = n;
    for(k = 0; k < 8; k++)
    {
      if(c & 1) c = 0xedb88320L ^ (c >> 1);
      else c = c >> 1;
    }
    Crc32_crc_table[n] = c;
  }
  Crc32_crc_table_computed = 1;
}

/*Update a running CRC with the bytes buf[0..len-1]--the CRC should be
initialized to all 1's, and the transmitted value is the 1's complement of the
final running CRC (see the crc() routine below).*/
static unsigned Crc32_update_crc(const unsigned char* buf, unsigned crc, size_t len)
{
  unsigned c = crc;
  size_t n;

  if(!Crc32_crc_table_computed) Crc32_make_crc_table();
  for(n = 0; n < len; n++)
  {
    c = Crc32_crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c;
}

/*Return the CRC of the bytes buf[0..len-1].*/
static unsigned Crc32_crc(const unsigned char* buf, size_t len)
{
  return Crc32_update_crc(buf, 0xffffffffL, len) ^ 0xffffffffL;
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Reading and writing single bits and bytes from/to stream for LodePNG   / */
/* ////////////////////////////////////////////////////////////////////////// */

static unsigned char readBitFromReversedStream(size_t* bitpointer, const unsigned char* bitstream)
{
  unsigned char result = (unsigned char)((bitstream[(*bitpointer) >> 3] >> (7 - ((*bitpointer) & 0x7))) & 1);
  (*bitpointer)++;
  return result;
}

static unsigned readBitsFromReversedStream(size_t* bitpointer, const unsigned char* bitstream, size_t nbits)
{
  unsigned result = 0;
  size_t i;
  for(i = nbits - 1; i < nbits; i--)
  {
    result += (unsigned)readBitFromReversedStream(bitpointer, bitstream) << i;
  }
  return result;
}

#ifdef LODEPNG_COMPILE_DECODER
static void setBitOfReversedStream0(size_t* bitpointer, unsigned char* bitstream, unsigned char bit)
{
  /*the current bit in bitstream must be 0 for this to work*/
  if(bit)
  {
    /*earlier bit of huffman code is in a lesser significant bit of an earlier byte*/
    bitstream[(*bitpointer) >> 3] |= (bit << (7 - ((*bitpointer) & 0x7)));
  }
  (*bitpointer)++;
}
#endif /*LODEPNG_COMPILE_DECODER*/

static void setBitOfReversedStream(size_t* bitpointer, unsigned char* bitstream, unsigned char bit)
{
  /*the current bit in bitstream may be 0 or 1 for this to work*/
  if(bit == 0) bitstream[(*bitpointer) >> 3] &=  (unsigned char)(~(1 << (7 - ((*bitpointer) & 0x7))));
  else         bitstream[(*bitpointer) >> 3] |=  (1 << (7 - ((*bitpointer) & 0x7)));
  (*bitpointer)++;
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG chunks                                                             / */
/* ////////////////////////////////////////////////////////////////////////// */

unsigned LodePNG_chunk_length(const unsigned char* chunk)
{
  return LodePNG_read32bitInt(&chunk[0]);
}

void LodePNG_chunk_type(char type[5], const unsigned char* chunk)
{
  unsigned i;
  for(i = 0; i < 4; i++) type[i] = chunk[4 + i];
  type[4] = 0; /*null termination char*/
}

unsigned char LodePNG_chunk_type_equals(const unsigned char* chunk, const char* type)
{
  if(strlen(type) != 4) return 0;
  return (chunk[4] == type[0] && chunk[5] == type[1] && chunk[6] == type[2] && chunk[7] == type[3]);
}

unsigned char LodePNG_chunk_critical(const unsigned char* chunk)
{
  return((chunk[4] & 32) == 0);
}

unsigned char LodePNG_chunk_private(const unsigned char* chunk)
{
  return((chunk[6] & 32) != 0);
}

unsigned char LodePNG_chunk_safetocopy(const unsigned char* chunk)
{
  return((chunk[7] & 32) != 0);
}

unsigned char* LodePNG_chunk_data(unsigned char* chunk)
{
  return &chunk[8];
}

const unsigned char* LodePNG_chunk_data_const(const unsigned char* chunk)
{
  return &chunk[8];
}

unsigned LodePNG_chunk_check_crc(const unsigned char* chunk)
{
  unsigned length = LodePNG_chunk_length(chunk);
  unsigned CRC = LodePNG_read32bitInt(&chunk[length + 8]);
  /*the CRC is taken of the data and the 4 chunk type letters, not the length*/
  unsigned checksum = Crc32_crc(&chunk[4], length + 4);
  if(CRC != checksum) return 1;
  else return 0;
}

void LodePNG_chunk_generate_crc(unsigned char* chunk)
{
  unsigned length = LodePNG_chunk_length(chunk);
  unsigned CRC = Crc32_crc(&chunk[4], length + 4);
  LodePNG_set32bitInt(chunk + 8 + length, CRC);
}

unsigned char* LodePNG_chunk_next(unsigned char* chunk)
{
  unsigned total_chunk_length = LodePNG_chunk_length(chunk) + 12;
  return &chunk[total_chunk_length];
}

const unsigned char* LodePNG_chunk_next_const(const unsigned char* chunk)
{
  unsigned total_chunk_length = LodePNG_chunk_length(chunk) + 12;
  return &chunk[total_chunk_length];
}

unsigned LodePNG_append_chunk(unsigned char** out, size_t* outlength, const unsigned char* chunk)
{
  unsigned i;
  unsigned total_chunk_length = LodePNG_chunk_length(chunk) + 12;
  unsigned char *chunk_start, *new_buffer;
  size_t new_length = (*outlength) + total_chunk_length;
  if(new_length < total_chunk_length || new_length < (*outlength)) return 77; /*integer overflow happened*/

  new_buffer = (unsigned char*)realloc(*out, new_length);
  if(!new_buffer) return 9929; /*alloc fail*/
  (*out) = new_buffer;
  (*outlength) = new_length;
  chunk_start = &(*out)[new_length - total_chunk_length];

  for(i = 0; i < total_chunk_length; i++) chunk_start[i] = chunk[i];

  return 0;
}

unsigned LodePNG_create_chunk(unsigned char** out, size_t* outlength, unsigned length,
                              const char* type, const unsigned char* data)
{
  unsigned i;
  unsigned char *chunk, *new_buffer;
  size_t new_length = (*outlength) + length + 12;
  if(new_length < length + 12 || new_length < (*outlength)) return 77; /*integer overflow happened*/
  new_buffer = (unsigned char*)realloc(*out, new_length);
  if(!new_buffer) return 9930; /*alloc fail*/
  (*out) = new_buffer;
  (*outlength) = new_length;
  chunk = &(*out)[(*outlength) - length - 12];

  /*1: length*/
  LodePNG_set32bitInt(chunk, (unsigned)length);

  /*2: chunk name (4 letters)*/
  chunk[4] = type[0];
  chunk[5] = type[1];
  chunk[6] = type[2];
  chunk[7] = type[3];

  /*3: the data*/
  for(i = 0; i < length; i++) chunk[8 + i] = data[i];

  /*4: CRC (of the chunkname characters and the data)*/
  LodePNG_chunk_generate_crc(chunk);

  return 0;
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Color types and such                                                   / */
/* ////////////////////////////////////////////////////////////////////////// */

/*return type is a LodePNG error code*/
static unsigned checkColorValidity(unsigned colorType, unsigned bd) /*bd = bitDepth*/
{
  switch(colorType)
  {
    case 0: if(!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16)) return 37; break; /*grey*/
    case 2: if(!(                                 bd == 8 || bd == 16)) return 37; break; /*RGB*/
    case 3: if(!(bd == 1 || bd == 2 || bd == 4 || bd == 8            )) return 37; break; /*palette*/
    case 4: if(!(                                 bd == 8 || bd == 16)) return 37; break; /*grey + alpha*/
    case 6: if(!(                                 bd == 8 || bd == 16)) return 37; break; /*RGBA*/
    default: return 31;
  }
  return 0; /*allowed color type / bits combination*/
}

static unsigned getNumColorChannels(unsigned colorType)
{
  switch(colorType)
  {
    case 0: return 1; /*grey*/
    case 2: return 3; /*RGB*/
    case 3: return 1; /*palette*/
    case 4: return 2; /*grey + alpha*/
    case 6: return 4; /*RGBA*/
  }
  return 0; /*unexisting color type*/
}

static unsigned getBpp(unsigned colorType, unsigned bitDepth)
{
  /*bits per pixel is amount of channels * bits per channel*/
  return getNumColorChannels(colorType) * bitDepth;
}

/* ////////////////////////////////////////////////////////////////////////// */

void LodePNG_InfoColor_init(LodePNG_InfoColor* info)
{
  info->key_defined = 0;
  info->key_r = info->key_g = info->key_b = 0;
  info->colorType = 6;
  info->bitDepth = 8;
  info->palette = 0;
  info->palettesize = 0;
}

void LodePNG_InfoColor_cleanup(LodePNG_InfoColor* info)
{
  LodePNG_InfoColor_clearPalette(info);
}

void LodePNG_InfoColor_clearPalette(LodePNG_InfoColor* info)
{
  if(info->palette) free(info->palette);
  info->palettesize = 0;
}

unsigned LodePNG_InfoColor_addPalette(LodePNG_InfoColor* info,
                                      unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  unsigned char* data;
  /*the same resize technique as C++ std::vectors is used, and here it's made so that for a palette with
  the max of 256 colors, it'll have the exact alloc size*/
  if(!(info->palettesize & (info->palettesize - 1))) /*if palettesize is 0 or a power of two*/
  {
    /*allocated data must be at least 4* palettesize (for 4 color bytes)*/
    size_t alloc_size = info->palettesize == 0 ? 4 : info->palettesize * 4 * 2;
    data = (unsigned char*)realloc(info->palette, alloc_size);
    if(!data) return 9931; /*alloc fail*/
    else info->palette = data;
  }
  info->palette[4 * info->palettesize + 0] = r;
  info->palette[4 * info->palettesize + 1] = g;
  info->palette[4 * info->palettesize + 2] = b;
  info->palette[4 * info->palettesize + 3] = a;
  info->palettesize++;
  return 0;
}

unsigned LodePNG_InfoColor_getBpp(const LodePNG_InfoColor* info)
{
  /*calculate bits per pixel out of colorType and bitDepth*/
  return getBpp(info->colorType, info->bitDepth);
}

unsigned LodePNG_InfoColor_getChannels(const LodePNG_InfoColor* info)
{
  return getNumColorChannels(info->colorType);
}

unsigned LodePNG_InfoColor_isGreyscaleType(const LodePNG_InfoColor* info)
{
  return info->colorType == 0 || info->colorType == 4;
}

unsigned LodePNG_InfoColor_isAlphaType(const LodePNG_InfoColor* info)
{
  return (info->colorType & 4) != 0;
}

unsigned LodePNG_InfoColor_isPaletteType(const LodePNG_InfoColor* info)
{
  return info->colorType == 3;
}

unsigned LodePNG_InfoColor_hasPaletteAlpha(const LodePNG_InfoColor* info)
{
  size_t i;
  for(i = 0; i < info->palettesize; i++)
  {
    if(info->palette[i * 4 + 3] < 255) return 1;
  }
  return 0;
}

unsigned LodePNG_InfoColor_canHaveAlpha(const LodePNG_InfoColor* info)
{
  return info->key_defined
      || LodePNG_InfoColor_isAlphaType(info)
      || LodePNG_InfoColor_hasPaletteAlpha(info);
}

unsigned LodePNG_InfoColor_equal(const LodePNG_InfoColor* info1, const LodePNG_InfoColor* info2)
{
  return info1->colorType == info2->colorType
      && info1->bitDepth  == info2->bitDepth; /*palette and color key not compared*/
}

#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
void LodePNG_UnknownChunks_init(LodePNG_UnknownChunks* chunks)
{
  unsigned i;
  for(i = 0; i < 3; i++) chunks->data[i] = 0;
  for(i = 0; i < 3; i++) chunks->datasize[i] = 0;
}

void LodePNG_UnknownChunks_cleanup(LodePNG_UnknownChunks* chunks)
{
  unsigned i;
  for(i = 0; i < 3; i++) free(chunks->data[i]);
}

unsigned LodePNG_UnknownChunks_copy(LodePNG_UnknownChunks* dest, const LodePNG_UnknownChunks* src)
{
  unsigned i;

  LodePNG_UnknownChunks_cleanup(dest);

  for(i = 0; i < 3; i++)
  {
    size_t j;
    dest->datasize[i] = src->datasize[i];
    dest->data[i] = (unsigned char*)malloc(src->datasize[i]);
    if(!dest->data[i] && dest->datasize[i]) return 9932; /*alloc fail*/
    for(j = 0; j < src->datasize[i]; j++) dest->data[i][j] = src->data[i][j];
  }

  return 0;
}
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
void LodePNG_Text_init(LodePNG_Text* text)
{
  text->num = 0;
  text->keys = NULL;
  text->strings = NULL;
}

void LodePNG_Text_cleanup(LodePNG_Text* text)
{
  LodePNG_Text_clear(text);
}

unsigned LodePNG_Text_copy(LodePNG_Text* dest, const LodePNG_Text* source)
{
  size_t i = 0;
  dest->keys = 0;
  dest->strings = 0;
  dest->num = 0;
  for(i = 0; i < source->num; i++)
  {
    unsigned error = LodePNG_Text_add(dest, source->keys[i], source->strings[i]);
    if(error) return error;
  }
  return 0;
}

void LodePNG_Text_clear(LodePNG_Text* text)
{
  size_t i;
  for(i = 0; i < text->num; i++)
  {
    string_cleanup(&text->keys[i]);
    string_cleanup(&text->strings[i]);
  }
  free(text->keys);
  free(text->strings);
}

unsigned LodePNG_Text_add(LodePNG_Text* text, const char* key, const char* str)
{
  char** new_keys = (char**)(realloc(text->keys, sizeof(char*) * (text->num + 1)));
  char** new_strings = (char**)(realloc(text->strings, sizeof(char*) * (text->num + 1)));
  if(!new_keys || !new_strings)
  {
    free(new_keys);
    free(new_strings);
    return 9933; /*alloc fail*/
  }

  text->num++;
  text->keys = new_keys;
  text->strings = new_strings;

  string_init(&text->keys[text->num - 1]);
  string_set(&text->keys[text->num - 1], key);

  string_init(&text->strings[text->num - 1]);
  string_set(&text->strings[text->num - 1], str);

  return 0;
}

/******************************************************************************/

void LodePNG_IText_init(LodePNG_IText* text)
{
  text->num = 0;
  text->keys = NULL;
  text->langtags = NULL;
  text->transkeys = NULL;
  text->strings = NULL;
}

void LodePNG_IText_cleanup(LodePNG_IText* text)
{
  LodePNG_IText_clear(text);
}

unsigned LodePNG_IText_copy(LodePNG_IText* dest, const LodePNG_IText* source)
{
  size_t i = 0;
  dest->keys = 0;
  dest->langtags = 0;
  dest->transkeys = 0;
  dest->strings = 0;
  dest->num = 0;
  for(i = 0; i < source->num; i++)
  {
    unsigned error = LodePNG_IText_add(dest, source->keys[i], source->langtags[i],
                                       source->transkeys[i], source->strings[i]);
    if(error) return error;
  }
  return 0;
}

void LodePNG_IText_clear(LodePNG_IText* text)
{
  size_t i;
  for(i = 0; i < text->num; i++)
  {
    string_cleanup(&text->keys[i]);
    string_cleanup(&text->langtags[i]);
    string_cleanup(&text->transkeys[i]);
    string_cleanup(&text->strings[i]);
  }
  free(text->keys);
  free(text->langtags);
  free(text->transkeys);
  free(text->strings);
}

unsigned LodePNG_IText_add(LodePNG_IText* text, const char* key, const char* langtag,
                           const char* transkey, const char* str)
{
  char** new_keys = (char**)(realloc(text->keys, sizeof(char*) * (text->num + 1)));
  char** new_langtags = (char**)(realloc(text->langtags, sizeof(char*) * (text->num + 1)));
  char** new_transkeys = (char**)(realloc(text->transkeys, sizeof(char*) * (text->num + 1)));
  char** new_strings = (char**)(realloc(text->strings, sizeof(char*) * (text->num + 1)));
  if(!new_keys || !new_langtags || !new_transkeys || !new_strings)
  {
    free(new_keys);
    free(new_langtags);
    free(new_transkeys);
    free(new_strings);
    return 9934; /*alloc fail*/
  }

  text->num++;
  text->keys = new_keys;
  text->langtags = new_langtags;
  text->transkeys = new_transkeys;
  text->strings = new_strings;

  string_init(&text->keys[text->num - 1]);
  string_set(&text->keys[text->num - 1], key);

  string_init(&text->langtags[text->num - 1]);
  string_set(&text->langtags[text->num - 1], langtag);

  string_init(&text->transkeys[text->num - 1]);
  string_set(&text->transkeys[text->num - 1], transkey);

  string_init(&text->strings[text->num - 1]);
  string_set(&text->strings[text->num - 1], str);

  return 0;
}
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

void LodePNG_InfoPng_init(LodePNG_InfoPng* info)
{
  info->width = info->height = 0;
  LodePNG_InfoColor_init(&info->color);
  info->interlaceMethod = 0;
  info->compressionMethod = 0;
  info->filterMethod = 0;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  info->background_defined = 0;
  info->background_r = info->background_g = info->background_b = 0;

  LodePNG_Text_init(&info->text);
  LodePNG_IText_init(&info->itext);

  info->time_defined = 0;
  info->phys_defined = 0;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
  LodePNG_UnknownChunks_init(&info->unknown_chunks);
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
}

void LodePNG_InfoPng_cleanup(LodePNG_InfoPng* info)
{
  LodePNG_InfoColor_cleanup(&info->color);
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  LodePNG_Text_cleanup(&info->text);
  LodePNG_IText_cleanup(&info->itext);
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
  LodePNG_UnknownChunks_cleanup(&info->unknown_chunks);
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
}

unsigned LodePNG_InfoPng_copy(LodePNG_InfoPng* dest, const LodePNG_InfoPng* source)
{
  unsigned error = 0;
  LodePNG_InfoPng_cleanup(dest);
  *dest = *source;
  LodePNG_InfoColor_init(&dest->color);
  error = LodePNG_InfoColor_copy(&dest->color, &source->color); if(error) return error;

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  error = LodePNG_Text_copy(&dest->text, &source->text); if(error) return error;
  error = LodePNG_IText_copy(&dest->itext, &source->itext); if(error) return error;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
  LodePNG_UnknownChunks_init(&dest->unknown_chunks);
  error = LodePNG_UnknownChunks_copy(&dest->unknown_chunks, &source->unknown_chunks); if(error) return error;
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
  return error;
}

void LodePNG_InfoPng_swap(LodePNG_InfoPng* a, LodePNG_InfoPng* b)
{
  LodePNG_InfoPng temp = *a;
  *a = *b;
  *b = temp;
}

unsigned LodePNG_InfoColor_copy(LodePNG_InfoColor* dest, const LodePNG_InfoColor* source)
{
  size_t i;
  LodePNG_InfoColor_cleanup(dest);
  *dest = *source;
  dest->palette = (unsigned char*)malloc(source->palettesize * 4);
  if(!dest->palette && source->palettesize) return 9935; /*alloc fail*/
  for(i = 0; i < source->palettesize * 4; i++) dest->palette[i] = source->palette[i];
  return 0;
}

void LodePNG_InfoRaw_init(LodePNG_InfoRaw* info)
{
  LodePNG_InfoColor_init(&info->color);
}

void LodePNG_InfoRaw_cleanup(LodePNG_InfoRaw* info)
{
  LodePNG_InfoColor_cleanup(&info->color);
}

unsigned LodePNG_InfoRaw_copy(LodePNG_InfoRaw* dest, const LodePNG_InfoRaw* source)
{
  unsigned error = 0;
  LodePNG_InfoRaw_cleanup(dest);
  *dest = *source;
  LodePNG_InfoColor_init(&dest->color);
  error = LodePNG_InfoColor_copy(&dest->color, &source->color);
  return error; /*this variable could be removed, but it's more clear what is returned this way*/
}

/* ////////////////////////////////////////////////////////////////////////// */

/*convert from any color type to RGB or RGBA with 8 bits per sample*/
static unsigned LodePNG_convert_rgb_a_8(unsigned char* out, const unsigned char* in,
                                        LodePNG_InfoColor* infoIn, size_t numpixels, unsigned bytes, unsigned alpha)
{
  size_t i, c, bp = 0; /*bp = bitpointer, used by less-than-8-bit color types*/

  if(infoIn->bitDepth == 8)
  {
    switch(infoIn->colorType)
    {
      case 0: /*greyscale color*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = out[bytes * i + 1] = out[bytes * i + 2] = in[i];
          if(alpha) out[bytes * i + 3] = infoIn->key_defined && in[i] == infoIn->key_r ? 0 : 255;
        }
        break;
      case 2: /*RGB color*/
        for(i = 0; i < numpixels; i++)
        {
          for(c = 0; c < 3; c++) out[bytes * i + c] = in[3 * i + c];
          if(alpha)
          {
            if(infoIn->key_defined == 1 && in[3 * i + 0] == infoIn->key_r
               && in[3 * i + 1] == infoIn->key_g && in[3 * i + 2] == infoIn->key_b)
              out[bytes * i + 3] = 0;
            else out[bytes * i + 3] = 255;
          }
        }
        break;
      case 3: /*indexed color (palette)*/
        for(i = 0; i < numpixels; i++)
        {
          if(in[i] >= infoIn->palettesize) return 46; /*invalid palette index*/
          for(c = 0; c < bytes; c++) out[bytes * i + c] = infoIn->palette[4 * in[i] + c];
        }
        break;
      case 4: /*greyscale with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = out[bytes * i + 1] = out[bytes * i + 2] = in[2 * i + 0];
          if(alpha) out[bytes * i + 3] = in[2 * i + 1];
        }
        break;
      case 6: /*RGB with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          for(c = 0; c < bytes; c++) out[bytes * i + c] = in[4 * i + c];
        }
        break;
      default: break;
    }
  }
  else if(infoIn->bitDepth == 16)
  {
    switch(infoIn->colorType)
    {
      case 0: /*greyscale color*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = out[bytes * i + 1] = out[bytes * i + 2] = in[2 * i];
          if(alpha) out[bytes * i + 3] = infoIn->key_defined && 256U * in[i] + in[i + 1] == infoIn->key_r ? 0 : 255;
        }
        break;
      case 2: /*RGB color*/
        for(i = 0; i < numpixels; i++)
        {
          if(alpha) out[bytes * i + 3] = 255;
          for(c = 0; c < 3; c++) out[bytes * i + c] = in[6 * i + 2 * c];
          if(alpha)
          {
            if(infoIn->key_defined && 256U * in[6 * i + 0] + in[6 * i + 1] == infoIn->key_r
               && 256U * in[6 * i + 2] + in[6 * i + 3] == infoIn->key_g
               && 256U * in[6 * i + 4] + in[6 * i + 5] == infoIn->key_b)
              out[bytes * i + 3] = 0;
            else out[bytes * i + 3] = 255;
          }
        }
        break;
      case 4: /*greyscale with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = out[bytes * i + 1] = out[bytes * i + 2] = in[4 * i];
          if(alpha) out[bytes * i + 3] = in[4 * i + 2];
        }
        break;
      case 6: /*RGB with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          for(c = 0; c < bytes; c++) out[bytes * i + c] = in[8 * i + 2 * c];
        }
        break;
      default: break;
    }
  }
  else /*infoIn->bitDepth is less than 8 bit per channel*/
  {
    switch(infoIn->colorType)
    {
      case 0: /*greyscale color*/
        for(i = 0; i < numpixels; i++)
        {
          unsigned value = readBitsFromReversedStream(&bp, in, infoIn->bitDepth);
          unsigned highest = ((1U << infoIn->bitDepth) - 1U); /*highest possible value for this bit depth*/
          /*scale value from 0 to 255*/
          out[bytes * i + 0] = out[bytes * i + 1] = out[bytes * i + 2]
                             = (unsigned char)((value * 255) / highest);
          if(alpha) out[bytes * i + 3] = infoIn->key_defined && value == infoIn->key_r ? 0 : 255;
        }
      break;
      case 3: /*indexed color (palette)*/
        for(i = 0; i < numpixels; i++)
        {
          unsigned value = readBitsFromReversedStream(&bp, in, infoIn->bitDepth);
          if(value >= infoIn->palettesize) return 47;
          for(c = 0; c < bytes; c++) out[bytes * i + c] = infoIn->palette[4 * value + c];
        }
      break;
      default: break;
    }
  }
  return 0;
}

/*convert from any greyscale color type to 8-bit greyscale with or without alpha channel*/
static unsigned LodePNG_convert_grey_8(unsigned char* out, const unsigned char* in,
                                       LodePNG_InfoColor* infoIn, size_t numpixels, unsigned bytes, unsigned alpha)
{
  size_t i, bp = 0; /*bp = bitpointer, used by less-than-8-bit color types*/

  if(infoIn->bitDepth == 8)
  {
    switch(infoIn->colorType)
    {
      case 0: /*greyscale color*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i] = in[i];
          if(alpha) out[bytes * i + 1] = infoIn->key_defined && in[i] == infoIn->key_r ? 0 : 255;
        }
        break;
      case 4: /*greyscale with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = in[2 * i + 0];
          if(alpha) out[bytes * i + 1] = in[2 * i + 1];
        }
        break;
      default: return 31;
    }
  }
  else if(infoIn->bitDepth == 16)
  {
    switch(infoIn->colorType)
    {
      case 0: /*greyscale color*/
        for(i = 0; i < numpixels; i++)
        {
          if(alpha) out[bytes * i + 1] = 255;
          out[bytes * i] = in[2 * i];
          if(alpha && infoIn->key_defined && 256U * in[i] + in[i + 1] == infoIn->key_r)
          {
            out[bytes * i + 1] = 0;
          }
        }
        break;
      case 4: /*greyscale with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i] = in[4 * i]; /*most significant byte*/
          if(alpha) out[bytes * i + 1] = in[4 * i + 2]; /*most significant byte*/
        }
        break;
      default: return 31;
    }
  }
  else /*infoIn->bitDepth is less than 8 bit per channel*/
  {
    if(infoIn->colorType != 0) return 31; /*colorType 0 is the only greyscale type with < 8 bits per channel*/
    for(i = 0; i < numpixels; i++)
    {
      unsigned value = readBitsFromReversedStream(&bp, in, infoIn->bitDepth);
      unsigned highest = ((1U << infoIn->bitDepth) - 1U); /*highest possible value for this bit depth*/
      out[bytes * i] = (unsigned char)((value * 255) / highest); /*scale value from 0 to 255*/
      if(alpha)
      {
        if(infoIn->key_defined && value == infoIn->key_r) out[bytes * i + 1] = 0;
        else out[bytes * i + 1] = 255;
      }
    }
  }
  return 0;
}

/*convert from any color type to RGB or RGBA with 8 bits per sample*/
static unsigned LodePNG_convert_rgb_a_16(unsigned char* out, const unsigned char* in,
                                         LodePNG_InfoColor* infoIn, size_t numpixels, unsigned bytes, unsigned alpha)
{
  size_t i, c, bp = 0; /*bp = bitpointer, used by less-than-8-bit color types*/

  if(infoIn->bitDepth == 8)
  {
    switch(infoIn->colorType)
    {
      case 0: /*greyscale color*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = out[bytes * i + 2] = out[bytes * i + 4] = in[i];
          out[bytes * i + 1] = out[bytes * i + 3] = out[bytes * i + 5] = in[i];
          if(alpha)
          {
            if(alpha && infoIn->key_defined && in[i] == infoIn->key_r) out[bytes * i + 6] = out[bytes * i + 7] = 0;
            else out[bytes * i + 6] = out[bytes * i + 7] = 255;
          }
        }
        break;
      case 2: /*RGB color*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = out[bytes * i + 1] = in[3 * i + 0];
          out[bytes * i + 2] = out[bytes * i + 3] = in[3 * i + 1];
          out[bytes * i + 4] = out[bytes * i + 5] = in[3 * i + 2];
          if(alpha)
          {
            if(infoIn->key_defined == 1 && in[3 * i + 0] == infoIn->key_r
               && in[3 * i + 1] == infoIn->key_g && in[3 * i + 2] == infoIn->key_b)
              out[bytes * i + 6] = out[bytes * i + 7] = 0;
            else out[bytes * i + 6] = out[bytes * i + 7] = 255;
          }
        }
        break;
      case 3: /*indexed color (palette)*/
        for(i = 0; i < numpixels; i++)
        {
          if(in[i] >= infoIn->palettesize) return 46; /*invalid palette index*/
          for(c = 0; c < bytes; c++)
          {
            out[bytes * i + 2 * c] = out[bytes * i + 2 * c + 1] = infoIn->palette[4 * in[i] + c];
          }
        }
        break;
      case 4: /*greyscale with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = out[bytes * i + 2] = out[bytes * i + 4] = in[i];
          out[bytes * i + 1] = out[bytes * i + 3] = out[bytes * i + 5] = in[i];
          if(alpha)
          {
            out[bytes * i + 6] = out[bytes * i + 7] = in[2 * i + 1];
          }
        }
        break;
      case 6: /*RGB with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          for(c = 0; c < 3; c++)
          {
            out[bytes * i + 2 * c] = out[bytes * i + 2 * c + 1] = in[4 * i + c];
          }
          if(alpha)
          {
            out[bytes * i + 6] = out[bytes * i + 7] = in[4 * i + c];
          }
        }
        break;
      default: break;
    }
  }
  else if(infoIn->bitDepth == 16)
  {
    switch(infoIn->colorType)
    {
      case 0: /*greyscale color*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = out[bytes * i + 2] = out[bytes * i + 4] = in[2 * i];
          out[bytes * i + 1] = out[bytes * i + 3] = out[bytes * i + 5] = in[2 * i + 1];
          if(alpha)
          {
            if(infoIn->key_defined && 256U * in[i] + in[i + 1] == infoIn->key_r)
              out[bytes * i + 6] = out[bytes * i + 7] = 0;
            else out[bytes * i + 6] = out[bytes * i + 7] = 255;
          }
        }
        break;
      case 2: /*RGB color*/
        for(i = 0; i < numpixels; i++)
        {
          for(c = 0; c < 6; c++) out[bytes * i + c] = in[6 * i + c];
          if(alpha)
          {
            if(infoIn->key_defined && 256U * in[6 * i + 0] + in[6 * i + 1] == infoIn->key_r
               && 256U * in[6 * i + 2] + in[6 * i + 3] == infoIn->key_g
               && 256U * in[6 * i + 4] + in[6 * i + 5] == infoIn->key_b)
              out[bytes * i + 6] = out[bytes * i + 7] = 0;
            else out[bytes * i + 6] = out[bytes * i + 7] = 255;
          }
        }
        break;
      case 4: /*greyscale with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = out[bytes * i + 2] = out[bytes * i + 4] = in[4 * i + 0];
          out[bytes * i + 1] = out[bytes * i + 3] = out[bytes * i + 5] = in[4 * i + 1];
          if(alpha)
          {
            out[bytes * i + 6] = in[4 * i + 2];
            out[bytes * i + 7] = in[4 * i + 3];
          }
        }
        break;
      case 6: /*RGB with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          for(c = 0; c < bytes; c++) out[bytes * i + c] = in[8 * i + 2 * c];
        }
        break;
      default: break;
    }
  }
  else /*infoIn->bitDepth is less than 8 bit per channel*/
  {
    switch(infoIn->colorType)
    {
      case 0: /*greyscale color*/
        for(i = 0; i < numpixels; i++)
        {
          unsigned value = readBitsFromReversedStream(&bp, in, infoIn->bitDepth);
          unsigned highest = ((1U << infoIn->bitDepth) - 1U); /*highest possible value for this bit depth*/
          /*scale value from 0 to 255*/
          out[bytes * i + 0] = out[bytes * i + 2] = out[bytes * i + 4]
                             = out[bytes * i + 1] = out[bytes * i + 3]
                             = out[bytes * i + 5] = (unsigned char)((value * 255) / highest);
          if(alpha)
          {
            if(infoIn->key_defined && value == infoIn->key_r) out[bytes * i + 6] = out[bytes * i + 7] = 0;
            else out[bytes * i + 6] = out[bytes * i + 7] = 255;
          }
        }
        break;
      case 3: /*indexed color (palette)*/
        for(i = 0; i < numpixels; i++)
        {
          unsigned value = readBitsFromReversedStream(&bp, in, infoIn->bitDepth);
          if(value >= infoIn->palettesize) return 47;
          for(c = 0; c < bytes / 2; c++)
          {
            out[bytes * i + c * 2 + 0] = out[bytes * i + c * 2 + 1] = infoIn->palette[4 * value + c];
          }
        }
        break;
      default: break;
    }
  }
  return 0;
}

/*convert from any greyscale color type to 16-bit greyscale with or without alpha channel*/
static unsigned LodePNG_convert_grey_16(unsigned char* out, const unsigned char* in,
                                        LodePNG_InfoColor* infoIn, size_t numpixels, unsigned bytes, unsigned alpha)
{
  size_t i, c, bp = 0; /*bp = bitpointer, used by less-than-8-bit color types*/

  if(infoIn->bitDepth == 8)
  {
    switch(infoIn->colorType)
    {
      case 0: /*greyscale color*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i] = out[bytes * i + 1] = in[i];
          if(alpha)
          {
            if(infoIn->key_defined && in[i] == infoIn->key_r) out[bytes * i + 2] = out[bytes * i + 3] = 0;
            else out[bytes * i + 2] = out[bytes * i + 3] = 255;
          }
        }
        break;
      case 4: /*greyscale with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = out[bytes * i + 1] = in[2 * i + 0];
          if(alpha) out[bytes * i + 2] = out[bytes * i + 3] = in[2 * i + 1];
        }
        break;
      default: return 31;
    }
  }
  else if(infoIn->bitDepth == 16)
  {
    switch(infoIn->colorType)
    {
      case 0: /*greyscale color*/
        for(i = 0; i < numpixels; i++)
        {
          out[bytes * i + 0] = in[2 * i + 0];
          out[bytes * i + 1] = in[2 * i + 1];
          if(alpha)
          {
            if(infoIn->key_defined && 256U * in[i] + in[i + 1] == infoIn->key_r)
              out[bytes * i + 2] = out[bytes * i + 3] = 0;
            else out[bytes * i + 2] = out[bytes * i + 3] = 255;
          }
        }
        break;
      case 4: /*greyscale with alpha*/
        for(i = 0; i < numpixels; i++)
        {
          for(c = 0; c < bytes; c++) out[bytes * i + c] = in[4 * i + c];
        }
        break;
      default: return 31;
    }
  }
  else /*infoIn->bitDepth is less than 8 bit per channel*/
  {
    if(infoIn->colorType != 0) return 31; /*colorType 0 is the only greyscale type with < 8 bits per channel*/
    for(i = 0; i < numpixels; i++)
    {
      unsigned value = readBitsFromReversedStream(&bp, in, infoIn->bitDepth);
      unsigned highest = ((1U << infoIn->bitDepth) - 1U); /*highest possible value for this bit depth*/
      out[bytes * i] = out[bytes * i + 1] = (unsigned char)((value * 255) / highest); /*scale value from 0 to 255*/
      if(alpha)
      {
        if(infoIn->key_defined && value == infoIn->key_r) out[bytes * i + 2] = out[bytes * i + 3] = 0;
        else out[bytes * i + 2] = out[bytes * i + 3] = 255;
      }
    }
  }
  return 0;
}

/*
converts from any color type to 24-bit or 32-bit (later maybe more supported). return value = LodePNG error code
the out buffer must have (w * h * bpp + 7) / 8 bytes, where bpp is the bits per pixel of the output color type
(LodePNG_InfoColor_getBpp) for < 8 bpp images, there may _not_ be padding bits at the end of scanlines.
*/
unsigned LodePNG_convert(unsigned char* out, const unsigned char* in, LodePNG_InfoColor* infoOut,
                         LodePNG_InfoColor* infoIn, unsigned w, unsigned h)
{
  size_t numpixels = w * h; /*amount of pixels*/
  unsigned bytes = LodePNG_InfoColor_getBpp(infoOut) / 8; /*bytes per pixel in the output image*/
  unsigned alpha = LodePNG_InfoColor_isAlphaType(infoOut); /*use 8-bit alpha channel*/
  
  /*cases where in and out already have the same format*/
  if(LodePNG_InfoColor_equal(infoIn, infoOut))
  {
    size_t i;
    size_t size = (numpixels * LodePNG_InfoColor_getBpp(infoIn) + 7) / 8;
    for(i = 0; i < size; i++) out[i] = in[i];
    return 0;
  }
  else if((infoOut->colorType == 2 || infoOut->colorType == 6) && infoOut->bitDepth == 8)
  {
    LodePNG_convert_rgb_a_8(out, in, infoIn, numpixels, bytes, alpha);
  }
  else if(LodePNG_InfoColor_isGreyscaleType(infoOut) && infoOut->bitDepth == 8)
  {
    /*conversion from greyscale to greyscale*/
    if(!LodePNG_InfoColor_isGreyscaleType(infoIn)) return 62; /*converting from color to grey is not supported*/
    LodePNG_convert_grey_8(out, in, infoIn, numpixels, bytes, alpha);
  }
  else if((infoOut->colorType == 2 || infoOut->colorType == 6) && infoOut->bitDepth == 16)
  {
    LodePNG_convert_rgb_a_16(out, in, infoIn, numpixels, bytes, alpha);
  }
  else if(LodePNG_InfoColor_isGreyscaleType(infoOut) && infoOut->bitDepth == 16)
  {
    /*conversion from greyscale to greyscale*/
    if(!LodePNG_InfoColor_isGreyscaleType(infoIn)) return 62; /*converting from color to grey is not supported*/
    LodePNG_convert_grey_16(out, in, infoIn, numpixels, bytes, alpha);
  }
  else return 59; /*invalid color mode*/
  return 0;
}

/*
Paeth predicter, used by PNG filter type 4
The parameters are of type short, but should come from unsigned chars, the shorts
are only needed to make the paeth calculation correct.
*/
static unsigned char paethPredictor(short a, short b, short c)
{
  short pa = abs(b - c);
  short pb = abs(a - c);
  short pc = abs(a + b - c - c);

  /*short pc = a + b - c;
  short pa = abs(pc - a);
  short pb = abs(pc - b);
  pc = abs(pc - c);*/

  if(pa <= pb && pa <= pc) return (unsigned char)a;
  else if(pb <= pc) return (unsigned char)b;
  else return (unsigned char)c;
}

/*shared values used by multiple Adam7 related functions*/

static const unsigned ADAM7_IX[7] = { 0, 4, 0, 2, 0, 1, 0 }; /*x start values*/
static const unsigned ADAM7_IY[7] = { 0, 0, 4, 0, 2, 0, 1 }; /*y start values*/
static const unsigned ADAM7_DX[7] = { 8, 8, 4, 4, 2, 2, 1 }; /*x delta values*/
static const unsigned ADAM7_DY[7] = { 8, 8, 8, 4, 4, 2, 2 }; /*y delta values*/

static void Adam7_getpassvalues(unsigned passw[7], unsigned passh[7], size_t filter_passstart[8],
                                size_t padded_passstart[8], size_t passstart[8], unsigned w, unsigned h, unsigned bpp)
{
  /*the passstart values have 8 values: the 8th one indicates the byte after the end of the 7th (= last) pass*/
  unsigned i;

  /*calculate width and height in pixels of each pass*/
  for(i = 0; i < 7; i++)
  {
    passw[i] = (w + ADAM7_DX[i] - ADAM7_IX[i] - 1) / ADAM7_DX[i];
    passh[i] = (h + ADAM7_DY[i] - ADAM7_IY[i] - 1) / ADAM7_DY[i];
    if(passw[i] == 0) passh[i] = 0;
    if(passh[i] == 0) passw[i] = 0;
  }

  filter_passstart[0] = padded_passstart[0] = passstart[0] = 0;
  for(i = 0; i < 7; i++)
  {
    /*if passw[i] is 0, it's 0 bytes, not 1 (no filtertype-byte)*/
    filter_passstart[i + 1] = filter_passstart[i]
                            + ((passw[i] && passh[i]) ? passh[i] * (1 + (passw[i] * bpp + 7) / 8) : 0);
    /*bits padded if needed to fill full byte at end of each scanline*/
    padded_passstart[i + 1] = padded_passstart[i] + passh[i] * ((passw[i] * bpp + 7) / 8);
    /*only padded at end of reduced image*/
    passstart[i + 1] = passstart[i] + (passh[i] * passw[i] * bpp + 7) / 8;
  }
}

#ifdef LODEPNG_COMPILE_DECODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG Decoder                                                            / */
/* ////////////////////////////////////////////////////////////////////////// */

/*read the information from the header and store it in the LodePNG_Info. return value is error*/
void LodePNG_Decoder_inspect(LodePNG_Decoder* decoder, const unsigned char* in, size_t inlength)
{
  if(inlength == 0 || in == 0)
  {
    decoder->error = 48; /*the given data is empty*/
    return;
  }
  if(inlength < 29)
  {
    decoder->error = 27; /*error: the data length is smaller than the length of a PNG header*/
    return;
  }

  /*when decoding a new PNG image, make sure all parameters created after previous decoding are reset*/
  LodePNG_InfoPng_cleanup(&decoder->infoPng);
  LodePNG_InfoPng_init(&decoder->infoPng);
  decoder->error = 0;

  if(in[0] != 137 || in[1] != 80 || in[2] != 78 || in[3] != 71
     || in[4] != 13 || in[5] != 10 || in[6] != 26 || in[7] != 10)
  {
    decoder->error = 28; /*error: the first 8 bytes are not the correct PNG signature*/
    return;
  }
  if(in[12] != 'I' || in[13] != 'H' || in[14] != 'D' || in[15] != 'R')
  {
    decoder->error = 29; /*error: it doesn't start with a IHDR chunk!*/
    return;
  }

  /*read the values given in the header*/
  decoder->infoPng.width = LodePNG_read32bitInt(&in[16]);
  decoder->infoPng.height = LodePNG_read32bitInt(&in[20]);
  decoder->infoPng.color.bitDepth = in[24];
  decoder->infoPng.color.colorType = in[25];
  decoder->infoPng.compressionMethod = in[26];
  decoder->infoPng.filterMethod = in[27];
  decoder->infoPng.interlaceMethod = in[28];

  if(!decoder->settings.ignoreCrc)
  {
    unsigned CRC = LodePNG_read32bitInt(&in[29]);
    unsigned checksum = Crc32_crc(&in[12], 17);
    if(CRC != checksum)
    {
      decoder->error = 57; /*invalid CRC*/
      return;
    }
  }

  /*error: only compression method 0 is allowed in the specification*/
  if(decoder->infoPng.compressionMethod != 0) { decoder->error = 32; return; }
  /*error: only filter method 0 is allowed in the specification*/
  if(decoder->infoPng.filterMethod != 0)      { decoder->error = 33; return; }
  /*error: only interlace methods 0 and 1 exist in the specification*/
  if(decoder->infoPng.interlaceMethod > 1)    { decoder->error = 34; return; }

  decoder->error = checkColorValidity(decoder->infoPng.color.colorType, decoder->infoPng.color.bitDepth);
}

static unsigned unfilterScanline(unsigned char* recon, const unsigned char* scanline, const unsigned char* precon,
                                 size_t bytewidth, unsigned char filterType, size_t length)
{
  /*
  For PNG filter method 0
  unfilter a PNG image scanline by scanline. when the pixels are smaller than 1 byte,
  the filter works byte per byte (bytewidth = 1)
  precon is the previous unfiltered scanline, recon the result, scanline the current one
  the incoming scanlines do NOT include the filtertype byte, that one is given in the parameter filterType instead
  recon and scanline MAY be the same memory address! precon must be disjoint.
  */

  size_t i;
  switch(filterType)
  {
    case 0:
      for(i = 0; i < length; i++) recon[i] = scanline[i];
      break;
    case 1:
      for(i =         0; i < bytewidth; i++) recon[i] = scanline[i];
      for(i = bytewidth; i <    length; i++) recon[i] = scanline[i] + recon[i - bytewidth];
      break;
    case 2:
      if(precon)
      {
        for(i = 0; i < length; i++) recon[i] = scanline[i] + precon[i];
      }
      else
      {
        for(i = 0; i < length; i++) recon[i] = scanline[i];
      }
      break;
    case 3:
      if(precon)
      {
        for(i =         0; i < bytewidth; i++) recon[i] = scanline[i] + precon[i] / 2;
        for(i = bytewidth; i <    length; i++) recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
      }
      else
      {
        for(i =         0; i < bytewidth; i++) recon[i] = scanline[i];
        for(i = bytewidth; i <    length; i++) recon[i] = scanline[i] + recon[i - bytewidth] / 2;
      }
      break;
    case 4:
      if(precon)
      {
        for(i = 0; i < bytewidth; i++)
        {
          recon[i] = (scanline[i] + precon[i]); /*paethPredictor(0, precon[i], 0) is always precon[i]*/
        }
        for(i = bytewidth; i < length; i++)
        {
          recon[i] = (scanline[i] + paethPredictor(recon[i - bytewidth], precon[i], precon[i - bytewidth]));
        }
      }
      else
      {
        for(i = 0; i < bytewidth; i++)
        {
          recon[i] = scanline[i];
        }
        for(i = bytewidth; i < length; i++)
        {
          /*paethPredictor(recon[i - bytewidth], 0, 0) is always recon[i - bytewidth]*/
          recon[i] = (scanline[i] + recon[i - bytewidth]);
        }
      }
      break;
    default: return 36; /*error: unexisting filter type given*/
  }
  return 0;
}

static unsigned unfilter(unsigned char* out, const unsigned char* in, unsigned w, unsigned h, unsigned bpp)
{
  /*
  For PNG filter method 0
  this function unfilters a single image (e.g. without interlacing this is called once, with Adam7 seven times)
  out must have enough bytes allocated already, in must have the scanlines + 1 filtertype byte per scanline
  w and h are image dimensions or dimensions of reduced image, bpp is bits per pixel
  in and out are allowed to be the same memory address (but aren't the same size since in has the extra filter bytes)
  */

  unsigned y;
  unsigned char* prevline = 0;

  /*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise*/
  size_t bytewidth = (bpp + 7) / 8;
  size_t linebytes = (w * bpp + 7) / 8;

  for(y = 0; y < h; y++)
  {
    size_t outindex = linebytes * y;
    size_t inindex = (1 + linebytes) * y; /*the extra filterbyte added to each row*/
    unsigned char filterType = in[inindex];

    unsigned error = unfilterScanline(&out[outindex], &in[inindex + 1], prevline, bytewidth, filterType, linebytes);
    if(error) return error;

    prevline = &out[outindex];
  }

  return 0;
}

static void Adam7_deinterlace(unsigned char* out, const unsigned char* in, unsigned w, unsigned h, unsigned bpp)
{
  /*Note: this function works on image buffers WITHOUT padding bits at end of scanlines
  with non-multiple-of-8 bit amounts, only between reduced images is padding
  out must be big enough AND must be 0 everywhere if bpp < 8 in the current implementation
  (because that's likely a little bit faster)*/
  unsigned passw[7], passh[7];
  size_t filter_passstart[8], padded_passstart[8], passstart[8];
  unsigned i;

  Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

  if(bpp >= 8)
  {
    for(i = 0; i < 7; i++)
    {
      unsigned x, y, b;
      size_t bytewidth = bpp / 8;
      for(y = 0; y < passh[i]; y++)
      for(x = 0; x < passw[i]; x++)
      {
        size_t pixelinstart = passstart[i] + (y * passw[i] + x) * bytewidth;
        size_t pixeloutstart = ((ADAM7_IY[i] + y * ADAM7_DY[i]) * w + ADAM7_IX[i] + x * ADAM7_DX[i]) * bytewidth;
        for(b = 0; b < bytewidth; b++)
        {
          out[pixeloutstart + b] = in[pixelinstart + b];
        }
      }
    }
  }
  else /*bpp < 8: Adam7 with pixels < 8 bit is a bit trickier: with bit pointers*/
  {
    for(i = 0; i < 7; i++)
    {
      unsigned x, y, b;
      unsigned ilinebits = bpp * passw[i];
      unsigned olinebits = bpp * w;
      size_t obp, ibp; /*bit pointers (for out and in buffer)*/
      for(y = 0; y < passh[i]; y++)
      for(x = 0; x < passw[i]; x++)
      {
        ibp = (8 * passstart[i]) + (y * ilinebits + x * bpp);
        obp = (ADAM7_IY[i] + y * ADAM7_DY[i]) * olinebits + (ADAM7_IX[i] + x * ADAM7_DX[i]) * bpp;
        for(b = 0; b < bpp; b++)
        {
          unsigned char bit = readBitFromReversedStream(&ibp, in);
          /*note that this function assumes the out buffer is completely 0, use setBitOfReversedStream otherwise*/
          setBitOfReversedStream0(&obp, out, bit);
        }
      }
    }
  }
}

static void removePaddingBits(unsigned char* out, const unsigned char* in,
                              size_t olinebits, size_t ilinebits, unsigned h)
{
  /*
  After filtering there are still padding bits if scanlines have non multiple of 8 bit amounts. They need
  to be removed (except at last scanline of (Adam7-reduced) image) before working with pure image buffers
  for the Adam7 code, the color convert code and the output to the user.
  in and out are allowed to be the same buffer, in may also be higher but still overlapping; in must
  have >= ilinebits*h bits, out must have >= olinebits*h bits, olinebits must be <= ilinebits
  also used to move bits after earlier such operations happened, e.g. in a sequence of reduced images from Adam7
  only useful if (ilinebits - olinebits) is a value in the range 1..7
  */
  unsigned y;
  size_t diff = ilinebits - olinebits;
  size_t ibp = 0, obp = 0; /*input and output bit pointers*/
  for(y = 0; y < h; y++)
  {
    size_t x;
    for(x = 0; x < olinebits; x++)
    {
      unsigned char bit = readBitFromReversedStream(&ibp, in);
      setBitOfReversedStream(&obp, out, bit);
    }
    ibp += diff;
  }
}

/*out must be buffer big enough to contain full image, and in must contain the full decompressed data from
the IDAT chunks (with filter index bytes and possible padding bits)
return value is error*/
static unsigned postProcessScanlines(unsigned char* out, unsigned char* in, const LodePNG_InfoPng* infoPng)
{
  /*
  This function converts the filtered-padded-interlaced data into pure 2D image buffer with the PNG's colortype.
  Steps:
  *) if no Adam7: 1) unfilter 2) remove padding bits (= posible extra bits per scanline if bpp < 8)
  *) if adam7: 1) 7x unfilter 2) 7x remove padding bits 3) Adam7_deinterlace
  NOTE: the in buffer will be overwritten with intermediate data!
  */
  unsigned bpp = LodePNG_InfoColor_getBpp(&infoPng->color);
  unsigned w = infoPng->width;
  unsigned h = infoPng->height;
  unsigned error = 0;
  if(bpp == 0) return 31; /*error: invalid colortype*/

  if(infoPng->interlaceMethod == 0)
  {
    if(bpp < 8 && w * bpp != ((w * bpp + 7) / 8) * 8)
    {
      error = unfilter(in, in, w, h, bpp);
      if(error) return error;
      removePaddingBits(out, in, w * bpp, ((w * bpp + 7) / 8) * 8, h);
    }
    else error = unfilter(out, in, w, h, bpp); /*we can immediatly filter into the out buffer, no other steps needed*/
  }
  else /*interlaceMethod is 1 (Adam7)*/
  {
    unsigned passw[7], passh[7]; size_t filter_passstart[8], padded_passstart[8], passstart[8];
    unsigned i;

    Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

    for(i = 0; i < 7; i++)
    {
      error = unfilter(&in[padded_passstart[i]], &in[filter_passstart[i]], passw[i], passh[i], bpp);
      if(error) return error;
      /*TODO: possible efficiency improvement: if in this reduced image the bits fit nicely in 1 scanline,
      move bytes instead of bits or move not at all*/
      if(bpp < 8)
      {
        /*remove padding bits in scanlines; after this there still may be padding
        bits between the different reduced images: each reduced image still starts nicely at a byte*/
        removePaddingBits(&in[passstart[i]], &in[padded_passstart[i]], passw[i] * bpp,
                          ((passw[i] * bpp + 7) / 8) * 8, passh[i]);
      }
    }

    Adam7_deinterlace(out, in, w, h, bpp);
  }

  return error;
}

/*read a PNG, the result will be in the same color type as the PNG (hence "generic")*/
static void decodeGeneric(LodePNG_Decoder* decoder, unsigned char** out, size_t* outsize,
                          const unsigned char* in, size_t insize)
{
  unsigned char IEND = 0;
  const unsigned char* chunk;
  size_t i;
  ucvector idat; /*the data from idat chunks*/

  /*for unknown chunk order*/
  unsigned unknown = 0;
#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
  unsigned critical_pos = 1; /*1 = after IHDR, 2 = after PLTE, 3 = after IDAT*/
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

  /*provide some proper output values if error will happen*/
  *out = 0;
  *outsize = 0;

  LodePNG_Decoder_inspect(decoder, in, insize); /*reads header and resets other parameters in decoder->infoPng*/
  if(decoder->error) return;

  ucvector_init(&idat);

  chunk = &in[33]; /*first byte of the first chunk after the header*/

  /*loop through the chunks, ignoring unknown chunks and stopping at IEND chunk.
  IDAT data is put at the start of the in buffer*/
  while(!IEND)
  {
    unsigned chunkLength;
    const unsigned char* data; /*the data in the chunk*/

    /*error: size of the in buffer too small to contain next chunk*/
    if((size_t)((chunk - in) + 12) > insize || chunk < in) CERROR_BREAK(decoder->error, 30);

    /*length of the data of the chunk, excluding the length bytes, chunk type and CRC bytes*/
    chunkLength = LodePNG_chunk_length(chunk);
    /*error: chunk length larger than the max PNG chunk size*/
    if(chunkLength > 2147483647) CERROR_BREAK(decoder->error, 63);

    if((size_t)((chunk - in) + chunkLength + 12) > insize || (chunk + chunkLength + 12) < in)
    {
      CERROR_BREAK(decoder->error, 64); /*error: size of the in buffer too small to contain next chunk*/
    }

    data = LodePNG_chunk_data_const(chunk);

    /*IDAT chunk, containing compressed image data*/
    if(LodePNG_chunk_type_equals(chunk, "IDAT"))
    {
      size_t oldsize = idat.size;
      if(!ucvector_resize(&idat, oldsize + chunkLength)) CERROR_BREAK(decoder->error, 9936 /*alloc fail*/);
      for(i = 0; i < chunkLength; i++) idat.data[oldsize + i] = data[i];
#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
      critical_pos = 3;
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
    }
    /*IEND chunk*/
    else if(LodePNG_chunk_type_equals(chunk, "IEND"))
    {
      IEND = 1;
    }
    /*palette chunk (PLTE)*/
    else if(LodePNG_chunk_type_equals(chunk, "PLTE"))
    {
      unsigned pos = 0;
      if(decoder->infoPng.color.palette) free(decoder->infoPng.color.palette);
      decoder->infoPng.color.palettesize = chunkLength / 3;
      decoder->infoPng.color.palette = (unsigned char*)malloc(4 * decoder->infoPng.color.palettesize);
      if(!decoder->infoPng.color.palette && decoder->infoPng.color.palettesize)
      {
        decoder->infoPng.color.palettesize = 0;
        CERROR_BREAK(decoder->error, 9937); /*alloc fail*/
      }
      if(decoder->infoPng.color.palettesize > 256) CERROR_BREAK(decoder->error, 38); /*error: palette too big*/

      for(i = 0; i < decoder->infoPng.color.palettesize; i++)
      {
        decoder->infoPng.color.palette[4 * i + 0] = data[pos++]; /*R*/
        decoder->infoPng.color.palette[4 * i + 1] = data[pos++]; /*G*/
        decoder->infoPng.color.palette[4 * i + 2] = data[pos++]; /*B*/
        decoder->infoPng.color.palette[4 * i + 3] = 255; /*alpha*/
      }
#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
      critical_pos = 2;
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
    }
    /*palette transparency chunk (tRNS)*/
    else if(LodePNG_chunk_type_equals(chunk, "tRNS"))
    {
      if(decoder->infoPng.color.colorType == 3)
      {
        /*error: more alpha values given than there are palette entries*/
        if(chunkLength > decoder->infoPng.color.palettesize) CERROR_BREAK(decoder->error, 38);

        for(i = 0; i < chunkLength; i++) decoder->infoPng.color.palette[4 * i + 3] = data[i];
      }
      else if(decoder->infoPng.color.colorType == 0)
      {
        /*error: this chunk must be 2 bytes for greyscale image*/
        if(chunkLength != 2) CERROR_BREAK(decoder->error, 30);

        decoder->infoPng.color.key_defined = 1;
        decoder->infoPng.color.key_r = decoder->infoPng.color.key_g
                                     = decoder->infoPng.color.key_b = 256 * data[0] + data[1];
      }
      else if(decoder->infoPng.color.colorType == 2)
      {
        /*error: this chunk must be 6 bytes for RGB image*/
        if(chunkLength != 6) CERROR_BREAK(decoder->error, 41);

        decoder->infoPng.color.key_defined = 1;
        decoder->infoPng.color.key_r = 256 * data[0] + data[1];
        decoder->infoPng.color.key_g = 256 * data[2] + data[3];
        decoder->infoPng.color.key_b = 256 * data[4] + data[5];
      }
      else CERROR_BREAK(decoder->error, 42); /*error: tRNS chunk not allowed for other color models*/
    }
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*background color chunk (bKGD)*/
    else if(LodePNG_chunk_type_equals(chunk, "bKGD"))
    {
      if(decoder->infoPng.color.colorType == 3)
      {
        /*error: this chunk must be 1 byte for indexed color image*/
        if(chunkLength != 1) CERROR_BREAK(decoder->error, 43);

        decoder->infoPng.background_defined = 1;
        decoder->infoPng.background_r = decoder->infoPng.background_g = decoder->infoPng.background_b = data[0];
      }
      else if(decoder->infoPng.color.colorType == 0 || decoder->infoPng.color.colorType == 4)
      {
        /*error: this chunk must be 2 bytes for greyscale image*/
        if(chunkLength != 2) CERROR_BREAK(decoder->error, 44);

        decoder->infoPng.background_defined = 1;
        decoder->infoPng.background_r = decoder->infoPng.background_g = decoder->infoPng.background_b
                                      = 256 * data[0] + data[1];
      }
      else if(decoder->infoPng.color.colorType == 2 || decoder->infoPng.color.colorType == 6)
      {
        /*error: this chunk must be 6 bytes for greyscale image*/
        if(chunkLength != 6) CERROR_BREAK(decoder->error, 45);

        decoder->infoPng.background_defined = 1;
        decoder->infoPng.background_r = 256 * data[0] + data[1];
        decoder->infoPng.background_g = 256 * data[2] + data[3];
        decoder->infoPng.background_b = 256 * data[4] + data[5];
      }
    }
    /*text chunk (tEXt)*/
    else if(LodePNG_chunk_type_equals(chunk, "tEXt"))
    {
      if(decoder->settings.readTextChunks)
      {
        char *key = 0, *str = 0;

        while(!decoder->error) /*not really a while loop, only used to break on error*/
        {
          unsigned length, string2_begin;

          length = 0;
          while(length < chunkLength && data[length] != 0) length++;
          /*error, end reached, no null terminator?*/
          if(length + 1 >= chunkLength) CERROR_BREAK(decoder->error, 75);

          key = (char*)malloc(length + 1);
          if(!key) CERROR_BREAK(decoder->error, 9938); /*alloc fail*/

          key[length] = 0;
          for(i = 0; i < length; i++) key[i] = data[i];

          string2_begin = length + 1;
          /*error, end reached, no null terminator?*/
          if(string2_begin > chunkLength) CERROR_BREAK(decoder->error, 75);

          length = chunkLength - string2_begin;
          str = (char*)malloc(length + 1);
          if(!str) CERROR_BREAK(decoder->error, 9939); /*alloc fail*/

          str[length] = 0;
          for(i = 0; i < length; i++) str[i] = data[string2_begin + i];

          decoder->error = LodePNG_Text_add(&decoder->infoPng.text, key, str);

          break;
        }

        free(key);
        free(str);
      }
    }
    /*compressed text chunk (zTXt)*/
    else if(LodePNG_chunk_type_equals(chunk, "zTXt"))
    {
      if(decoder->settings.readTextChunks)
      {
        unsigned length, string2_begin;
        char *key = 0;
        ucvector decoded;

        ucvector_init(&decoded);

        while(!decoder->error) /*not really a while loop, only used to break on error*/
        {
          for(length = 0; length < chunkLength && data[length] != 0; length++) ;
          if(length + 2 >= chunkLength) CERROR_BREAK(decoder->error, 75); /*no null termination, corrupt?*/

          key = (char*)malloc(length + 1);
          if(!key) CERROR_BREAK(decoder->error, 9940); /*alloc fail*/

          key[length] = 0;
          for(i = 0; i < length; i++) key[i] = data[i];

          if(data[length + 1] != 0) CERROR_BREAK(decoder->error, 72); /*the 0 byte indicating compression must be 0*/

          string2_begin = length + 2;
          if(string2_begin > chunkLength) CERROR_BREAK(decoder->error, 75); /*no null termination, corrupt?*/

          length = chunkLength - string2_begin;
          decoder->error = LodePNG_zlib_decompress(&decoded.data, &decoded.size,
                                                   (unsigned char*)(&data[string2_begin]),
                                                   length, &decoder->settings.zlibsettings);
          if(decoder->error) break;
          ucvector_push_back(&decoded, 0);

          decoder->error = LodePNG_Text_add(&decoder->infoPng.text, key, (char*)decoded.data);

          break;
        }

        free(key);
        ucvector_cleanup(&decoded);
        if(decoder->error) break;
      }
    }
    /*international text chunk (iTXt)*/
    else if(LodePNG_chunk_type_equals(chunk, "iTXt"))
    {
      if(decoder->settings.readTextChunks)
      {
        unsigned length, begin, compressed;
        char *key = 0, *langtag = 0, *transkey = 0;
        ucvector decoded;
        ucvector_init(&decoded);

        while(!decoder->error) /*not really a while loop, only used to break on error*/
        {
          /*Quick check if the chunk length isn't too small. Even without check
          it'd still fail with other error checks below if it's too short. This just gives a different error code.*/
          if(chunkLength < 5) CERROR_BREAK(decoder->error, 30); /*iTXt chunk too short*/

          /*read the key*/
          for(length = 0; length < chunkLength && data[length] != 0; length++) ;
          if(length + 2 >= chunkLength) CERROR_BREAK(decoder->error, 75); /*no null termination char found*/

          key = (char*)malloc(length + 1);
          if(!key) CERROR_BREAK(decoder->error, 9941); /*alloc fail*/

          key[length] = 0;
          for(i = 0; i < length; i++) key[i] = data[i];

          /*read the compression method*/
          compressed = data[length + 1];
          if(data[length + 2] != 0) CERROR_BREAK(decoder->error, 72); /*the 0 byte indicating compression must be 0*/

          /*read the langtag*/
          begin = length + 3;
          length = 0;
          for(i = begin; i < chunkLength && data[i] != 0; i++) length++;
          if(begin + length + 1 >= chunkLength) CERROR_BREAK(decoder->error, 75); /*no null termination char found*/

          langtag = (char*)malloc(length + 1);
          if(!langtag) CERROR_BREAK(decoder->error, 9942); /*alloc fail*/

          langtag[length] = 0;
          for(i = 0; i < length; i++) langtag[i] = data[begin + i];

          /*read the transkey*/
          begin += length + 1;
          length = 0;
          for(i = begin; i < chunkLength && data[i] != 0; i++) length++;
          if(begin + length + 1 >= chunkLength) CERROR_BREAK(decoder->error, 75); /*no null termination, corrupt?*/

          transkey = (char*)malloc(length + 1);
          if(!transkey) CERROR_BREAK(decoder->error, 9943); /*alloc fail*/

          transkey[length] = 0;
          for(i = 0; i < length; i++) transkey[i] = data[begin + i];

          /*read the actual text*/
          begin += length + 1;
          if(begin > chunkLength) CERROR_BREAK(decoder->error, 75); /*no null termination, corrupt?*/

          length = chunkLength - begin;

          if(compressed)
          {
            decoder->error = LodePNG_zlib_decompress(&decoded.data, &decoded.size,
                                                     (unsigned char*)(&data[begin]),
                                                      length, &decoder->settings.zlibsettings);
            if(decoder->error) break;
            ucvector_push_back(&decoded, 0);
          }
          else
          {
            if(!ucvector_resize(&decoded, length + 1)) CERROR_BREAK(decoder->error, 9944 /*alloc fail*/);

            decoded.data[length] = 0;
            for(i = 0; i < length; i++) decoded.data[i] = data[begin + i];
          }

          decoder->error = LodePNG_IText_add(&decoder->infoPng.itext, key, langtag, transkey, (char*)decoded.data);

          break;
        }

        free(key);
        free(langtag);
        free(transkey);
        ucvector_cleanup(&decoded);
        if(decoder->error) break;
      }
    }
    else if(LodePNG_chunk_type_equals(chunk, "tIME"))
    {
      if(chunkLength != 7) CERROR_BREAK(decoder->error, 73); /*invalid tIME chunk size*/

      decoder->infoPng.time_defined = 1;
      decoder->infoPng.time.year = 256 * data[0] + data[+ 1];
      decoder->infoPng.time.month = data[2];
      decoder->infoPng.time.day = data[3];
      decoder->infoPng.time.hour = data[4];
      decoder->infoPng.time.minute = data[5];
      decoder->infoPng.time.second = data[6];
    }
    else if(LodePNG_chunk_type_equals(chunk, "pHYs"))
    {
      if(chunkLength != 9) CERROR_BREAK(decoder->error, 74); /*invalid pHYs chunk size*/

      decoder->infoPng.phys_defined = 1;
      decoder->infoPng.phys_x = 16777216 * data[0] + 65536 * data[1] + 256 * data[2] + data[3];
      decoder->infoPng.phys_y = 16777216 * data[4] + 65536 * data[5] + 256 * data[6] + data[7];
      decoder->infoPng.phys_unit = data[8];
    }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    else /*it's not an implemented chunk type, so ignore it: skip over the data*/
    {
      /*error: unknown critical chunk (5th bit of first byte of chunk type is 0)*/
      if(LodePNG_chunk_critical(chunk)) CERROR_BREAK(decoder->error, 69);

      unknown = 1;
#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
      if(decoder->settings.rememberUnknownChunks)
      {
        LodePNG_UnknownChunks* unknown = &decoder->infoPng.unknown_chunks;
        decoder->error = LodePNG_append_chunk(&unknown->data[critical_pos - 1],
                                              &unknown->datasize[critical_pos - 1], chunk);
        if(decoder->error) break;
      }
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
    }

    if(!decoder->settings.ignoreCrc && !unknown) /*check CRC if wanted, only on known chunk types*/
    {
      if(LodePNG_chunk_check_crc(chunk)) CERROR_BREAK(decoder->error, 57); /*invalid CRC*/
    }

    if(!IEND) chunk = LodePNG_chunk_next_const(chunk);
  }

  if(!decoder->error)
  {
    ucvector scanlines;
    ucvector_init(&scanlines);

    /*maximum final image length is already reserved in the vector's length - this is not really necessary*/
    if(!ucvector_resize(&scanlines, ((decoder->infoPng.width * (decoder->infoPng.height
       * LodePNG_InfoColor_getBpp(&decoder->infoPng.color) + 7)) / 8) + decoder->infoPng.height))
    {
      decoder->error = 9945; /*alloc fail*/
    }
    if(!decoder->error)
    {
      /*decompress with the Zlib decompressor*/
      decoder->error = LodePNG_zlib_decompress(&scanlines.data, &scanlines.size, idat.data,
                                               idat.size, &decoder->settings.zlibsettings);
    }

    if(!decoder->error)
    {
      ucvector outv;
      ucvector_init(&outv);
      if(!ucvector_resizev(&outv, (decoder->infoPng.height * decoder->infoPng.width
         * LodePNG_InfoColor_getBpp(&decoder->infoPng.color) + 7) / 8, 0)) decoder->error = 9946; /*alloc fail*/
      if(!decoder->error) decoder->error = postProcessScanlines(outv.data, scanlines.data, &decoder->infoPng);
      *out = outv.data;
      *outsize = outv.size;
    }
    ucvector_cleanup(&scanlines);
  }

  ucvector_cleanup(&idat);
}

void LodePNG_Decoder_decode(LodePNG_Decoder* decoder, unsigned char** out, size_t* outsize,
                            const unsigned char* in, size_t insize)
{
  *out = 0;
  *outsize = 0;
  decodeGeneric(decoder, out, outsize, in, insize);
  if(decoder->error) return;
  if(!decoder->settings.color_convert || LodePNG_InfoColor_equal(&decoder->infoRaw.color, &decoder->infoPng.color))
  {
    /*same color type, no copying or converting of data needed*/
    /*store the infoPng color settings on the infoRaw so that the infoRaw still reflects what colorType
    the raw image has to the end user*/
    if(!decoder->settings.color_convert)
    {
      decoder->error = LodePNG_InfoColor_copy(&decoder->infoRaw.color, &decoder->infoPng.color);
      if(decoder->error) return;
    }
  }
  else
  {
    /*color conversion needed; sort of copy of the data*/
    unsigned char* data = *out;

    /*TODO: check if this works according to the statement in the documentation: "The converter can convert
    from greyscale input color type, to 8-bit greyscale or greyscale with alpha"*/
    if(!(decoder->infoRaw.color.colorType == 2 || decoder->infoRaw.color.colorType == 6)
       && !(decoder->infoRaw.color.bitDepth == 8))
    {
      decoder->error = 56; /*unsupported color mode conversion*/
      return;
    }

    *outsize = (decoder->infoPng.width * decoder->infoPng.height
                * LodePNG_InfoColor_getBpp(&decoder->infoRaw.color) + 7) / 8;
    *out = (unsigned char*)malloc(*outsize);
    if(!(*out))
    {
      decoder->error = 9947; /*alloc fail*/
      *outsize = 0;
    }
    else decoder->error = LodePNG_convert(*out, data, &decoder->infoRaw.color, &decoder->infoPng.color,
                                          decoder->infoPng.width, decoder->infoPng.height);
    free(data);
  }
}

unsigned LodePNG_decode(unsigned char** out, unsigned* w, unsigned* h, const unsigned char* in,
                        size_t insize, unsigned colorType, unsigned bitDepth)
{
  unsigned error;
  size_t dummy_size;
  LodePNG_Decoder decoder;
  LodePNG_Decoder_init(&decoder);
  decoder.infoRaw.color.colorType = colorType;
  decoder.infoRaw.color.bitDepth = bitDepth;
  LodePNG_Decoder_decode(&decoder, out, &dummy_size, in, insize);
  error = decoder.error;
  *w = decoder.infoPng.width;
  *h = decoder.infoPng.height;
  LodePNG_Decoder_cleanup(&decoder);
  return error;
}

unsigned LodePNG_decode32(unsigned char** out, unsigned* w, unsigned* h, const unsigned char* in, size_t insize)
{
  return LodePNG_decode(out, w, h, in, insize, 6, 8);
}

unsigned LodePNG_decode24(unsigned char** out, unsigned* w, unsigned* h, const unsigned char* in, size_t insize)
{
  return LodePNG_decode(out, w, h, in, insize, 2, 8);
}

#ifdef LODEPNG_COMPILE_DISK
unsigned LodePNG_decode_file(unsigned char** out, unsigned* w, unsigned* h, const char* filename,
                             unsigned colorType, unsigned bitDepth)
{
  unsigned char* buffer;
  size_t buffersize;
  unsigned error;
  error = LodePNG_loadFile(&buffer, &buffersize, filename);
  if(!error) error = LodePNG_decode(out, w, h, buffer, buffersize, colorType, bitDepth);
  free(buffer);
  return error;
}

unsigned LodePNG_decode32_file(unsigned char** out, unsigned* w, unsigned* h, const char* filename)
{
  return LodePNG_decode_file(out, w, h, filename, 6, 8);
}

unsigned LodePNG_decode24_file(unsigned char** out, unsigned* w, unsigned* h, const char* filename)
{
  return LodePNG_decode_file(out, w, h, filename, 2, 8);
}
#endif /*LODEPNG_COMPILE_DISK*/

void LodePNG_DecodeSettings_init(LodePNG_DecodeSettings* settings)
{
  settings->color_convert = 1;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  settings->readTextChunks = 1;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
  settings->ignoreCrc = 0;
#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
  settings->rememberUnknownChunks = 0;
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
  LodePNG_DecompressSettings_init(&settings->zlibsettings);
}

void LodePNG_Decoder_init(LodePNG_Decoder* decoder)
{
  LodePNG_DecodeSettings_init(&decoder->settings);
  LodePNG_InfoRaw_init(&decoder->infoRaw);
  LodePNG_InfoPng_init(&decoder->infoPng);
  decoder->error = 1;
}

void LodePNG_Decoder_cleanup(LodePNG_Decoder* decoder)
{
  LodePNG_InfoRaw_cleanup(&decoder->infoRaw);
  LodePNG_InfoPng_cleanup(&decoder->infoPng);
}

void LodePNG_Decoder_copy(LodePNG_Decoder* dest, const LodePNG_Decoder* source)
{
  LodePNG_Decoder_cleanup(dest);
  *dest = *source;
  LodePNG_InfoRaw_init(&dest->infoRaw);
  LodePNG_InfoPng_init(&dest->infoPng);
  dest->error = LodePNG_InfoRaw_copy(&dest->infoRaw, &source->infoRaw); if(dest->error) return;
  dest->error = LodePNG_InfoPng_copy(&dest->infoPng, &source->infoPng); if(dest->error) return;
}

#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG Encoder                                                            / */
/* ////////////////////////////////////////////////////////////////////////// */

/*chunkName must be string of 4 characters*/
static unsigned addChunk(ucvector* out, const char* chunkName, const unsigned char* data, size_t length)
{
  unsigned error = LodePNG_create_chunk(&out->data, &out->size, (unsigned)length, chunkName, data);
  if(error) return error;
  out->allocsize = out->size; /*fix the allocsize again*/
  return 0;
}

static void writeSignature(ucvector* out)
{
  /*8 bytes PNG signature, aka the magic bytes*/
  ucvector_push_back(out, 137);
  ucvector_push_back(out, 80);
  ucvector_push_back(out, 78);
  ucvector_push_back(out, 71);
  ucvector_push_back(out, 13);
  ucvector_push_back(out, 10);
  ucvector_push_back(out, 26);
  ucvector_push_back(out, 10);
}

static unsigned addChunk_IHDR(ucvector* out, unsigned w, unsigned h, unsigned bitDepth,
                              unsigned colorType, unsigned interlaceMethod)
{
  unsigned error = 0;
  ucvector header;
  ucvector_init(&header);

  LodePNG_add32bitInt(&header, w); /*width*/
  LodePNG_add32bitInt(&header, h); /*height*/
  ucvector_push_back(&header, (unsigned char)bitDepth); /*bit depth*/
  ucvector_push_back(&header, (unsigned char)colorType); /*color type*/
  ucvector_push_back(&header, 0); /*compression method*/
  ucvector_push_back(&header, 0); /*filter method*/
  ucvector_push_back(&header, interlaceMethod); /*interlace method*/

  error = addChunk(out, "IHDR", header.data, header.size);
  ucvector_cleanup(&header);

  return error;
}

static unsigned addChunk_PLTE(ucvector* out, const LodePNG_InfoColor* info)
{
  unsigned error = 0;
  size_t i;
  ucvector PLTE;
  ucvector_init(&PLTE);
  for(i = 0; i < info->palettesize * 4; i++)
  {
    /*add all channels except alpha channel*/
    if(i % 4 != 3) ucvector_push_back(&PLTE, info->palette[i]);
  }
  error = addChunk(out, "PLTE", PLTE.data, PLTE.size);
  ucvector_cleanup(&PLTE);

  return error;
}

static unsigned addChunk_tRNS(ucvector* out, const LodePNG_InfoColor* info)
{
  unsigned error = 0;
  size_t i;
  ucvector tRNS;
  ucvector_init(&tRNS);
  if(info->colorType == 3)
  {
    /*add only alpha channel*/
    for(i = 0; i < info->palettesize; i++) ucvector_push_back(&tRNS, info->palette[4 * i + 3]);
  }
  else if(info->colorType == 0)
  {
    if(info->key_defined)
    {
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r / 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r % 256));
    }
  }
  else if(info->colorType == 2)
  {
    if(info->key_defined)
    {
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r / 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r % 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_g / 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_g % 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_b / 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_b % 256));
    }
  }

  error = addChunk(out, "tRNS", tRNS.data, tRNS.size);
  ucvector_cleanup(&tRNS);

  return error;
}

static unsigned addChunk_IDAT(ucvector* out, const unsigned char* data, size_t datasize,
                              LodePNG_CompressSettings* zlibsettings)
{
  ucvector zlibdata;
  unsigned error = 0;

  /*compress with the Zlib compressor*/
  ucvector_init(&zlibdata);
  error = LodePNG_zlib_compress(&zlibdata.data, &zlibdata.size, data, datasize, zlibsettings);
  if(!error) error = addChunk(out, "IDAT", zlibdata.data, zlibdata.size);
  ucvector_cleanup(&zlibdata);

  return error;
}

static unsigned addChunk_IEND(ucvector* out)
{
  unsigned error = 0;
  error = addChunk(out, "IEND", 0, 0);
  return error;
}

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS

static unsigned addChunk_tEXt(ucvector* out, const char* keyword, const char* textstring)
{
  unsigned error = 0;
  size_t i;
  ucvector text;
  ucvector_init(&text);
  for(i = 0; keyword[i] != 0; i++) ucvector_push_back(&text, (unsigned char)keyword[i]);
  ucvector_push_back(&text, 0);
  for(i = 0; textstring[i] != 0; i++) ucvector_push_back(&text, (unsigned char)textstring[i]);
  error = addChunk(out, "tEXt", text.data, text.size);
  ucvector_cleanup(&text);

  return error;
}

static unsigned addChunk_zTXt(ucvector* out, const char* keyword, const char* textstring,
                              LodePNG_CompressSettings* zlibsettings)
{
  unsigned error = 0;
  ucvector data, compressed;
  size_t i, textsize = strlen(textstring);

  ucvector_init(&data);
  ucvector_init(&compressed);
  for(i = 0; keyword[i] != 0; i++) ucvector_push_back(&data, (unsigned char)keyword[i]);
  ucvector_push_back(&data, 0); /* 0 termination char*/
  ucvector_push_back(&data, 0); /*compression method: 0*/

  error = LodePNG_zlib_compress(&compressed.data, &compressed.size,
                                (unsigned char*)textstring, textsize, zlibsettings);
  if(!error)
  {
    for(i = 0; i < compressed.size; i++) ucvector_push_back(&data, compressed.data[i]);
    error = addChunk(out, "zTXt", data.data, data.size);
  }

  ucvector_cleanup(&compressed);
  ucvector_cleanup(&data);
  return error;
}

static unsigned addChunk_iTXt(ucvector* out, unsigned compressed, const char* keyword, const char* langtag,
                              const char* transkey, const char* textstring, LodePNG_CompressSettings* zlibsettings)
{
  unsigned error = 0;
  ucvector data, compressed_data;
  size_t i, textsize = strlen(textstring);

  ucvector_init(&data);

  for(i = 0; keyword[i] != 0; i++) ucvector_push_back(&data, (unsigned char)keyword[i]);
  ucvector_push_back(&data, 0); /*null termination char*/
  ucvector_push_back(&data, compressed ? 1 : 0); /*compression flag*/
  ucvector_push_back(&data, 0); /*compression method*/
  for(i = 0; langtag[i] != 0; i++) ucvector_push_back(&data, (unsigned char)langtag[i]);
  ucvector_push_back(&data, 0); /*null termination char*/
  for(i = 0; transkey[i] != 0; i++) ucvector_push_back(&data, (unsigned char)transkey[i]);
  ucvector_push_back(&data, 0); /*null termination char*/

  if(compressed)
  {
    ucvector_init(&compressed_data);
    error = LodePNG_zlib_compress(&compressed_data.data, &compressed_data.size,
                                  (unsigned char*)textstring, textsize, zlibsettings);
    if(!error)
    {
      for(i = 0; i < compressed_data.size; i++) ucvector_push_back(&data, compressed_data.data[i]);
      for(i = 0; textstring[i] != 0; i++) ucvector_push_back(&data, (unsigned char)textstring[i]);
    }
  }
  else /*not compressed*/
  {
    for(i = 0; textstring[i] != 0; i++) ucvector_push_back(&data, (unsigned char)textstring[i]);
  }

  if(!error) error = addChunk(out, "iTXt", data.data, data.size);
  ucvector_cleanup(&data);
  return error;
}

static unsigned addChunk_bKGD(ucvector* out, const LodePNG_InfoPng* info)
{
  unsigned error = 0;
  ucvector bKGD;
  ucvector_init(&bKGD);
  if(info->color.colorType == 0 || info->color.colorType == 4)
  {
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r / 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r % 256));
  }
  else if(info->color.colorType == 2 || info->color.colorType == 6)
  {
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r / 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r % 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_g / 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_g % 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_b / 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_b % 256));
  }
  else if(info->color.colorType == 3)
  {
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r % 256)); /*palette index*/
  }

  error = addChunk(out, "bKGD", bKGD.data, bKGD.size);
  ucvector_cleanup(&bKGD);

  return error;
}

static unsigned addChunk_tIME(ucvector* out, const LodePNG_Time* time)
{
  unsigned error = 0;
  unsigned char* data = (unsigned char*)malloc(7);
  if(!data) return 9948; /*alloc fail*/
  data[0] = (unsigned char)(time->year / 256);
  data[1] = (unsigned char)(time->year % 256);
  data[2] = time->month;
  data[3] = time->day;
  data[4] = time->hour;
  data[5] = time->minute;
  data[6] = time->second;
  error = addChunk(out, "tIME", data, 7);
  free(data);
  return error;
}

static unsigned addChunk_pHYs(ucvector* out, const LodePNG_InfoPng* info)
{
  unsigned error = 0;
  ucvector data;
  ucvector_init(&data);

  LodePNG_add32bitInt(&data, info->phys_x);
  LodePNG_add32bitInt(&data, info->phys_y);
  ucvector_push_back(&data, info->phys_unit);

  error = addChunk(out, "pHYs", data.data, data.size);
  ucvector_cleanup(&data);

  return error;
}

#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

static void filterScanline(unsigned char* out, const unsigned char* scanline, const unsigned char* prevline,
                           size_t length, size_t bytewidth, unsigned char filterType)
{
  size_t i;
  switch(filterType)
  {
    case 0: /*None*/
      for(i = 0; i < length; i++) out[i] = scanline[i];
      break;
    case 1: /*Sub*/
      if(prevline)
      {
        for(i = 0; i < bytewidth; i++) out[i] = scanline[i];
        for(i = bytewidth; i < length   ; i++) out[i] = scanline[i] - scanline[i - bytewidth];
      }
      else
      {
        for(i = 0; i < bytewidth; i++) out[i] = scanline[i];
        for(i = bytewidth; i <    length; i++) out[i] = scanline[i] - scanline[i - bytewidth];
      }
      break;
    case 2: /*Up*/
      if(prevline)
      {
        for(i = 0; i < length; i++) out[i] = scanline[i] - prevline[i];
      }
      else
      {
        for(i = 0; i < length; i++) out[i] = scanline[i];
      }
      break;
    case 3: /*Average*/
      if(prevline)
      {
        for(i = 0; i < bytewidth; i++) out[i] = scanline[i] - prevline[i] / 2;
        for(i = bytewidth; i < length; i++) out[i] = scanline[i] - ((scanline[i - bytewidth] + prevline[i]) / 2);
      }
      else
      {
        for(i = 0; i < bytewidth; i++) out[i] = scanline[i];
        for(i = bytewidth; i < length; i++) out[i] = scanline[i] - scanline[i - bytewidth] / 2;
      }
      break;
    case 4: /*Paeth*/
      if(prevline)
      {
        /*paethPredictor(0, prevline[i], 0) is always prevline[i]*/
        for(i = 0; i < bytewidth; i++) out[i] = (scanline[i] - prevline[i]);
        for(i = bytewidth; i < length; i++)
        {
          out[i] = (scanline[i] - paethPredictor(scanline[i - bytewidth], prevline[i], prevline[i - bytewidth]));
        }
      }
      else
      {
        for(i = 0; i < bytewidth; i++) out[i] = scanline[i];
        /*paethPredictor(scanline[i - bytewidth], 0, 0) is always scanline[i - bytewidth]*/
        for(i = bytewidth; i < length; i++) out[i] = (scanline[i] - scanline[i - bytewidth]);
      }
      break;
    default: return; /*unexisting filter type given*/
  }
}

static unsigned filter(unsigned char* out, const unsigned char* in, unsigned w, unsigned h,
                       const LodePNG_InfoColor* info, const LodePNG_EncodeSettings* settings)
{
  /*
  For PNG filter method 0
  out must be a buffer with as size: h + (w * h * bpp + 7) / 8, because there are
  the scanlines with 1 extra byte per scanline
  */

  unsigned bpp = LodePNG_InfoColor_getBpp(info);
  /*the width of a scanline in bytes, not including the filter type*/
  size_t linebytes = (w * bpp + 7) / 8;
  /*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise*/
  size_t bytewidth = (bpp + 7) / 8;
  const unsigned char* prevline = 0;
  unsigned x, y;
  unsigned error = 0;

  if(bpp == 0) return 31; /*error: invalid color type*/

  if(!settings->bruteForceFilters)
  {
    /*
    There is a heuristic called the minimum sum of absolute differences heuristic, suggested by the PNG standard:
       *  If the image type is Palette, or the bit depth is smaller than 8, then do not filter the image (i.e.
          use fixed filtering, with the filter None).
       * (The other case) If the image type is Grayscale or RGB (with or without Alpha), and the bit depth is
         not smaller than 8, then use adaptive filtering heuristic as follows: independently for each row, apply
         all five filters and select the filter that produces the smallest sum of absolute values per row.
    */
    if(info->colorType == 3 || info->bitDepth < 8) /*None filtertype for everything*/
    {
      for(y = 0; y < h; y++)
      {
        size_t outindex = (1 + linebytes) * y; /*the extra filterbyte added to each row*/
        size_t inindex = linebytes * y;
        const unsigned TYPE = 0;
        out[outindex] = TYPE; /*filter type byte*/
        filterScanline(&out[outindex + 1], &in[inindex], prevline, linebytes, bytewidth, TYPE);
        prevline = &in[inindex];
      }
    }
    else /*adaptive filtering*/
    {
      size_t sum[5];
      ucvector attempt[5]; /*five filtering attempts, one for each filter type*/
      size_t smallest = 0;
      unsigned type, bestType = 0;

      for(type = 0; type < 5; type++) ucvector_init(&attempt[type]);
      for(type = 0; type < 5; type++)
      {
        if(!ucvector_resize(&attempt[type], linebytes)) ERROR_BREAK(9949 /*alloc fail*/);
      }

      if(!error)
      {
        for(y = 0; y < h; y++)
        {
          /*try the 5 filter types*/
          for(type = 0; type < 5; type++)
          {
            filterScanline(attempt[type].data, &in[y * linebytes], prevline, linebytes, bytewidth, type);

            /*calculate the sum of the result*/
            sum[type] = 0;
            /*note that not all pixels are checked to speed this up while still having probably the best choice*/
            for(x = 0; x < attempt[type].size; x+=3)
            {
              /*For differences, each byte should be treated as signed, values above 127 are negative
              (converted to signed char). Filtertype 0 isn't a difference though, so use unsigned there.
              This means filtertype 0 is almost never chosen, but that is justified.*/
              if(type == 0) sum[type] += (unsigned char)(attempt[type].data[x]);
              else
              {
                signed char s = (signed char)(attempt[type].data[x]);
                sum[type] += s < 0 ? -s : s;
              }
            }

            /*check if this is smallest sum (or if type == 0 it's the first case so always store the values)*/
            if(type == 0 || sum[type] < smallest)
            {
              bestType = type;
              smallest = sum[type];
            }
          }

          prevline = &in[y * linebytes];

          /*now fill the out values*/
          out[y * (linebytes + 1)] = bestType; /*the first byte of a scanline will be the filter type*/
          for(x = 0; x < linebytes; x++) out[y * (linebytes + 1) + 1 + x] = attempt[bestType].data[x];
        }
      }

      for(type = 0; type < 5; type++) ucvector_cleanup(&attempt[type]);
    }
  }
  else
  {
    /*brute force filter chooser.
    deflate the scanline after every filter attempt to see which one deflates best.
    This is very slow and gives only slightly smaller, sometimes even larger, result*/
    size_t size[5];
    ucvector attempt[5]; /*five filtering attempts, one for each filter type*/
    size_t smallest;
    unsigned type = 0, bestType = 0;
    unsigned char* dummy;
    LodePNG_CompressSettings zlibsettings = settings->zlibsettings;
    /*use fixed tree on the attempts so that the tree is not adapted to the filtertype on purpose,
    to simulate the true case where the tree is the same for the whole image. Sometimes it gives
    better result with dynamic tree anyway. Using the fixed tree sometimes gives worse, but in rare
    cases better compression. It does make this a bit less slow, so it's worth doing this.*/
    zlibsettings.btype = 1;
    for(type = 0; type < 5; type++)
    {
      ucvector_init(&attempt[type]);
      ucvector_resize(&attempt[type], linebytes); /*todo: give error if resize failed*/
    }
    for(y = 0; y < h; y++) /*try the 5 filter types*/
    {
      for(type = 0; type < 5; type++)
      {
        unsigned testsize = attempt[type].size;
        /*unsigned testsize = attempt[type].size / 8;*/ /*it already works good enough by testing a part of the row*/
        /*if(testsize == 0) testsize = attempt[type].size;*/

        filterScanline(attempt[type].data, &in[y * linebytes], prevline, linebytes, bytewidth, type);
        size[type] = 0;
        dummy = 0;
        LodePNG_zlib_compress(&dummy, &size[type], attempt[type].data, testsize, &zlibsettings);
        free(dummy);
        /*check if this is smallest size (or if type == 0 it's the first case so always store the values)*/
        if(type == 0 || size[type] < smallest)
        {
          bestType = type;
          smallest = size[type];
        }
      }
      prevline = &in[y * linebytes];
      out[y * (linebytes + 1)] = bestType; /*the first byte of a scanline will be the filter type*/
      for(x = 0; x < linebytes; x++) out[y * (linebytes + 1) + 1 + x] = attempt[bestType].data[x];
    }
    for(type = 0; type < 5; type++) ucvector_cleanup(&attempt[type]);
  }

  return error;
}

static void addPaddingBits(unsigned char* out, const unsigned char* in,
                           size_t olinebits, size_t ilinebits, unsigned h)
{
  /*The opposite of the removePaddingBits function
  olinebits must be >= ilinebits*/
  unsigned y;
  size_t diff = olinebits - ilinebits;
  size_t obp = 0, ibp = 0; /*bit pointers*/
  for(y = 0; y < h; y++)
  {
    size_t x;
    for(x = 0; x < ilinebits; x++)
    {
      unsigned char bit = readBitFromReversedStream(&ibp, in);
      setBitOfReversedStream(&obp, out, bit);
    }
    /*obp += diff; --> no, fill in some value in the padding bits too, to avoid
    "Use of uninitialised value of size ###" warning from valgrind*/
    for(x = 0; x < diff; x++) setBitOfReversedStream(&obp, out, 0);
  }
}

static void Adam7_interlace(unsigned char* out, const unsigned char* in, unsigned w, unsigned h, unsigned bpp)
{
  /*Note: this function works on image buffers WITHOUT padding bits at end of scanlines with non-multiple-of-8
  bit amounts, only between reduced images is padding*/
  unsigned passw[7], passh[7];
  size_t filter_passstart[8], padded_passstart[8], passstart[8];
  unsigned i;

  Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

  if(bpp >= 8)
  {
    for(i = 0; i < 7; i++)
    {
      unsigned x, y, b;
      size_t bytewidth = bpp / 8;
      for(y = 0; y < passh[i]; y++)
      for(x = 0; x < passw[i]; x++)
      {
        size_t pixelinstart = ((ADAM7_IY[i] + y * ADAM7_DY[i]) * w + ADAM7_IX[i] + x * ADAM7_DX[i]) * bytewidth;
        size_t pixeloutstart = passstart[i] + (y * passw[i] + x) * bytewidth;
        for(b = 0; b < bytewidth; b++)
        {
          out[pixeloutstart + b] = in[pixelinstart + b];
        }
      }
    }
  }
  else /*bpp < 8: Adam7 with pixels < 8 bit is a bit trickier: with bit pointers*/
  {
    for(i = 0; i < 7; i++)
    {
      unsigned x, y, b;
      unsigned ilinebits = bpp * passw[i];
      unsigned olinebits = bpp * w;
      size_t obp, ibp; /*bit pointers (for out and in buffer)*/
      for(y = 0; y < passh[i]; y++)
      for(x = 0; x < passw[i]; x++)
      {
        ibp = (ADAM7_IY[i] + y * ADAM7_DY[i]) * olinebits + (ADAM7_IX[i] + x * ADAM7_DX[i]) * bpp;
        obp = (8 * passstart[i]) + (y * ilinebits + x * bpp);
        for(b = 0; b < bpp; b++)
        {
          unsigned char bit = readBitFromReversedStream(&ibp, in);
          setBitOfReversedStream(&obp, out, bit);
        }
      }
    }
  }
}

/*out must be buffer big enough to contain uncompressed IDAT chunk data, and in must contain the full image.
return value is error**/
static unsigned preProcessScanlines(unsigned char** out, size_t* outsize, const unsigned char* in,
                                    const LodePNG_InfoPng* infoPng, const LodePNG_EncodeSettings* settings)
{
  /*
  This function converts the pure 2D image with the PNG's colortype, into filtered-padded-interlaced data. Steps:
  *) if no Adam7: 1) add padding bits (= posible extra bits per scanline if bpp < 8) 2) filter
  *) if adam7: 1) Adam7_interlace 2) 7x add padding bits 3) 7x filter
  */
  unsigned bpp = LodePNG_InfoColor_getBpp(&infoPng->color);
  unsigned w = infoPng->width;
  unsigned h = infoPng->height;
  unsigned error = 0;

  if(infoPng->interlaceMethod == 0)
  {
    *outsize = h + (h * ((w * bpp + 7) / 8)); /*image size plus an extra byte per scanline + possible padding bits*/
    *out = (unsigned char*)malloc(*outsize);
    if(!(*out) && (*outsize)) error = 9950; /*alloc fail*/

    if(!error)
    {
      /*non multiple of 8 bits per scanline, padding bits needed per scanline*/
      if(bpp < 8 && w * bpp != ((w * bpp + 7) / 8) * 8)
      {
        ucvector padded;
        ucvector_init(&padded);
        if(!ucvector_resize(&padded, h * ((w * bpp + 7) / 8))) error = 9951; /*alloc fail*/
        if(!error)
        {
          addPaddingBits(padded.data, in, ((w * bpp + 7) / 8) * 8, w * bpp, h);
          error = filter(*out, padded.data, w, h, &infoPng->color, settings);
        }
        ucvector_cleanup(&padded);
      }
      else
      {
        /*we can immediatly filter into the out buffer, no other steps needed*/
        error = filter(*out, in, w, h, &infoPng->color, settings);
      }
    }
  }
  else /*interlaceMethod is 1 (Adam7)*/
  {
    unsigned char* adam7 = (unsigned char*)malloc((h * w * bpp + 7) / 8);
    if(!adam7 && ((h * w * bpp + 7) / 8)) error = 9952; /*alloc fail*/

    while(!error) /*not a real while loop, used to break out to cleanup to avoid a goto*/
    {
      unsigned passw[7], passh[7];
      size_t filter_passstart[8], padded_passstart[8], passstart[8];
      unsigned i;

      Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

      *outsize = filter_passstart[7]; /*image size plus an extra byte per scanline + possible padding bits*/
      *out = (unsigned char*)malloc(*outsize);
      if(!(*out) && (*outsize)) ERROR_BREAK(9953 /*alloc fail*/);

      Adam7_interlace(adam7, in, w, h, bpp);

      for(i = 0; i < 7; i++)
      {
        if(bpp < 8)
        {
          ucvector padded;
          ucvector_init(&padded);
          if(!ucvector_resize(&padded, h * ((w * bpp + 7) / 8))) error = 9954; /*alloc fail*/
          if(!error)
          {
            addPaddingBits(&padded.data[padded_passstart[i]], &adam7[passstart[i]],
                           ((passw[i] * bpp + 7) / 8) * 8, passw[i] * bpp, passh[i]);
            error = filter(&(*out)[filter_passstart[i]], &padded.data[padded_passstart[i]],
                           passw[i], passh[i], &infoPng->color, settings);
          }

          ucvector_cleanup(&padded);
        }
        else
        {
          error = filter(&(*out)[filter_passstart[i]], &adam7[padded_passstart[i]],
                         passw[i], passh[i], &infoPng->color, settings);
        }
      }

      break;
    }

    free(adam7);
  }

  return error;
}

/*palette must have 4 * palettesize bytes allocated, and given in format RGBARGBARGBARGBA...*/
static unsigned isPaletteFullyOpaque(const unsigned char* palette, size_t palettesize)
{
  size_t i;
  for(i = 0; i < palettesize; i++)
  {
    if(palette[4 * i + 3] != 255) return 0;
  }
  return 1;
}

/*this function checks if the input image given by the user has no transparent pixels*/
static unsigned isFullyOpaque(const unsigned char* image, unsigned w, unsigned h, const LodePNG_InfoColor* info)
{
  /*TODO: When the user specified a color key for the input image, then this function must
  also check for pixels that are the same as the color key and treat those as transparent.*/

  unsigned i, numpixels = w * h;
  if(info->colorType == 6)
  {
    if(info->bitDepth == 8)
    {
      for(i = 0; i < numpixels; i++)
      {
        if(image[i * 4 + 3] != 255) return 0;
      }
    }
    else
    {
      for(i = 0; i < numpixels; i++)
      {
        if(image[i * 8 + 6] != 255 || image[i * 8 + 7] != 255) return 0;
      }
    }
    return 1; /*no single pixel with alpha channel other than 255 found*/
  }
  else if(info->colorType == 4)
  {
    if(info->bitDepth == 8)
    {
      for(i = 0; i < numpixels; i++)
      {
        if(image[i * 2 + 1] != 255) return 0;
      }
    }
    else
    {
      for(i = 0; i < numpixels; i++)
      {
        if(image[i * 4 + 2] != 255 || image[i * 4 + 3] != 255) return 0;
      }
    }
    return 1; /*no single pixel with alpha channel other than 255 found*/
  }
  else if(info->colorType == 3)
  {
    /*when there's a palette, we could check every pixel for translucency,
    but much quicker is to just check the palette*/
    return(isPaletteFullyOpaque(info->palette, info->palettesize));
  }

  return 0; /*color type that isn't supported by this function yet, so assume there is transparency to be safe*/
}

#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
static unsigned addUnknownChunks(ucvector* out, unsigned char* data, size_t datasize)
{
  unsigned char* inchunk = data;
  while((size_t)(inchunk - data) < datasize)
  {
    unsigned error = LodePNG_append_chunk(&out->data, &out->size, inchunk);
    if(error) return error; /*error: not enough memory*/
    out->allocsize = out->size; /*fix the allocsize again*/
    inchunk = LodePNG_chunk_next(inchunk);
  }
  return 0;
}
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/

void LodePNG_Encoder_encode(LodePNG_Encoder* encoder, unsigned char** out, size_t* outsize,
                            const unsigned char* image, unsigned w, unsigned h)
{
  LodePNG_InfoPng info;
  ucvector outv;
  unsigned char* data = 0; /*uncompressed version of the IDAT chunk data*/
  size_t datasize = 0;

  /*provide some proper output values if error will happen*/
  *out = 0;
  *outsize = 0;
  encoder->error = 0;

  /*UNSAFE copy to avoid having to cleanup! but we will only change primitive parameters,
  and not invoke the cleanup function nor touch the palette's buffer so we use it safely*/
  info = encoder->infoPng;
  info.width = w;
  info.height = h;

  if(encoder->settings.autoLeaveOutAlphaChannel && isFullyOpaque(image, w, h, &encoder->infoRaw.color))
  {
    /*go to a color type without alpha channel*/
    if(info.color.colorType == 6) info.color.colorType = 2;
    else if(info.color.colorType == 4) info.color.colorType = 0;
  }

  if(encoder->settings.zlibsettings.windowSize > 32768)
  {
    encoder->error = 60; /*error: windowsize larger than allowed*/
    return;
  }
  if(encoder->settings.zlibsettings.btype > 2)
  {
    encoder->error = 61; /*error: unexisting btype*/
    return;
  }
  if(encoder->infoPng.interlaceMethod > 1)
  {
    encoder->error = 71; /*error: unexisting interlace mode*/
    return;
  }
  /*error: unexisting color type given*/
  if((encoder->error = checkColorValidity(info.color.colorType, info.color.bitDepth))) return;
  /*error: unexisting color type given*/
  if((encoder->error = checkColorValidity(encoder->infoRaw.color.colorType, encoder->infoRaw.color.bitDepth))) return;

  if(!LodePNG_InfoColor_equal(&encoder->infoRaw.color, &info.color))
  {
    unsigned char* converted;
    size_t size = (w * h * LodePNG_InfoColor_getBpp(&info.color) + 7) / 8;

    if((info.color.colorType != 6 && info.color.colorType != 2) || (info.color.bitDepth != 8))
    {
      encoder->error = 59; /*for the output image, only these types are supported*/
      return;
    }
    converted = (unsigned char*)malloc(size);
    if(!converted && size) encoder->error = 9955; /*alloc fail*/
    if(!encoder->error)
    {
      encoder->error = LodePNG_convert(converted, image, &info.color, &encoder->infoRaw.color, w, h);
    }
    if(!encoder->error) preProcessScanlines(&data, &datasize, converted, &info, &encoder->settings);
    free(converted);
  }
  else preProcessScanlines(&data, &datasize, image, &info, &encoder->settings);

  ucvector_init(&outv);
  while(!encoder->error) /*while only executed once, to break on error*/
  {
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    size_t i;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    /*write signature and chunks*/
    writeSignature(&outv);
    /*IHDR*/
    addChunk_IHDR(&outv, w, h, info.color.bitDepth, info.color.colorType, info.interlaceMethod);
#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
    /*unknown chunks between IHDR and PLTE*/
    if(info.unknown_chunks.data[0])
    {
      encoder->error = addUnknownChunks(&outv, info.unknown_chunks.data[0], info.unknown_chunks.datasize[0]);
      if(encoder->error) break;
    }
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
    /*PLTE*/
    if(info.color.colorType == 3)
    {
      if(info.color.palettesize == 0 || info.color.palettesize > 256)
      {
        encoder->error = 68; /*invalid palette size*/
        break;
      }
      addChunk_PLTE(&outv, &info.color);
    }
    if(encoder->settings.force_palette && (info.color.colorType == 2 || info.color.colorType == 6))
    {
      if(info.color.palettesize == 0 || info.color.palettesize > 256)
      {
        encoder->error = 68; /*invalid palette size*/
        break;
      }
      addChunk_PLTE(&outv, &info.color);
    }
    /*tRNS*/
    if(info.color.colorType == 3 && !isPaletteFullyOpaque(info.color.palette, info.color.palettesize))
    {
      addChunk_tRNS(&outv, &info.color);
    }
    if((info.color.colorType == 0 || info.color.colorType == 2) && info.color.key_defined)
    {
      addChunk_tRNS(&outv, &info.color);
    }
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*bKGD (must come between PLTE and the IDAt chunks*/
    if(info.background_defined) addChunk_bKGD(&outv, &info);
    /*pHYs (must come before the IDAT chunks)*/
    if(info.phys_defined) addChunk_pHYs(&outv, &info);
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
    /*unknown chunks between PLTE and IDAT*/
    if(info.unknown_chunks.data[1])
    {
      encoder->error = addUnknownChunks(&outv, info.unknown_chunks.data[1], info.unknown_chunks.datasize[1]);
      if(encoder->error) break;
    }
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
    /*IDAT (multiple IDAT chunks must be consecutive)*/
    encoder->error = addChunk_IDAT(&outv, data, datasize, &encoder->settings.zlibsettings);
    if(encoder->error) break;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*tIME*/
    if(info.time_defined) addChunk_tIME(&outv, &info.time);
    /*tEXt and/or zTXt*/
    for(i = 0; i < info.text.num; i++)
    {
      if(strlen(info.text.keys[i]) > 79)
      {
        encoder->error = 66; /*text chunk too large*/
        break;
      }
      if(strlen(info.text.keys[i]) < 1)
      {
        encoder->error = 67; /*text chunk too small*/
        break;
      }
      if(encoder->settings.text_compression)
        addChunk_zTXt(&outv, info.text.keys[i], info.text.strings[i], &encoder->settings.zlibsettings);
      else
        addChunk_tEXt(&outv, info.text.keys[i], info.text.strings[i]);
    }
    /*LodePNG version id in text chunk*/
    if(encoder->settings.add_id)
    {
      unsigned alread_added_id_text = 0;
      for(i = 0; i < info.text.num; i++)
      {
        if(!strcmp(info.text.keys[i], "LodePNG"))
        {
          alread_added_id_text = 1;
          break;
        }
      }
      if(alread_added_id_text == 0)
        addChunk_tEXt(&outv, "LodePNG", VERSION_STRING); /*it's shorter as tEXt than as zTXt chunk*/
    }
    /*iTXt*/
    for(i = 0; i < info.itext.num; i++)
    {
      if(strlen(info.itext.keys[i]) > 79)
      {
        encoder->error = 66; /*text chunk too large*/
        break;
      }
      if(strlen(info.itext.keys[i]) < 1)
      {
        encoder->error = 67; /*text chunk too small*/
        break;
      }
      addChunk_iTXt(&outv, encoder->settings.text_compression,
                    info.itext.keys[i], info.itext.langtags[i], info.itext.transkeys[i], info.itext.strings[i],
                    &encoder->settings.zlibsettings);
    }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
#ifdef LODEPNG_COMPILE_UNKNOWN_CHUNKS
    /*unknown chunks between IDAT and IEND*/
    if(info.unknown_chunks.data[2])
    {
      encoder->error = addUnknownChunks(&outv, info.unknown_chunks.data[2], info.unknown_chunks.datasize[2]);
      if(encoder->error) break;
    }
#endif /*LODEPNG_COMPILE_UNKNOWN_CHUNKS*/
    /*IEND*/
    addChunk_IEND(&outv);

    break; /*this isn't really a while loop; no error happened so break out now!*/
  }

  free(data);
  /*instead of cleaning the vector up, give it to the output*/
  *out = outv.data;
  *outsize = outv.size;
}

unsigned LodePNG_encode(unsigned char** out, size_t* outsize, const unsigned char* image,
                        unsigned w, unsigned h, unsigned colorType, unsigned bitDepth)
{
  unsigned error;
  LodePNG_Encoder encoder;
  LodePNG_Encoder_init(&encoder);
  encoder.infoRaw.color.colorType = colorType;
  encoder.infoRaw.color.bitDepth = bitDepth;
  encoder.infoPng.color.colorType = colorType;
  encoder.infoPng.color.bitDepth = bitDepth;
  LodePNG_Encoder_encode(&encoder, out, outsize, image, w, h);
  error = encoder.error;
  LodePNG_Encoder_cleanup(&encoder);
  return error;
}

unsigned LodePNG_encode32(unsigned char** out, size_t* outsize, const unsigned char* image, unsigned w, unsigned h)
{
  return LodePNG_encode(out, outsize, image, w, h, 6, 8);
}

unsigned LodePNG_encode24(unsigned char** out, size_t* outsize, const unsigned char* image, unsigned w, unsigned h)
{
  return LodePNG_encode(out, outsize, image, w, h, 2, 8);
}

#ifdef LODEPNG_COMPILE_DISK
unsigned LodePNG_encode_file(const char* filename, const unsigned char* image, unsigned w, unsigned h,
                             unsigned colorType, unsigned bitDepth)
{
  unsigned char* buffer;
  size_t buffersize;
  unsigned error = LodePNG_encode(&buffer, &buffersize, image, w, h, colorType, bitDepth);
  if(!error) error = LodePNG_saveFile(buffer, buffersize, filename);
  free(buffer);
  return error;
}

unsigned LodePNG_encode32_file(const char* filename, const unsigned char* image, unsigned w, unsigned h)
{
  return LodePNG_encode_file(filename, image, w, h, 6, 8);
}

unsigned LodePNG_encode24_file(const char* filename, const unsigned char* image, unsigned w, unsigned h)
{
  return LodePNG_encode_file(filename, image, w, h, 2, 8);
}
#endif /*LODEPNG_COMPILE_DISK*/

void LodePNG_EncodeSettings_init(LodePNG_EncodeSettings* settings)
{
  LodePNG_CompressSettings_init(&settings->zlibsettings);
  settings->bruteForceFilters = 0;
  settings->autoLeaveOutAlphaChannel = 1;
  settings->force_palette = 0;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  settings->add_id = 1;
  settings->text_compression = 0;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
}

void LodePNG_Encoder_init(LodePNG_Encoder* encoder)
{
  LodePNG_EncodeSettings_init(&encoder->settings);
  LodePNG_InfoPng_init(&encoder->infoPng);
  LodePNG_InfoRaw_init(&encoder->infoRaw);
  encoder->error = 1;
}

void LodePNG_Encoder_cleanup(LodePNG_Encoder* encoder)
{
  LodePNG_InfoPng_cleanup(&encoder->infoPng);
  LodePNG_InfoRaw_cleanup(&encoder->infoRaw);
}

void LodePNG_Encoder_copy(LodePNG_Encoder* dest, const LodePNG_Encoder* source)
{
  LodePNG_Encoder_cleanup(dest);
  *dest = *source;
  LodePNG_InfoPng_init(&dest->infoPng);
  LodePNG_InfoRaw_init(&dest->infoRaw);
  dest->error = LodePNG_InfoPng_copy(&dest->infoPng, &source->infoPng);
  if(dest->error) return;
  dest->error = LodePNG_InfoRaw_copy(&dest->infoRaw, &source->infoRaw);
  if(dest->error) return;
}

#endif /*LODEPNG_COMPILE_ENCODER*/

#endif /*LODEPNG_COMPILE_PNG*/

#ifdef LODEPNG_COMPILE_ERROR_TEXT

/*
This returns the description of a numerical error code in English. This is also
the documentation of all the error codes.
*/
const char* LodePNG_error_text(unsigned code)
{
  switch(code)
  {
    case 0: return "no error, everything went ok";
    case 1: return "nothing done yet"; /*the Encoder/Decoder has done nothing yet, error checking makes no sense yet*/
    case 10: return "end of input memory reached without huffman end code"; /*while huffman decoding*/
    case 11: return "error in code tree made it jump outside of huffman tree"; /*while huffman decoding*/
    case 13: return "problem while processing dynamic deflate block";
    case 14: return "problem while processing dynamic deflate block";
    case 15: return "problem while processing dynamic deflate block";
    case 16: return "unexisting code while processing dynamic deflate block";
    case 17: return "end of out buffer memory reached while inflating";
    case 18: return "invalid distance code while inflating";
    case 19: return "end of out buffer memory reached while inflating";
    case 20: return "invalid deflate block BTYPE encountered while decoding";
    case 21: return "NLEN is not ones complement of LEN in a deflate block";

     /*end of out buffer memory reached while inflating:
     This can happen if the inflated deflate data is longer than the amount of bytes required to fill up
     all the pixels of the image, given the color depth and image dimensions. Something that doesn't
     happen in a normal, well encoded, PNG image.*/
    case 22: return "end of out buffer memory reached while inflating";
    
    case 23: return "end of in buffer memory reached while inflating";
    case 24: return "invalid FCHECK in zlib header";
    case 25: return "invalid compression method in zlib header";
    case 26: return "FDICT encountered in zlib header while it's not used for PNG";
    case 27: return "PNG file is smaller than a PNG header";
    /*Checks the magic file header, the first 8 bytes of the PNG file*/
    case 28: return "incorrect PNG signature, it's no PNG or corrupted";
    case 29: return "first chunk is not the header chunk";
    case 30: return "chunk length too large, chunk broken off at end of file";
    case 31: return "illegal PNG color type or bpp";
    case 32: return "illegal PNG compression method";
    case 33: return "illegal PNG filter method";
    case 34: return "illegal PNG interlace method";
    case 35: return "chunk length of a chunk is too large or the chunk too small";
    case 36: return "illegal PNG filter type encountered";
    case 37: return "illegal bit depth for this color type given";
    case 38: return "the palette is too big"; /*more than 256 colors*/
    case 39: return "more palette alpha values given in tRNS chunk than there are colors in the palette";
    case 40: return "tRNS chunk has wrong size for greyscale image";
    case 41: return "tRNS chunk has wrong size for RGB image";
    case 42: return "tRNS chunk appeared while it was not allowed for this color type";
    case 43: return "bKGD chunk has wrong size for palette image";
    case 44: return "bKGD chunk has wrong size for greyscale image";
    case 45: return "bKGD chunk has wrong size for RGB image";
    /*Is the palette too small?*/
    case 46: return "a value in indexed image is larger than the palette size (bitdepth = 8)";
    /*Is the palette too small?*/
    case 47: return "a value in indexed image is larger than the palette size (bitdepth < 8)";
    /*the input data is empty, maybe a PNG file doesn't exist or is in the wrong path*/
    case 48: return "empty input or file doesn't exist";
    case 49: return "jumped past memory while generating dynamic huffman tree";
    case 50: return "jumped past memory while generating dynamic huffman tree";
    case 51: return "jumped past memory while inflating huffman block";
    case 52: return "jumped past memory while inflating";
    case 53: return "size of zlib data too small";

    /*jumped past tree while generating huffman tree, this could be when the
    tree will have more leaves than symbols after generating it out of the
    given lenghts. They call this an oversubscribed dynamic bit lengths tree in zlib.*/
    case 55: return "jumped past tree while generating huffman tree";
    
    case 56: return "given output image colorType or bitDepth not supported for color conversion";
    case 57: return "invalid CRC encountered (checking CRC can be disabled)";
    case 58: return "invalid ADLER32 encountered (checking ADLER32 can be disabled)";
    case 59: return "conversion to unexisting color mode or color mode conversion not supported";
    case 60: return "invalid window size given in the settings of the encoder (must be 0-32768)";
    case 61: return "invalid BTYPE given in the settings of the encoder (only 0, 1 and 2 are allowed)";
    /*LodePNG leaves the choice of RGB to greyscale conversion formula to the user.*/
    case 62: return "conversion from RGB to greyscale not supported";
    case 63: return "length of a chunk too long, max allowed for PNG is 2147483647 bytes per chunk"; /*(2^31-1)*/
    /*this would result in the inability of a deflated block to ever contain an end code. It must be at least 1.*/
    case 64: return "the length of the END symbol 256 in the Huffman tree is 0"; 
    case 66: return "the length of a text chunk keyword given to the encoder is longer than the maximum of 79 bytes";
    case 67: return "the length of a text chunk keyword given to the encoder is smaller than the minimum of 1 byte";
    case 68: return "tried to encode a PLTE chunk with a palette that has less than 1 or more than 256 colors";
    case 69: return "unknown chunk type with 'critical' flag encountered by the decoder";
    case 71: return "unexisting interlace mode given to encoder (must be 0 or 1)";
    case 72: return "while decoding, unexisting compression method encountering in zTXt or iTXt chunk (it must be 0)";
    case 73: return "invalid tIME chunk size";
    case 74: return "invalid pHYs chunk size";
    /*length could be wrong, or data chopped off*/
    case 75: return "no null termination char found while decoding text chunk";
    case 76: return "iTXt chunk too short to contain required bytes";
    case 77: return "integer overflow in buffer size";
    case 78: return "failed to open file for reading"; /*file doesn't exist or couldn't be opened for reading*/
    case 79: return "failed to open file for writing";
    case 80: return "tried creating a tree of 0 symbols";
    case 81: return "lazy matching at pos 0 is impossible";
    default: ; /*nothing to do here, checks for other error values are below*/
  }

  if(code >= 9900 && code <= 9999) return "memory allocation failed";

  return "unknown error code";
}

#endif /*LODEPNG_COMPILE_ERROR_TEXT*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // End of PNG related code. Begin of C++ wrapper.                       // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef __cplusplus
namespace LodePNG
{
#ifdef LODEPNG_COMPILE_DISK
  void loadFile(std::vector<unsigned char>& buffer, const std::string& filename)
  {
    std::ifstream file(filename.c_str(), std::ios::in|std::ios::binary|std::ios::ate);

    /*get filesize*/
    std::streamsize size = 0;
    if(file.seekg(0, std::ios::end).good()) size = file.tellg();
    if(file.seekg(0, std::ios::beg).good()) size -= file.tellg();

    /*read contents of the file into the vector*/
    buffer.resize(size_t(size));
    if(size > 0) file.read((char*)(&buffer[0]), size);
  }

  /*write given buffer to the file, overwriting the file, it doesn't append to it.*/
  void saveFile(const std::vector<unsigned char>& buffer, const std::string& filename)
  {
    std::ofstream file(filename.c_str(), std::ios::out|std::ios::binary);
    file.write(buffer.empty() ? 0 : (char*)&buffer[0], std::streamsize(buffer.size()));
  }
#endif //LODEPNG_COMPILE_DISK

/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_ZLIB
#ifdef LODEPNG_COMPILE_DECODER
  unsigned decompress(std::vector<unsigned char>& out, const unsigned char* in, size_t insize,
                      const LodePNG_DecompressSettings& settings)
  {
    unsigned char* buffer = 0;
    size_t buffersize = 0;
    unsigned error = LodePNG_zlib_decompress(&buffer, &buffersize, in, insize, &settings);
    if(buffer)
    {
      out.insert(out.end(), &buffer[0], &buffer[buffersize]);
      free(buffer);
    }
    return error;
  }

  unsigned decompress(std::vector<unsigned char>& out, const std::vector<unsigned char>& in,
                      const LodePNG_DecompressSettings& settings)
  {
    return decompress(out, in.empty() ? 0 : &in[0], in.size(), settings);
  }
#endif //LODEPNG_COMPILE_DECODER

#ifdef LODEPNG_COMPILE_ENCODER
  unsigned compress(std::vector<unsigned char>& out, const unsigned char* in, size_t insize,
                    const LodePNG_CompressSettings& settings)
  {
    unsigned char* buffer = 0;
    size_t buffersize = 0;
    unsigned error = LodePNG_zlib_compress(&buffer, &buffersize, in, insize, &settings);
    if(buffer)
    {
      out.insert(out.end(), &buffer[0], &buffer[buffersize]);
      free(buffer);
    }
    return error;
  }

  unsigned compress(std::vector<unsigned char>& out, const std::vector<unsigned char>& in,
                    const LodePNG_CompressSettings& settings)
  {
    return compress(out, in.empty() ? 0 : &in[0], in.size(), settings);
  }
#endif //LODEPNG_COMPILE_ENCODER
#endif //LODEPNG_COMPILE_ZLIB

#ifdef LODEPNG_COMPILE_PNG
#ifdef LODEPNG_COMPILE_DECODER
  Decoder::Decoder()
  {
    LodePNG_Decoder_init(this);
  }

  Decoder::~Decoder()
  {
    LodePNG_Decoder_cleanup(this);
  }

  void Decoder::operator=(const LodePNG_Decoder& other)
  {
    LodePNG_Decoder_copy(this, &other);
  }

  bool Decoder::hasError() const
  {
    return error != 0;
  }
  unsigned Decoder::getError() const
  {
    return error;
  }

  unsigned Decoder::getWidth() const
  {
    return infoPng.width;
  }

  unsigned Decoder::getHeight() const
  {
    return infoPng.height;
  }

  unsigned Decoder::getBpp()
  {
    return LodePNG_InfoColor_getBpp(&infoPng.color);
  }

  unsigned Decoder::getChannels()
  {
    return LodePNG_InfoColor_getChannels(&infoPng.color);
  }

  unsigned Decoder::isGreyscaleType()
  {
    return LodePNG_InfoColor_isGreyscaleType(&infoPng.color);
  }

  unsigned Decoder::isAlphaType()
  {
    return LodePNG_InfoColor_isAlphaType(&infoPng.color);
  }

  void Decoder::decode(std::vector<unsigned char>& out, const unsigned char* in, size_t insize)
  {
    unsigned char* buffer;
    size_t buffersize;
    LodePNG_Decoder_decode(this, &buffer, &buffersize, in, insize);
    if(buffer)
    {
      out.insert(out.end(), &buffer[0], &buffer[buffersize]);
      free(buffer);
    }
  }

  void Decoder::decode(std::vector<unsigned char>& out, const std::vector<unsigned char>& in)
  {
    decode(out, in.empty() ? 0 : &in[0], in.size());
  }

  void Decoder::inspect(const unsigned char* in, size_t insize)
  {
    LodePNG_Decoder_inspect(this, in, insize);
  }

  void Decoder::inspect(const std::vector<unsigned char>& in)
  {
    inspect(in.empty() ? 0 : &in[0], in.size());
  }

  const LodePNG_DecodeSettings& Decoder::getSettings() const
  {
    return settings;
  }

  LodePNG_DecodeSettings& Decoder::getSettings()
  {
    return settings;
  }

  void Decoder::setSettings(const LodePNG_DecodeSettings& settings)
  {
    this->settings = settings;
  }

  const LodePNG_InfoPng& Decoder::getInfoPng() const
  {
    return infoPng;
  }

  LodePNG_InfoPng& Decoder::getInfoPng()
  {
    return infoPng;
  }

  void Decoder::setInfoPng(const LodePNG_InfoPng& info)
  {
    error = LodePNG_InfoPng_copy(&this->infoPng, &info);
  }

  void Decoder::swapInfoPng(LodePNG_InfoPng& info)
  {
    LodePNG_InfoPng_swap(&this->infoPng, &info);
  }

  const LodePNG_InfoRaw& Decoder::getInfoRaw() const
  {
    return infoRaw;
  }

  LodePNG_InfoRaw& Decoder::getInfoRaw()
  {
    return infoRaw;
  }

  void Decoder::setInfoRaw(const LodePNG_InfoRaw& info)
  {
    error = LodePNG_InfoRaw_copy(&this->infoRaw, &info);
  }

#endif //LODEPNG_COMPILE_DECODER

  /* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_ENCODER

  Encoder::Encoder()
  {
    LodePNG_Encoder_init(this);
  }

  Encoder::~Encoder()
  {
    LodePNG_Encoder_cleanup(this);
  }

  void Encoder::operator=(const LodePNG_Encoder& other)
  {
    LodePNG_Encoder_copy(this, &other);
  }

  bool Encoder::hasError() const
  {
    return error != 0;
  }

  unsigned Encoder::getError() const
  {
    return error;
  }

  void Encoder::encode(std::vector<unsigned char>& out, const unsigned char* image, unsigned w, unsigned h)
  {
    unsigned char* buffer;
    size_t buffersize;
    LodePNG_Encoder_encode(this, &buffer, &buffersize, image, w, h);
    if(buffer)
    {
      out.insert(out.end(), &buffer[0], &buffer[buffersize]);
      free(buffer);
    }
  }

  void Encoder::encode(std::vector<unsigned char>& out, const std::vector<unsigned char>& image,
                       unsigned w, unsigned h)
  {
    encode(out, image.empty() ? 0 : &image[0], w, h);
  }

  void Encoder::clearPalette()
  {
    LodePNG_InfoColor_clearPalette(&infoPng.color);
  }

  void Encoder::addPalette(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
  {
    error = LodePNG_InfoColor_addPalette(&infoPng.color, r, g, b, a);
  }

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  void Encoder::clearText()
  {
    LodePNG_Text_clear(&infoPng.text);
  }

  void Encoder::addText(const std::string& key, const std::string& str)
  {
    error = LodePNG_Text_add(&infoPng.text, key.c_str(), str.c_str());
  }

  void Encoder::clearIText()
  {
    LodePNG_IText_clear(&infoPng.itext);
  }

  void Encoder::addIText(const std::string& key, const std::string& langtag,
                         const std::string& transkey, const std::string& str)
  {
    error = LodePNG_IText_add(&infoPng.itext, key.c_str(), langtag.c_str(), transkey.c_str(), str.c_str());
  }
#endif //LODEPNG_COMPILE_ANCILLARY_CHUNKS

  const LodePNG_EncodeSettings& Encoder::getSettings() const
  {
    return settings;
  }

  LodePNG_EncodeSettings& Encoder::getSettings()
  {
    return settings;
  }

  void Encoder::setSettings(const LodePNG_EncodeSettings& settings)
  {
    this->settings = settings;
  }

  const LodePNG_InfoPng& Encoder::getInfoPng() const
  {
    return infoPng;
  }

  LodePNG_InfoPng& Encoder::getInfoPng()
  {
    return infoPng;
  }

  void Encoder::setInfoPng(const LodePNG_InfoPng& info)
  {
    error = LodePNG_InfoPng_copy(&this->infoPng, &info);
  }

  void Encoder::swapInfoPng(LodePNG_InfoPng& info)
  {
    LodePNG_InfoPng_swap(&this->infoPng, &info);
  }

  const LodePNG_InfoRaw& Encoder::getInfoRaw() const
  {
    return infoRaw;
  }

  LodePNG_InfoRaw& Encoder::getInfoRaw()
  {
    return infoRaw;
  }

  void Encoder::setInfoRaw(const LodePNG_InfoRaw& info)
  {
    error = LodePNG_InfoRaw_copy(&this->infoRaw, &info);
  }
#endif //LODEPNG_COMPILE_ENCODER

  /* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_DECODER

  unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h, const unsigned char* in,
                  size_t insize, unsigned colorType, unsigned bitDepth)
  {
    Decoder decoder;
    decoder.getInfoRaw().color.colorType = colorType;
    decoder.getInfoRaw().color.bitDepth = bitDepth;
    decoder.decode(out, in, insize);
    w = decoder.getWidth();
    h = decoder.getHeight();
    return decoder.getError();
  }

  unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                  const std::vector<unsigned char>& in, unsigned colorType, unsigned bitDepth)
  {
    return decode(out, w, h, in.empty() ? 0 : &in[0], (unsigned)in.size(), colorType, bitDepth);
  }

#ifdef LODEPNG_COMPILE_DISK
  unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h, const std::string& filename,
                  unsigned colorType, unsigned bitDepth)
  {
    std::vector<unsigned char> buffer;
    loadFile(buffer, filename);
    return decode(out, w, h, buffer, colorType, bitDepth);
  }
#endif //LODEPNG_COMPILE_DECODER
#endif //LODEPNG_COMPILE_DISK

#ifdef LODEPNG_COMPILE_ENCODER

  unsigned encode(std::vector<unsigned char>& out, const unsigned char* in, unsigned w, unsigned h,
                  unsigned colorType, unsigned bitDepth)
  {
    Encoder encoder;
    encoder.getInfoRaw().color.colorType = colorType;
    encoder.getInfoRaw().color.bitDepth = bitDepth;
    encoder.getInfoPng().color.colorType = colorType;
    encoder.getInfoPng().color.bitDepth = bitDepth;
    encoder.encode(out, in, w, h);
    return encoder.getError();
  }

  unsigned encode(std::vector<unsigned char>& out, const std::vector<unsigned char>& in, unsigned w, unsigned h,
                  unsigned colorType, unsigned bitDepth)
  {
    return encode(out, in.empty() ? 0 : &in[0], w, h, colorType, bitDepth);
  }

#ifdef LODEPNG_COMPILE_DISK
  unsigned encode(const std::string& filename, const unsigned char* in, unsigned w, unsigned h,
                  unsigned colorType, unsigned bitDepth)
  {
    std::vector<unsigned char> buffer;
    Encoder encoder;
    encoder.getInfoRaw().color.colorType = colorType;
    encoder.getInfoRaw().color.bitDepth = bitDepth;
    encoder.encode(buffer, in, w, h);
    if(!encoder.hasError()) saveFile(buffer, filename);
    return encoder.getError();
  }

  unsigned encode(const std::string& filename, const std::vector<unsigned char>& in, unsigned w, unsigned h,
                  unsigned colorType, unsigned bitDepth)
  {
    return encode(filename, in.empty() ? 0 : &in[0], w, h, colorType, bitDepth);
  }
#endif //LODEPNG_COMPILE_DISK
#endif //LODEPNG_COMPILE_ENCODER
#endif //LODEPNG_COMPILE_PNG
} //namespace LodePNG
#endif /*__cplusplus C++ RAII wrapper*/
