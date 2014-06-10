/*
diffpng - a program that compares two images

based on the paper :
A perceptual metric for production testing. Journal of graphics tools,
9(4):33-40, 2004, Hector Yee

Copyright (C) 2006-2011 Yangli Hector Yee
Copyright (C) 2011-2014 Steven Myint
(Some of this file was rewritten by Jim Tilander)
Copyright (C) 2014 Don Bright

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA
*/

/*
LodePNG Examples

Copyright (c) 2005-2012 Lode Vandevenne

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

This source has been modified from both PerceptualDiff and
LodePNG Examples

*/


// To use this file as a .hpp header file, uncomment the following
//#define DIFFPNG_HEADERONLY


#ifndef DIFFPNG_HPP
#define DIFFPNG_HPP

#include "lodepng.h"
#include <assert.h>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdint.h>
#include <sstream>
#include <string>
#include <vector>

namespace diffpng
{

using namespace std;

/** Class encapsulating an image containing R,G,B,A channels.
 *
 * Internal representation assumes data is in the ABGR format, with the RGB
 * color channels premultiplied by the alpha value. Premultiplied alpha is
 * often also called "associated alpha" - see the tiff 6 specification for some
 * discussion - http://partners.adobe.com/asn/developer/PDFS/TN/TIFF6.pdf
 *
 */
class RGBAImage
{
	RGBAImage(const RGBAImage &);
	RGBAImage &operator=(const RGBAImage &);

public:
	RGBAImage(unsigned int w, unsigned int h, const string &name="")
		: Width(w), Height(h), Name(name), Data(w * h)
	{
	}
	unsigned char Get_Red(unsigned int i) const
	{
		return (Data[i%Data.size()] & 0xFF);
	}
	unsigned char Get_Green(unsigned int i) const
	{
		return ((Data[i%Data.size()] >> 8) & 0xFF);
	}
	unsigned char Get_Blue(unsigned int i) const
	{
		return ((Data[i%Data.size()] >> 16) & 0xFF);
	}
	unsigned char Get_Alpha(unsigned int i) const
	{
		return ((Data[i%Data.size()] >> 24) & 0xFF);
	}
	void Set(unsigned char r, unsigned char g, unsigned char b,
			 unsigned char a, unsigned int i)
	{
		Data[i] = r | (g << 8) | (b << 16) | (a << 24);
	}
	unsigned int Get_Width() const
	{
		return Width;
	}
	unsigned int Get_Height() const
	{
		return Height;
	}
	void Set(unsigned int x, unsigned int y, unsigned int d)
	{
		Data[x + y * Width] = d;
	}
	unsigned int Get(unsigned int x, unsigned int y) const
	{
		return Data[x + y * Width];
	}
	unsigned int Get(unsigned int i) const
	{
		return Data[i%Data.size()];
	}
	const string &Get_Name() const
	{
		return Name;
	}
	unsigned int *Get_Data()
	{
		return &Data[0];
	}
	const unsigned int *Get_Data() const
	{
		return &Data[0];
	}

	void WriteToFile(const string &filename) const
	{
		cout << "WriteToFile:" << filename << "\n";

		unsigned width = this->Width, height = this->Height;
		vector<unsigned char> image;
		image.resize(width * height * 4);
		for(unsigned y = 0; y < height; y++) {
		for(unsigned x = 0; x < width; x++) {
			unsigned char red, green, blue, alpha;
			red = Get_Red( y*width + x );
			green = Get_Green( y*width + x );
			blue = Get_Blue( y*width + x );
			alpha = Get_Alpha( y*width + x );
			image[4 * width * y + 4 * x + 0] = red;
			image[4 * width * y + 4 * x + 1] = blue;
			image[4 * width * y + 4 * x + 2] = green;
			image[4 * width * y + 4 * x + 3] = alpha;
		}
		}

		//Encode from raw pixels to disk with a single function call
		//The image argument has width * height RGBA pixels or width * height * 4 bytes
		unsigned error = lodepng::encode(filename.c_str(), image, width, height);

		if(error) cout << "encoder error " << error << ": "<< lodepng_error_text(error) << endl;
	}

	static RGBAImage *ReadFromFile(const string &filename)
	{
		cout << "reading from file:" << filename << "\n";
		vector<unsigned char> lodepng_image; //the raw pixels
		unsigned width, height;
		unsigned error = lodepng::decode(lodepng_image, width, height, filename.c_str());
		if (error) {
			cout << "decoder error " << error << ": " << lodepng_error_text(error) << endl;
			return NULL;
		}

		//the pixels are now in the vector "image", 4 bytes per pixel, 
		//ordered RGBARGBA..., use it as texture, draw it, ...
		cout << "width " << width << ", height " << height << "\n";

		RGBAImage *rgbaimg = new RGBAImage(width,height,filename);

		for(unsigned y = 0; y < height; y += 1) {
		for(unsigned x = 0; x < width; x += 1) {
			uint32_t red = lodepng_image[4 * y * width + 4 * x + 0]; //red
			uint32_t green = lodepng_image[4 * y * width + 4 * x + 1]; //green
			uint32_t blue = lodepng_image[4 * y * width + 4 * x + 2]; //blue
			uint32_t alpha = lodepng_image[4 * y * width + 4 * x + 3]; //alpha
			rgbaimg->Set( red, green, blue, alpha, y*width+x );
		}
		}
		return rgbaimg;
	}

	void UpSample()
	{
		unsigned char red, green, blue, alpha;
		unsigned oldwidth = Width;
		//unsigned oldheight = Height;
		unsigned newwidth = Width*2;
		unsigned newheight = Height*2;
		RGBAImage newimg( newwidth, newheight, this->Name );
		for (unsigned x = 0; x < newwidth; x++) {
		for (unsigned y = 0; y < newheight; y++) {
                        red = this->Get_Red(     (y/2)*oldwidth + (x/2) );
			green = this->Get_Green( (y/2)*oldwidth + (x/2) );
			blue = this->Get_Blue(   (y/2)*oldwidth + (x/2) );
			alpha = this->Get_Alpha( (y/2)*oldwidth + (x/2) );
			newimg.Set( red, green, blue, alpha, y*newwidth+x );
		}
		}
		Width = newwidth;
		Height = newheight;
		Data.clear();
		Data.resize( newimg.Data.size() );
		for (unsigned i=0;i<newimg.Data.size();i++) {
			Data[i] = newimg.Data[i];
		}
	}

	// make the image half its original size.
	// this will slightly blur the image.
	// the result somewhat resembles antialiasing.
	void DownSample()
	{
		unsigned int redsum,greensum,bluesum,alphasum;
		unsigned int redavg,greenavg,blueavg,alphaavg;
		unsigned char red, green, blue, alpha;
		unsigned oldwidth = Width;
		//unsigned oldheight = Height;
		unsigned newwidth = Width/2;
		unsigned newheight = Height/2;
		RGBAImage newimg( newwidth, newheight, this->Name );
		for (unsigned x = 0; x < newwidth; x++) {
		for (unsigned y = 0; y < newheight; y++) {
			redsum=greensum=bluesum=alphasum=0;
			redavg=greenavg=blueavg=alphaavg=0;
			for (int i=-1;i<=1;i++) {
			for (int j=-1;j<=1;j++) {
	                        red = this->Get_Red(     (y*2+i)*oldwidth + (x*2+j) );
				green = this->Get_Green( (y*2+i)*oldwidth + (x*2+j) );
				blue = this->Get_Blue(   (y*2+i)*oldwidth + (x*2+j) );
				alpha = this->Get_Alpha( (y*2+i)*oldwidth + (x*2+j) );
				redsum += red;
				greensum += green;
				bluesum += blue;
				alphasum += alpha;
			}
			}
			redavg = redsum / 9;
			greenavg = greensum / 9;
			blueavg = bluesum / 9;
			alphaavg = alphasum / 9;
			newimg.Set( redavg, greenavg, blueavg, alphaavg, y*newwidth+x );
		}
		}
		Width = newwidth;
		Height = newheight;
		Data.clear();
		Data.resize( newimg.Data.size() );
		for (unsigned i=0;i<newimg.Data.size();i++) {
			Data[i] = newimg.Data[i];
		}
	}

	// this somewhat resembles antialiasing.
	void SimpleBlur()
	{
		unsigned int redsum,greensum,bluesum,alphasum;
		unsigned int redavg,greenavg,blueavg,alphaavg;
		unsigned char red, green, blue, alpha;
		for (unsigned x = 0; x < Width; x++) {
		for (unsigned y = 0; y < Height; y++) {
			redsum=greensum=bluesum=alphasum=0;
			redavg=greenavg=blueavg=alphaavg=0;
			for (int i=-1;i<=1;i++) {
			for (int j=-1;j<=1;j++) {
	                        red = this->Get_Red(     (y+i)*Width + (x+j) );
				green = this->Get_Green( (y+i)*Width + (x+j) );
				blue = this->Get_Blue(   (y+i)*Width + (x+j) );
				alpha = this->Get_Alpha( (y+i)*Width + (x+j) );
				redsum += red;
				greensum += green;
				bluesum += blue;
				alphasum += alpha;
			}
			}
			redavg = redsum / 9;
			greenavg = greensum / 9;
			blueavg = bluesum / 9;
			alphaavg = alphasum / 9;
			this->Set( redavg, greenavg, blueavg, alphaavg, y*Width+x );
		}
		}
	}
private:
	unsigned int Width;
	unsigned int Height;
	const string Name;
	vector<unsigned int> Data;
};




/*
--------------------------------------Compare Args
*/

string copyright(
	"diffpng version 2014,\n\
based on PerceptualDiff Copyright (C) 2006 Yangli Hector Yee\n\
diffpng and PerceptualDiff comes with ABSOLUTELY NO WARRANTY;\n\
This is free software, and you are welcome\n\
to redistribute it under certain conditions;\n\
See the GPL page for details: http://www.gnu.org/copyleft/gpl.html\n\n");


string usage("Usage: diffpng image1 image2\n\
\n\
Compares image1 and image2 using Yee's perceptually based image metric.\n\
Returns 0 on PASS (perceptually similar), 1 on FAIL (perceptually different)\n\
\n\
Options:\n\
 --fov deg	 Field of view in degrees (0.1 to 89.9)\n\
 --threshold p   # of pixels p below which differences are ignored\n\
 --gamma g	 Value to convert rgb into linear space (default 2.2)\n\
 --luminance l   White luminance (default 100.0 cdm^-2)\n\
 --luminanceonly Only consider luminance; ignore chroma (color) in the comparison\n\
 --colorfactor   How much of color to use, 0.0 to 1.0, 0.0 = ignore color.\n\
 --sum-errors	 Print a sum of the luminance and color differences.\n\
 --output o.png  Write difference image to o.png (black=same, red=differ)\n\
 --initmax n     Set the initial maximum number of Laplacian Pyramid Levels\n\
 --finalmax n    Set the final maximum number of Laplacian Pyramid Levels\n\
 --flipexit      Flip the normal return values: PASS returns 1, FAIL returns 0\n\
 --quiet         Turns off verbose mode\n\
\n");



template <typename T>
static T lexical_cast(const string &input)
{
	stringstream ss(input);
	T output;
	if (not (ss >> output))
	{
		cout << "invalid_argument(""):" << input;
	}
	return output;
}


static bool option_matches(const char *arg, const string &option_name)
{
	string string_arg(arg);

	return (string_arg == "--" + option_name) or
		   (string_arg == "-" + option_name);
}


class RGBAImage;

// Args to pass into the comparison function
class CompareArgs
{
public:
	CompareArgs()
	{
		// use some nice defaults that will 'just work' for most cases
		// heavy on luminance, light on color (colorfactor 0.1)
		Verbose = true;
		LuminanceOnly = false;
		SumErrors = false;
		FieldOfView = 45.0f;
		Gamma = 2.2f;
		ThresholdPixels = 100;
		Luminance = 100.0f;
		ColorFactor = 0.1f;
		MaxPyramidLevels = 2;
		//FinalMaxPyramidLevels = 5; // 58 fails
		FinalMaxPyramidLevels = 3; // 57 fails
		//FinalMaxPyramidLevels = 2; // 75 fails
		FlipExit = false;
		ImgA = ImgB = ImgDiff = NULL;
	}
	bool Parse_Args(int argc, char **argv)
	{
		if (argc < 3)
		{
			stringstream ss;
			ss << copyright;
			ss << usage;
			ss << "\n";
			ErrorStr = ss.str();
			return false;
		}
		unsigned image_count = 0u;
		const char *output_file_name = NULL;
		for (int i = 1; i < argc; i++)
		{
//			try
//			{
				if (option_matches(argv[i], "fov"))
				{
					if (++i < argc)
					{
						FieldOfView = lexical_cast<float>(argv[i]);
					}
				}
				else if (option_matches(argv[i], "quiet"))
				{
					Verbose = false;
				}
				else if (option_matches(argv[i], "flipexit"))
				{
					FlipExit = true;
				}
				else if (option_matches(argv[i], "threshold"))
				{
					if (++i < argc)
					{
						int temporary = lexical_cast<int>(argv[i]);
						if (temporary < 0)
						{
							cout << " invalid_argument(" <<
								"-threshold must be positive";
						}
						ThresholdPixels = static_cast<unsigned int>(temporary);
					}
				}
				else if (option_matches(argv[i], "gamma"))
				{
					if (++i < argc)
					{
						Gamma = lexical_cast<float>(argv[i]);
					}
				}
				else if (option_matches(argv[i], "initmax"))
				{
					if (++i < argc)
					{
						MaxPyramidLevels = lexical_cast<int>(argv[i]);
					}
					if (MaxPyramidLevels<2 || MaxPyramidLevels>8) {
						cout << "Error: MaxPyramidLevels must be between >1 and <9\n";
						return false;
					}
				}
				else if (option_matches(argv[i], "finalmax"))
				{
					if (++i < argc)
					{
						FinalMaxPyramidLevels = lexical_cast<int>(argv[i]);
					}
					if (FinalMaxPyramidLevels<2 || FinalMaxPyramidLevels>8) {
						cout << "Error: FinalMaxPyramidLevels must be between >1 and <9\n";
						return false;
					}
				}
				else if (option_matches(argv[i], "luminance"))
				{
					if (++i < argc)
					{
						Luminance = lexical_cast<float>(argv[i]);
					}
				}
				else if (option_matches(argv[i], "luminanceonly"))
				{
					LuminanceOnly = true;
				}
				else if (option_matches(argv[i], "sum-errors"))
				{
					SumErrors = true;
				}
				else if (option_matches(argv[i], "colorfactor"))
				{
					if (++i < argc)
					{
						ColorFactor = lexical_cast<float>(argv[i]);
					}
				}
				else if (option_matches(argv[i], "output"))
				{
					if (++i < argc)
					{
						output_file_name = argv[i];
					}
				}
				else if (image_count < 2)
				{
					RGBAImage *img = RGBAImage::ReadFromFile(argv[i]);
					if (not img)
					{
						ErrorStr = "FAILCannot open ";
						ErrorStr += argv[i];
						ErrorStr += "\n";
						return false;
					}
					else
					{
						++image_count;
						if (image_count == 1)
						{
							ImgA = img;
						}
						else
						{
							ImgB = img;
						}
					}
				}
				else if (option_matches(argv[i], "help"))
				{
					cout << usage;
					return false;
				}
				else
				{
					cerr << "Warningoption/file \"" << argv[i]
						  << "\" ignored\n";
				}
//			}
/*			catch (const invalid_argument &exception)
			{
				string reason = "";
				if (not string(exception.what()).empty())
				{
					reason = string("; ") + exception.what();
				}
				cout << "Invalid argument (" << string(argv[i]) <<
									 ") for " << argv[i - 1] << reason;
				return false;
			}
*/
		}

		if (not ImgA or not ImgB)
		{
			ErrorStr = "FAILNot enough image files specified\n";
			return false;
		}

		if (output_file_name)
		{
			ImgDiff = new RGBAImage(ImgA->Get_Width(), ImgA->Get_Height(),
					output_file_name);
		}

		return true;
	}

	void Print_Args() const
	{
		cout << "Field of view is " << FieldOfView << " degrees\n"
			  << "Threshold pixels is " << ThresholdPixels << " pixels\n"
			  << "The Gamma is " << Gamma << "\n"
			  << "The Display's luminance is " << Luminance
			  << " candela per meter squared\n"
			  << "The Color Factor is " << ColorFactor << "\n"
			  << "Initial Max Laplacian Pyramid Levels is " << MaxPyramidLevels << "\n"
			  << "Final Max Laplacian Pyramid Levels is " << FinalMaxPyramidLevels << "\n";
	}

	RGBAImage *ImgA;	 // Image A
	RGBAImage *ImgB;	 // Image B
	RGBAImage *ImgDiff;  // Diff image
	bool Verbose;						// Print lots of text or not
	bool LuminanceOnly;  // Only consider luminance; ignore chroma channels in
						 // the
						 // comparison.
	bool SumErrors;  // Print a sum of the luminance and color differences of
					 // each
					 // pixel.
	float FieldOfView;  // Field of view in degrees
	float Gamma;		// The gamma to convert to linear color space
	float Luminance;	// the display's luminance
	unsigned int ThresholdPixels;  // How many pixels different to ignore
	string ErrorStr;		  // Error string

	// How much color to use in the metric.
	// 0.0 is the same as LuminanceOnly = true,
	// 1.0 means full strength.
	float ColorFactor;

	// normally we return 0 on PASS, 1 on FAIL. this can flip it.
	bool FlipExit;

	// Fewer Max Pyramid Levels makes it run faster, but will return
	// false FAILs. We 'climb' from a low number, and only go higher
	// if there is a FAIL... to further verify the FAIL.
	// This allows PASS to run relatively fast.
	unsigned int MaxPyramidLevels;
	unsigned int FinalMaxPyramidLevels;
};


/*class ParseException : public virtual invalid_argument
{
public:

	ParseException(const string &message)
		: invalid_argument(message)
	{
	}
};
*/


/*

------------------------------- Laplacian Pyramid

*/

static vector<float> Copy(const float *img,
							   const unsigned int width,
							   const unsigned int height)
{
	const unsigned long max = width * height;
	vector<float> out(max);
	for (unsigned long i = 0u; i < max; i++)
	{
		out[i] = img[i];
	}

	return out;
}

class LPyramid
{
public:
	LPyramid(const float *image, unsigned int width, unsigned int height, unsigned int maxlevels)
	: Width(width), Height(height), MaxPyramidLevels(maxlevels)
	{
		this->Levels.resize(MaxPyramidLevels);
		// Make the Laplacian pyramid by successively
		// copying the earlier levels and blurring them
		for (unsigned i = 0u; i < maxlevels; i++)
		{
			if (i == 0 or width * height <= 1)
			{
				Levels[i] = Copy(image, width, height);
			}
			else
			{
				Levels[i].resize(Width * Height);
				Convolve(Levels[i], Levels[i - 1]);
			}
		}
	}

	float Get_Value(unsigned int x, unsigned int y, unsigned int level) const
	{
		const size_t index = x + y * Width;
		assert(level < MaxPyramidLevels);
		return Levels[level][index];
	}

private:
	// Convolves image b with the filter kernel and stores it in a.
	void Convolve(vector<float> &a, const vector<float> &b) const
	{
		assert(a.size() > 1);
		assert(b.size() > 1);

		const float Kernel[] = {0.05f, 0.25f, 0.4f, 0.25f, 0.05f};
//#pragma omp parallel for
		for (unsigned y = 0u; y < Height; y++)
		{
			for (unsigned x = 0u; x < Width; x++)
			{
				size_t index = y * Width + x;
				a[index] = 0.0f;
				for (int i = -2; i <= 2; i++)
				{
					for (int j = -2; j <= 2; j++)
					{
						int nx = x + i;
						int ny = y + j;
						if (nx < 0)
						{
							nx = -nx;
						}
						if (ny < 0)
						{
							ny = -ny;
						}
						if (nx >= static_cast<long>(Width))
						{
							nx = 2 * Width - nx - 1;
						}
						if (ny >= static_cast<long>(Height))
						{
							ny = 2 * Height - ny - 1;
						}
						a[index] +=
							Kernel[i + 2] * Kernel[j + 2] * b[ny * Width + nx];
					}
				}
			}
		}
	}

	// Successively blurred versions of the original image
	vector< vector<float> > Levels;

	unsigned int Width;
	unsigned int Height;
	unsigned int MaxPyramidLevels;
}; //LPyramid


/*
Metric
Copyright (C) 2006-2011 Yangli Hector Yee
Copyright (C) 2011-2014 Steven Myint

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef M_PI
#define M_PI 3.14159265f
#endif

class CompareArgs;

/*
* Given the adaptation luminance, this function returns the
* threshold of visibility in cd per m^2
* TVI means Threshold vs Intensity function
* This version comes from Ward Larson Siggraph 1997
*/
static float tvi(float adaptation_luminance)
{
	// returns the threshold luminance given the adaptation luminance
// units are candelas per meter squared

	const float log_a = log10f(adaptation_luminance);

	float r;
	if (log_a < -3.94f)
	{
		r = -2.86f;
	}
	else if (log_a < -1.44f)
	{
		r = powf(0.405f * log_a + 1.6f, 2.18f) - 2.86f;
	}
	else if (log_a < -0.0184f)
	{
		r = log_a - 0.395f;
	}
	else if (log_a < 1.9f)
	{
		r = powf(0.249f * log_a + 0.65f, 2.7f) - 0.72f;
	}
	else
	{
		r = log_a - 1.255f;
	}

	return powf(10.0f, r);
}


// computes the contrast sensitivity function (Barten SPIE 1989)
// given the cycles per degree (cpd) and luminance (lum)
static float csf(float cpd, float lum)
{
	const float a = 440.f * powf((1.f + 0.7f / lum), -0.2f);
	const float b = 0.3f * powf((1.0f + 100.0f / lum), 0.15f);

	return a * cpd * expf(-b * cpd) * sqrtf(1.0f + 0.06f * expf(b * cpd));
}


/*
* Visual Masking Function
* from Daly 1993
*/
static float mask(float contrast)
{
	const float a = powf(392.498f * contrast, 0.7f);
	const float b = powf(0.0153f * a, 4.f);
	return powf(1.0f + b, 0.25f);
}


// convert Adobe RGB (1998) with reference white D65 to XYZ
static void AdobeRGBToXYZ(float r, float g, float b,
						  float &x, float &y, float &z)
{
	// matrix is from http://www.brucelindbloom.com/
	x = r * 0.576700f + g * 0.185556f + b * 0.188212f;
	y = r * 0.297361f + g * 0.627355f + b * 0.0752847f;
	z = r * 0.0270328f + g * 0.0706879f + b * 0.991248f;
}


struct White
{
	White()
	{
		AdobeRGBToXYZ(1.f, 1.f, 1.f, x, y, z);
	}

	float x;
	float y;
	float z;
};


static const White global_white;


static void XYZToLAB(float x, float y, float z, float &L, float &A, float &B)
{
	const float epsilon = 216.0f / 24389.0f;
	const float kappa = 24389.0f / 27.0f;
	float f[3];
	float r[3];
	r[0] = x / global_white.x;
	r[1] = y / global_white.y;
	r[2] = z / global_white.z;
	for (unsigned int i = 0; i < 3; i++)
	{
		if (r[i] > epsilon)
		{
			f[i] = powf(r[i], 1.0f / 3.0f);
		}
		else
		{
			f[i] = (kappa * r[i] + 16.0f) / 116.0f;
		}
	}
	L = 116.0f * f[1] - 16.0f;
	A = 500.0f * (f[0] - f[1]);
	B = 200.0f * (f[1] - f[2]);
}


static unsigned int adaptation(float num_one_degree_pixels, unsigned int max_pyramid_levels)
{
	float num_pixels = 1.f;
	unsigned adaptation_level = 0u;
	for (unsigned i = 0u; i < max_pyramid_levels; i++)
	{
		adaptation_level = i;
		if (num_pixels > num_one_degree_pixels)
		{
			break;
		}
		num_pixels *= 2;
	}
	return adaptation_level;  // LCOV_EXCL_LINE
}


bool Yee_Compare_Engine(CompareArgs &args)
{
	if ((args.ImgA->Get_Width() != args.ImgB->Get_Width())or(
			args.ImgA->Get_Height() != args.ImgB->Get_Height()))
	{
		args.ErrorStr = "Image dimensions do not match\n";
		return false;
	}

	const unsigned dim = args.ImgA->Get_Width() * args.ImgA->Get_Height();
	bool identical = true;
	for (unsigned i = 0u; i < dim; i++)
	{
		if (args.ImgA->Get(i) != args.ImgB->Get(i))
		{
			identical = false;
			break;
		}
	}
	if (identical)
	{
		cout << "Images are binary identical\n";
	}

	// assuming colorspaces are in Adobe RGB (1998) convert to XYZ
	vector<float> aX(dim),aY(dim),aZ(dim),bX(dim),bY(dim),bZ(dim);
	vector<float> aLum(dim),bLum(dim),aA(dim),bA(dim),aB(dim),bB(dim);

	if (args.Verbose) cout << "Converting RGB to XYZ\n";

	const unsigned w = args.ImgA->Get_Width();
	const unsigned h = args.ImgA->Get_Height();
//#pragma omp parallel for
	for (unsigned y = 0u; y < h; y++)
	{
		for (unsigned x = 0u; x < w; x++)
		{
			const unsigned i = x + y * w;
			float r = powf(args.ImgA->Get_Red(i) / 255.0f, args.Gamma);
			float g = powf(args.ImgA->Get_Green(i) / 255.0f, args.Gamma);
			float b = powf(args.ImgA->Get_Blue(i) / 255.0f, args.Gamma);
			AdobeRGBToXYZ(r, g, b, aX[i], aY[i], aZ[i]);
			float l;
			XYZToLAB(aX[i], aY[i], aZ[i], l, aA[i], aB[i]);
			r = powf(args.ImgB->Get_Red(i) / 255.0f, args.Gamma);
			g = powf(args.ImgB->Get_Green(i) / 255.0f, args.Gamma);
			b = powf(args.ImgB->Get_Blue(i) / 255.0f, args.Gamma);
			AdobeRGBToXYZ(r, g, b, bX[i], bY[i], bZ[i]);
			XYZToLAB(bX[i], bY[i], bZ[i], l, bA[i], bB[i]);
			aLum[i] = aY[i] * args.Luminance;
			bLum[i] = bY[i] * args.Luminance;
		}
	}

	if (args.Verbose)
	{
		cout << "Constructing Laplacian Pyramids\n";
	}

	const LPyramid la(&aLum[0], w, h, args.MaxPyramidLevels);
	const LPyramid lb(&bLum[0], w, h, args.MaxPyramidLevels);

	const float num_one_degree_pixels =
		2.f * tan(args.FieldOfView * 0.5 * M_PI / 180) * 180 / M_PI;
	const float pixels_per_degree = w / num_one_degree_pixels;

	if (args.Verbose)
	{
		cout << "Performing test\n";
	}

	const unsigned adaptation_level = adaptation(num_one_degree_pixels, args.MaxPyramidLevels);

	vector<float> cpd(args.MaxPyramidLevels);
	cpd[0] = 0.5f * pixels_per_degree;
	for (unsigned i = 1u; i < args.MaxPyramidLevels; i++)
	{
		cpd[i] = 0.5f * cpd[i - 1];
	}
	const float csf_max = csf(3.248f, 100.0f);

	assert(args.MaxPyramidLevels >= 2); // ?? >2 or >=2

	float F_freq[args.MaxPyramidLevels - 2];
	for (unsigned i = 0u; i < args.MaxPyramidLevels - 2; i++)
	{
		F_freq[i] = csf_max / csf(cpd[i], 100.0f);
	}

	unsigned pixels_failed = 0u;
	float error_sum = 0.;
//#pragma omp parallel for reduction(+ : pixels_failed) reduction(+ : error_sum)
	for (unsigned y = 0u; y < h; y++)
	{
		for (unsigned x = 0u; x < w; x++)
		{
			const unsigned index = x + y * w;
			float contrast[args.MaxPyramidLevels - 2];
			float sum_contrast = 0;
			for (unsigned i = 0u; i < args.MaxPyramidLevels - 2; i++)
			{
				float n1 =
					fabsf(la.Get_Value(x, y, i) - la.Get_Value(x, y, i + 1));
				float n2 =
					fabsf(lb.Get_Value(x, y, i) - lb.Get_Value(x, y, i + 1));
				float numerator = (n1 > n2) ? n1 : n2;
				float d1 = fabsf(la.Get_Value(x, y, i + 2));
				float d2 = fabsf(lb.Get_Value(x, y, i + 2));
				float denominator = (d1 > d2) ? d1 : d2;
				if (denominator < 1e-5f)
				{
					denominator = 1e-5f;
				}
				contrast[i] = numerator / denominator;
				sum_contrast += contrast[i];
			}
			if (sum_contrast < 1e-5)
			{
				sum_contrast = 1e-5f;
			}
			float F_mask[args.MaxPyramidLevels - 2];
			float adapt = la.Get_Value(x, y, adaptation_level) +
						 lb.Get_Value(x, y, adaptation_level);
			adapt *= 0.5f;
			if (adapt < 1e-5)
			{
				adapt = 1e-5f;
			}
			for (unsigned i = 0u; i < args.MaxPyramidLevels - 2; i++)
			{
				F_mask[i] = mask(contrast[i] * csf(cpd[i], adapt));
			}
			float factor = 0.f;
			for (unsigned i = 0u; i < args.MaxPyramidLevels - 2; i++)
			{
				factor += contrast[i] * F_freq[i] * F_mask[i] / sum_contrast;
			}
			if (factor < 1)
			{
				factor = 1;
			}
			if (factor > 10)
			{
				factor = 10;
			}
			const float delta =
				fabsf(la.Get_Value(x, y, 0) - lb.Get_Value(x, y, 0));
			error_sum += delta;
			bool pass = true;

			// pure luminance test
			if (delta > factor * tvi(adapt))
			{
				pass = false;
			}

			if (not args.LuminanceOnly)
			{
				// CIE delta E test with modifications
				float color_scale = args.ColorFactor;
				// ramp down the color test in scotopic regions
				if (adapt < 10.0f)
				{
					// Don't do color test at all.
					color_scale = 0.0;
				}
				float da = aA[index] - bA[index];
				float db = aB[index] - bB[index];
				da = da * da;
				db = db * db;
				const float delta_e = (da + db) * color_scale;
				error_sum += delta_e;
				if (delta_e > factor)
				{
					pass = false;
				}
			}

			if (not pass)
			{
				pixels_failed++;
				if (args.ImgDiff)
				{
					args.ImgDiff->Set(255, 0, 0, 255, index);
				}
			}
			else
			{
				if (args.ImgDiff)
				{
					args.ImgDiff->Set(0, 0, 0, 255, index);
				}
			}
		}
	}

	stringstream s;
	s << error_sum << " error sum\n";
	const string error_sum_buff = s.str();

	s.str("");
	s << pixels_failed << " pixels failed\n";
	const string different = s.str();

	// Always output image difference if requested.
	if (args.ImgDiff)
	{
		args.ImgDiff->WriteToFile(args.ImgDiff->Get_Name());

		args.ErrorStr += "Wrote difference image to ";
		args.ErrorStr += args.ImgDiff->Get_Name();
		args.ErrorStr += "\n";
	}

	if (pixels_failed < args.ThresholdPixels)
	{
		args.ErrorStr = "Images are roughly the same\n";
		args.ErrorStr += different;
		return true;
	}

	args.ErrorStr = "Images are visibly different\n";
	args.ErrorStr += different;
	if (args.SumErrors)
	{
		args.ErrorStr += error_sum_buff;
	}

	return false;
}

/*
Multi-stage comparison.

This is designed to run faster on sets of images that match (PASS) than
on sets of images that fail (FAIL). The basic idea is as follows:

When the number of Laplacian Pyramid Levels is low, the algorithm runs
relatively fast. It can detect similar images well, but it also FAILS
on images that should not fail. This is because it does not do enough 'blurring'.

Thus, the strategy is as follows.

Start with a low number of Pyramid Levels.... and if the images match, (PASS),
then quit the algorithm.

Now, only if the images dont match (FAIL) do we increase the number of
pyramid levels.

On a typical regression test system, this can create a good speedup. Why?
Imagine you have 400 image tests. Under normal circumstances, your program
will generate test-image output that matches the expected image output.
Thus, most of the comparisons will be of images that will probably match.
That means, this comparison will run relatively fast on all those matches.

Now say you create an experimental new feature for your program, and
want to see if it breaks anything. Well, the algorithm will stil run fast
on the test-output that matches what is expected. . . . it will only slow
down to do higher-levels of Pyramid processing for those few output images that
dont match what is expected.

Lets say your modification of your program causes 5 out of 400 tests to fail.
That means only those 5 will run really slowly.

Let's say that 'fast' means 2 seconds, and 'slow' means 20 seconds. We have
thus taken some code that would have run in 400*20 seconds, 8000 (>2 hours)
and made it run in only 395*2+5*20 seconds. Thats about 15 minutes, roughly
ten times faster.
*/

bool LevelClimberCompare(CompareArgs &args) {
	bool test = false;
	test = Yee_Compare_Engine( args );

	while (test==false && args.MaxPyramidLevels<args.FinalMaxPyramidLevels) {
		cout << "Test failed with Max # Pyramid Levels=" << args.MaxPyramidLevels;
		args.MaxPyramidLevels++;
		cout << ". Rerunning with " << args.MaxPyramidLevels << "\n";
		test = Yee_Compare_Engine( args );
	}
	if (test==false) {
		cout << "Tests failed at final max pyramid level. \n";
		//cout << "Retesting with Downsampling (shrink/blur image)\n";
		cout << "Retesting with downsampling and simple blur\n";

//		args.ImgA->UpSample();
//		args.ImgB->UpSample();
		args.ImgA->DownSample();
		args.ImgB->DownSample();
		if (args.ImgDiff) {
			args.ImgA->WriteToFile( args.ImgDiff->Get_Name()+".1.downsample.png" );
			args.ImgB->WriteToFile( args.ImgDiff->Get_Name()+".2.downsample.png" );
			args.ImgDiff->DownSample();
		}

		// i=1, lots of fail
		// i=2, 8 fail
		// i=3, 7 fail
		for (int i=0;i<3;i++){
			args.ImgA->SimpleBlur();
			args.ImgB->SimpleBlur();
		}

		if (args.ImgDiff) {
			args.ImgA->WriteToFile( args.ImgDiff->Get_Name()+".1.simpleblur.png" );
			args.ImgB->WriteToFile( args.ImgDiff->Get_Name()+".2.simpleblur.png" );
		}

		args.ColorFactor = 0.05;
		test = Yee_Compare_Engine( args );
	}
	return test;
}

#endif // DIFFPNG_HPP

////////////// metric

} // namespace diffpng

#ifndef DIFFPNG_HEADERONLY

// main() is only used for 'cpp' compile mode. to build as .hpp header file
// see the comment at the top of this file.

// is this program running inside a script or from the console?
bool interactive()
{
	return false;
}

int main(int argc, char **argv)
{
	diffpng::CompareArgs args;

	std::string red("\033[40;31m");
	std::string green("\033[40;32m");
	std::string nocolor("\033[0m");

//	try
//	{
		if (not args.Parse_Args(argc, argv))
		{
			std::cout << args.ErrorStr;
			return -1;
		}
		else
		{
			if (args.Verbose)
			{
				args.Print_Args();
			}
		}

		bool passed = diffpng::LevelClimberCompare(args);
		if (passed)
		{
			if (args.Verbose)
			{
				if (interactive()) std::cout << green;
				std::cout << "PASS: result: " << args.ErrorStr;
				if (interactive()) std::cout << nocolor;
			}
		}
		else
		{
			if (interactive()) std::cout << red;
			std::cout << "FAIL: result: " << args.ErrorStr;
			if (interactive()) std::cout << nocolor;
		}

	if (args.FlipExit) passed = !passed;
	if (passed) return 0;
	return 1;
//	}
/*	catch (...)
	{
		std::cerr << "Exception" << std::endl;
		return 1;
	}
*/
}

#endif // ifndef HEADERONLY

