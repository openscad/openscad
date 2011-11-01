#ifndef _yee_compare_h
#define _yee_compare_h

// source code modified for OpenSCAD, Sept 2011
// original copyright notice follows:
/*
Metric
RGBAImage.h
Comapre Args
Laplacian Pyramid
Copyright (C) 2006 Yangli Hector Yee

This program is free software; you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program;
if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <string>

class RGBAImage;

// Args to pass into the comparison function
class CompareArgs
{
public:
	CompareArgs();
	~CompareArgs();
	bool Parse_Args(int argc, char **argv);	
	void Print_Args();
	
	RGBAImage		*ImgA;				// Image A
	RGBAImage		*ImgB;				// Image B
	RGBAImage		*ImgDiff;			// Diff image
	bool			Verbose;			// Print lots of text or not
	bool			LuminanceOnly;		// Only consider luminance; ignore chroma channels in the comparison.
	float			FieldOfView;		// Field of view in degrees
	float			Gamma;				// The gamma to convert to linear color space
	float			Luminance;			// the display's luminance
	unsigned int	ThresholdPixels;	// How many pixels different to ignore
	std::string		ErrorStr;			// Error string
  // How much color to use in the metric.
  // 0.0 is the same as LuminanceOnly = true,
  // 1.0 means full strength.
  float ColorFactor;
  // How much to down sample image before comparing, in powers of 2.
  int DownSample;
};

#define MAX_PYR_LEVELS 8

class LPyramid
{
public:	
	LPyramid(float *image, int width, int height);
	virtual ~LPyramid();
	float Get_Value(int x, int y, int level);
protected:
	float *Copy(float *img);
	void Convolve(float *a, float *b);
	
	// Succesively blurred versions of the original image
	float *Levels[MAX_PYR_LEVELS];

	int Width;
	int Height;
};

class CompareArgs;

// Image comparison metric using Yee's method
// References: A Perceptual Metric for Production Testing, Hector Yee, Journal of Graphics Tools 2004
bool Yee_Compare(CompareArgs &args);

/** Class encapsulating an image containing R,G,B,A channels.
 *
 * Internal representation assumes data is in the ABGR format, with the RGB
 * color channels premultiplied by the alpha value.  Premultiplied alpha is
 * often also called "associated alpha" - see the tiff 6 specification for some
 * discussion - http://partners.adobe.com/asn/developer/PDFS/TN/TIFF6.pdf
 *
 */
class RGBAImage
{
	RGBAImage(const RGBAImage&);
	RGBAImage& operator=(const RGBAImage&);
public:
	RGBAImage(int w, int h, const char *name = 0)
	{
		Width = w;
		Height = h;
		if (name) Name = name;
		Data = new unsigned int[w * h];
	};
	~RGBAImage() { if (Data) delete[] Data; }
	unsigned char Get_Red(unsigned int i) { return (Data[i] & 0xFF); }
	unsigned char Get_Green(unsigned int i) { return ((Data[i]>>8) & 0xFF); }
	unsigned char Get_Blue(unsigned int i) { return ((Data[i]>>16) & 0xFF); }
	unsigned char Get_Alpha(unsigned int i) { return ((Data[i]>>24) & 0xFF); }
	void Set(unsigned char r, unsigned char g, unsigned char b, unsigned char a, unsigned int i)
	{ Data[i] = r | (g << 8) | (b << 16) | (a << 24); }
	int Get_Width(void) const { return Width; }
	int Get_Height(void) const { return Height; }
	void Set(int x, int y, unsigned int d) { Data[x + y * Width] = d; }
	unsigned int Get(int x, int y) const { return Data[x + y * Width]; }
	unsigned int Get(int i) const { return Data[i]; }
	const std::string &Get_Name(void) const { return Name; }
   RGBAImage* DownSample() const;
	
	bool WriteToFile(const char* filename);
	static RGBAImage* ReadFromFile(const char* filename);
	
protected:
	int Width;
	int Height;
	std::string Name;
	unsigned int *Data;
};

#endif
