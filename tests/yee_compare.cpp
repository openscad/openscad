// modified from PerceptualDiff source for OpenSCAD, 2011 September

#include "yee_compare.h"
#include "lodepng.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <math.h>

static const char* copyright = 
"PerceptualDiff version 1.1.1, Copyright (C) 2006 Yangli Hector Yee\n\
PerceptualDiff comes with ABSOLUTELY NO WARRANTY;\n\
This is free software, and you are welcome\n\
to redistribute it under certain conditions;\n\
See the GPL page for details: http://www.gnu.org/copyleft/gpl.html\n\n";

static const char *usage =
"PeceptualDiff image1.tif image2.tif\n\n\
   Compares image1.tif and image2.tif using a perceptually based image metric\n\
   Options:\n\
\t-verbose       : Turns on verbose mode\n\
\t-fov deg       : Field of view in degrees (0.1 to 89.9)\n\
\t-threshold p	 : #pixels p below which differences are ignored\n\
\t-gamma g       : Value to convert rgb into linear space (default 2.2)\n\
\t-luminance l   : White luminance (default 100.0 cdm^-2)\n\
\t-luminanceonly : Only consider luminance; ignore chroma (color) in the comparison\n\
\t-colorfactor   : How much of color to use, 0.0 to 1.0, 0.0 = ignore color.\n\
\t-downsample    : How many powers of two to down sample the image.\n\
\t-output o.ppm  : Write difference to the file o.ppm\n\
\n\
\n Note: Input or Output files can also be in the PNG or JPG format or any format\
\n that FreeImage supports.\
\n";

CompareArgs::CompareArgs()
{
	ImgA = NULL;
	ImgB = NULL;
	ImgDiff = NULL;
	Verbose = false;
	LuminanceOnly = false;
	FieldOfView = 45.0f;
	Gamma = 2.2f;
	ThresholdPixels = 100;
	Luminance = 100.0f;
   ColorFactor = 1.0f;
   DownSample = 0;
}

CompareArgs::~CompareArgs()
{
	if (ImgA) delete ImgA;
	if (ImgB) delete ImgB;
	if (ImgDiff) delete ImgDiff;
}

bool CompareArgs::Parse_Args(int argc, char **argv)
{
	if (argc < 3) {
		ErrorStr = copyright;
		ErrorStr += usage;
		return false;
	}
	int image_count = 0;
	const char* output_file_name = NULL;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-fov") == 0) {
			if (++i < argc) {
				FieldOfView = (float) atof(argv[i]);
			}
		} else if (strcmp(argv[i], "-verbose") == 0) {
			Verbose = true;
		} else if (strcmp(argv[i], "-threshold") == 0) {
			if (++i < argc) {
				ThresholdPixels = atoi(argv[i]);
			}
		} else if (strcmp(argv[i], "-gamma") == 0) {
			if (++i < argc) {
				Gamma = (float) atof(argv[i]);
			}
		} else if (strcmp(argv[i], "-luminance") == 0) {
			if (++i < argc) {
				Luminance = (float) atof(argv[i]);
			}
		} else if (strcmp(argv[i], "-luminanceonly") == 0) {
			LuminanceOnly = true;
		} else if (strcmp(argv[i], "-colorfactor") == 0) {
			if (++i < argc) {
				ColorFactor = (float) atof(argv[i]);
			}
		} else if (strcmp(argv[i], "-downsample") == 0) {
			if (++i < argc) {
				DownSample = (int) atoi(argv[i]);
			}
		} else if (strcmp(argv[i], "-output") == 0) {
			if (++i < argc) {
				output_file_name = argv[i];
			}
		} else if (image_count < 2) {
			RGBAImage* img = RGBAImage::ReadFromFile(argv[i]);
			if (!img) {
				ErrorStr = "FAIL: Cannot open ";
				ErrorStr += argv[i];
				ErrorStr += "\n";
				return false;
			} else {
				++image_count;
				if(image_count == 1)
					ImgA = img;
				else
					ImgB = img;
			}
		} else {
			fprintf(stderr, "Warning: option/file \"%s\" ignored\n", argv[i]);
		}
	} // i
	if(!ImgA || !ImgB) {
		ErrorStr = "FAIL: Not enough image files specified\n";
		return false;
	}
   for (int i = 0; i < DownSample; i++) {
      if (Verbose) printf("Downsampling by %d\n", 1 << (i+1));
      RGBAImage *tmp = ImgA->DownSample();
      if (tmp) {
         delete ImgA;
         ImgA = tmp;
      }
      tmp = ImgB->DownSample();
      if (tmp) {
         delete ImgB;
         ImgB = tmp;
      }
   }
	if(output_file_name) {
		ImgDiff = new RGBAImage(ImgA->Get_Width(), ImgA->Get_Height(), output_file_name);
	}
	return true;
}

void CompareArgs::Print_Args()
{
	printf("Field of view is %f degrees\n", FieldOfView);
	printf("Threshold pixels is %d pixels\n", ThresholdPixels);
	printf("The Gamma is %f\n", Gamma);
	printf("The Display's luminance is %f candela per meter squared\n", Luminance);
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

LPyramid::LPyramid(float *image, int width, int height) :
	Width(width),
	Height(height)
{
	// Make the Laplacian pyramid by successively
	// copying the earlier levels and blurring them
	for (int i=0; i<MAX_PYR_LEVELS; i++) {
		if (i == 0) {
			Levels[i] = Copy(image);
		} else {
			Levels[i] = new float[Width * Height];
			Convolve(Levels[i], Levels[i - 1]);
		}
	}
}

LPyramid::~LPyramid()
{
	for (int i=0; i<MAX_PYR_LEVELS; i++) {
		if (Levels[i]) delete Levels[i];
	}
}

float *LPyramid::Copy(float *img)
{
	int max = Width * Height;
	float *out = new float[max];
	for (int i = 0; i < max; i++) out[i] = img[i];
	
	return out;
}

void LPyramid::Convolve(float *a, float *b)
// convolves image b with the filter kernel and stores it in a
{
	int y,x,i,j,nx,ny;
	const float Kernel[] = {0.05f, 0.25f, 0.4f, 0.25f, 0.05f};

	for (y=0; y<Height; y++) {
		for (x=0; x<Width; x++) {
			int index = y * Width + x;
			a[index] = 0.0f;
			for (i=-2; i<=2; i++) {
				for (j=-2; j<=2; j++) {
					nx=x+i;
					ny=y+j;
					if (nx<0) nx=-nx;
					if (ny<0) ny=-ny;
					if (nx>=Width) nx=2*Width-nx-1;
					if (ny>=Height) ny=2*Height-ny-1;
					a[index] += Kernel[i+2] * Kernel[j+2] * b[ny * Width + nx];
				} 
			}
		}
	}
}

float LPyramid::Get_Value(int x, int y, int level)
{
	int index = x + y * Width;
	int l = level;
	if (l > MAX_PYR_LEVELS) l = MAX_PYR_LEVELS;
	return Levels[level][index];
}



#ifndef M_PI
#define M_PI 3.14159265f
#endif

/*
* Given the adaptation luminance, this function returns the
* threshold of visibility in cd per m^2
* TVI means Threshold vs Intensity function
* This version comes from Ward Larson Siggraph 1997
*/ 

float tvi(float adaptation_luminance)
{
      // returns the threshold luminance given the adaptation luminance
      // units are candelas per meter squared

      float log_a, r, result; 
      log_a = log10f(adaptation_luminance);

      if (log_a < -3.94f) {
            r = -2.86f;
      } else if (log_a < -1.44f) {
            r = powf(0.405f * log_a + 1.6f , 2.18f) - 2.86f;
      } else if (log_a < -0.0184f) {
            r = log_a - 0.395f;
      } else if (log_a < 1.9f) {
            r = powf(0.249f * log_a + 0.65f, 2.7f) - 0.72f;
      } else {
            r = log_a - 1.255f;
      }

      result = powf(10.0f , r); 

      return result;

} 

// computes the contrast sensitivity function (Barten SPIE 1989)
// given the cycles per degree (cpd) and luminance (lum)
float csf(float cpd, float lum)
{
	float a, b, result; 
	
	a = 440.0f * powf((1.0f + 0.7f / lum), -0.2f);
	b = 0.3f * powf((1.0f + 100.0f / lum), 0.15f);
		
	result = a * cpd * expf(-b * cpd) * sqrtf(1.0f + 0.06f * expf(b * cpd)); 
	
	return result;	
}

/*
* Visual Masking Function
* from Daly 1993
*/
float mask(float contrast)
{
      float a, b, result;
      a = powf(392.498f * contrast,  0.7f);
      b = powf(0.0153f * a, 4.0f);
      result = powf(1.0f + b, 0.25f); 

      return result;
} 

// convert Adobe RGB (1998) with reference white D65 to XYZ
void AdobeRGBToXYZ(float r, float g, float b, float &x, float &y, float &z)
{
	// matrix is from http://www.brucelindbloom.com/
	x = r * 0.576700f + g * 0.185556f + b * 0.188212f;
	y = r * 0.297361f + g * 0.627355f + b * 0.0752847f;
	z = r * 0.0270328f + g * 0.0706879f + b * 0.991248f;
}

void XYZToLAB(float x, float y, float z, float &L, float &A, float &B)
{
	static float xw = -1;
	static float yw;
	static float zw;
	// reference white
	if (xw < 0) {
		AdobeRGBToXYZ(1, 1, 1, xw, yw, zw);
	}
	const float epsilon  = 216.0f / 24389.0f;
	const float kappa = 24389.0f / 27.0f;
	float f[3];
	float r[3];
	r[0] = x / xw;
	r[1] = y / yw;
	r[2] = z / zw;
	for (int i = 0; i < 3; i++) {
		if (r[i] > epsilon) {
			f[i] = powf(r[i], 1.0f / 3.0f);
		} else {
			f[i] = (kappa * r[i] + 16.0f) / 116.0f;
		}
	}
	L = 116.0f * f[1] - 16.0f;
	A = 500.0f * (f[0] - f[1]);
	B = 200.0f * (f[1] - f[2]);
}

bool Yee_Compare(CompareArgs &args)
{
	if ((args.ImgA->Get_Width() != args.ImgB->Get_Width()) ||
		(args.ImgA->Get_Height() != args.ImgB->Get_Height())) {
		args.ErrorStr = "Image dimensions do not match\n";
		return false;
	}
	
	unsigned int i, dim;
	dim = args.ImgA->Get_Width() * args.ImgA->Get_Height();
	bool identical = true;
	for (i = 0; i < dim; i++) {
		if (args.ImgA->Get(i) != args.ImgB->Get(i)) {
		  identical = false;
		  break;
		}
	}
	if (identical) {
		args.ErrorStr = "Images are binary identical\n";
		return true;
	}
	
	// assuming colorspaces are in Adobe RGB (1998) convert to XYZ
	float *aX = new float[dim];
	float *aY = new float[dim];
	float *aZ = new float[dim];
	float *bX = new float[dim];
	float *bY = new float[dim];
	float *bZ = new float[dim];
	float *aLum = new float[dim];
	float *bLum = new float[dim];
	
	float *aA = new float[dim];
	float *bA = new float[dim];
	float *aB = new float[dim];
	float *bB = new float[dim];

	if (args.Verbose) printf("Converting RGB to XYZ\n");
	
	unsigned int x, y, w, h;
	w = args.ImgA->Get_Width();
	h = args.ImgA->Get_Height();
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			float r, g, b, l;
			i = x + y * w;
			r = powf(args.ImgA->Get_Red(i) / 255.0f, args.Gamma);
			g = powf(args.ImgA->Get_Green(i) / 255.0f, args.Gamma);
			b = powf(args.ImgA->Get_Blue(i) / 255.0f, args.Gamma);						
			AdobeRGBToXYZ(r,g,b,aX[i],aY[i],aZ[i]);			
			XYZToLAB(aX[i], aY[i], aZ[i], l, aA[i], aB[i]);
			r = powf(args.ImgB->Get_Red(i) / 255.0f, args.Gamma);
			g = powf(args.ImgB->Get_Green(i) / 255.0f, args.Gamma);
			b = powf(args.ImgB->Get_Blue(i) / 255.0f, args.Gamma);						
			AdobeRGBToXYZ(r,g,b,bX[i],bY[i],bZ[i]);
			XYZToLAB(bX[i], bY[i], bZ[i], l, bA[i], bB[i]);
			aLum[i] = aY[i] * args.Luminance;
			bLum[i] = bY[i] * args.Luminance;
		}
	}
	
	if (args.Verbose) printf("Constructing Laplacian Pyramids\n");
	
	LPyramid *la = new LPyramid(aLum, w, h);
	LPyramid *lb = new LPyramid(bLum, w, h);
	
	float num_one_degree_pixels = (float) (2 * tan( args.FieldOfView * 0.5 * M_PI / 180) * 180 / M_PI);
	float pixels_per_degree = w / num_one_degree_pixels;
	
	if (args.Verbose) printf("Performing test\n");
	
	float num_pixels = 1;
	unsigned int adaptation_level = 0;
	for (i = 0; i < MAX_PYR_LEVELS; i++) {
		adaptation_level = i;
		if (num_pixels > num_one_degree_pixels) break;
		num_pixels *= 2;
	}
	
	float cpd[MAX_PYR_LEVELS];
	cpd[0] = 0.5f * pixels_per_degree;
	for (i = 1; i < MAX_PYR_LEVELS; i++) cpd[i] = 0.5f * cpd[i - 1];
	float csf_max = csf(3.248f, 100.0f);
	
	float F_freq[MAX_PYR_LEVELS - 2];
	for (i = 0; i < MAX_PYR_LEVELS - 2; i++) F_freq[i] = csf_max / csf( cpd[i], 100.0f);
	
	unsigned int pixels_failed = 0;
	for (y = 0; y < h; y++) {
	  for (x = 0; x < w; x++) {
		int index = x + y * w;
		float contrast[MAX_PYR_LEVELS - 2];
		float sum_contrast = 0;
		for (i = 0; i < MAX_PYR_LEVELS - 2; i++) {
			float n1 = fabsf(la->Get_Value(x,y,i) - la->Get_Value(x,y,i + 1));
			float n2 = fabsf(lb->Get_Value(x,y,i) - lb->Get_Value(x,y,i + 1));
			float numerator = (n1 > n2) ? n1 : n2;
			float d1 = fabsf(la->Get_Value(x,y,i+2));
			float d2 = fabsf(lb->Get_Value(x,y,i+2));
			float denominator = (d1 > d2) ? d1 : d2;
			if (denominator < 1e-5f) denominator = 1e-5f;
			contrast[i] = numerator / denominator;
			sum_contrast += contrast[i];
		}
		if (sum_contrast < 1e-5) sum_contrast = 1e-5f;
		float F_mask[MAX_PYR_LEVELS - 2];
		float adapt = la->Get_Value(x,y,adaptation_level) + lb->Get_Value(x,y,adaptation_level);
		adapt *= 0.5f;
		if (adapt < 1e-5) adapt = 1e-5f;
		for (i = 0; i < MAX_PYR_LEVELS - 2; i++) {
			F_mask[i] = mask(contrast[i] * csf(cpd[i], adapt)); 
		}
		float factor = 0;
		for (i = 0; i < MAX_PYR_LEVELS - 2; i++) {
			factor += contrast[i] * F_freq[i] * F_mask[i] / sum_contrast;
		}
		if (factor < 1) factor = 1;
		if (factor > 10) factor = 10;
		float delta = fabsf(la->Get_Value(x,y,0) - lb->Get_Value(x,y,0));
		bool pass = true;
		// pure luminance test
		if (delta > factor * tvi(adapt)) {
			pass = false;
		} else if (!args.LuminanceOnly) {
			// CIE delta E test with modifications
                        float color_scale = args.ColorFactor;
			// ramp down the color test in scotopic regions
			if (adapt < 10.0f) {
                          // Don't do color test at all.
                          color_scale = 0.0;
			}
			float da = aA[index] - bA[index];
			float db = aB[index] - bB[index];
			da = da * da;
			db = db * db;
			float delta_e = (da + db) * color_scale;
			if (delta_e > factor) {
				pass = false;
			}
		}
		if (!pass) {
			pixels_failed++;
			if (args.ImgDiff) {
				args.ImgDiff->Set(255, 0, 0, 255, index);
			}
		} else {
			if (args.ImgDiff) {
				args.ImgDiff->Set(0, 0, 0, 255, index);
			}
		}
	  }
	}
	
	if (aX) delete[] aX;
	if (aY) delete[] aY;
	if (aZ) delete[] aZ;
	if (bX) delete[] bX;
	if (bY) delete[] bY;
	if (bZ) delete[] bZ;
	if (aLum) delete[] aLum;
	if (bLum) delete[] bLum;
	if (la) delete la;
	if (lb) delete lb;
	if (aA) delete aA;
	if (bA) delete bA;
	if (aB) delete aB;
	if (bB) delete bB;

	char different[100];
	sprintf(different, "%d pixels are different\n", pixels_failed);

        // Always output image difference if requested.
	if (args.ImgDiff) {
		if (args.ImgDiff->WriteToFile(args.ImgDiff->Get_Name().c_str())) {
			args.ErrorStr += "Wrote difference image to ";
			args.ErrorStr+= args.ImgDiff->Get_Name();
			args.ErrorStr += "\n";
		} else {
			args.ErrorStr += "Could not write difference image to ";
			args.ErrorStr+= args.ImgDiff->Get_Name();
			args.ErrorStr += "\n";
		}
	}

	if (pixels_failed < args.ThresholdPixels) {
		args.ErrorStr = "Images are perceptually indistinguishable\n";
                args.ErrorStr += different;
		return true;
	}
	
	args.ErrorStr = "Images are visibly different\n";
	args.ErrorStr += different;
	
	return false;
}

RGBAImage* RGBAImage::DownSample() const {
   if (Width <=1 || Height <=1) return NULL;
   int nw = Width / 2;
   int nh = Height / 2;
   RGBAImage* img = new RGBAImage(nw, nh, Name.c_str());
   for (int y = 0; y < nh; y++) {
      for (int x = 0; x < nw; x++) {
         int d[4];
         // Sample a 2x2 patch from the parent image.
         d[0] = Get(2 * x + 0, 2 * y + 0);
         d[1] = Get(2 * x + 1, 2 * y + 0);
         d[2] = Get(2 * x + 0, 2 * y + 1);
         d[3] = Get(2 * x + 1, 2 * y + 1);
         int rgba = 0;
         // Find the average color.
         for (int i = 0; i < 4; i++) {
            int c = (d[0] >> (8 * i)) & 0xFF;
            c += (d[1] >> (8 * i)) & 0xFF;
            c += (d[2] >> (8 * i)) & 0xFF;
            c += (d[3] >> (8 * i)) & 0xFF;
            c /= 4;
            rgba |= (c & 0xFF) << (8 * i);
         }
         img->Set(x, y, rgba);
      }
   }
   return img;
}


bool RGBAImage::WriteToFile(const char* filename)
{
	LodePNG::Encoder encoder;
	encoder.addText("Comment","lodepng");
	encoder.getSettings().zlibsettings.windowSize = 2048;
	

/*
	const FREE_IMAGE_FORMAT fileType = FreeImage_GetFIFFromFilename(filename);
	if(FIF_UNKNOWN == fileType)
	{
		printf("Can't save to unknown filetype %s\n", filename);
		return false;
	}

	FIBITMAP* bitmap = FreeImage_Allocate(Width, Height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000);
	if(!bitmap)
	{
		printf("Failed to create freeimage for %s\n", filename);
		return false;
	}

	const unsigned int* source = Data;
	for( int y=0; y < Height; y++, source += Width )
	{
		unsigned int* scanline = (unsigned int*)FreeImage_GetScanLine(bitmap, Height - y - 1 );
		memcpy(scanline, source, sizeof(source[0]) * Width);
	}	
	
	FreeImage_SetTransparent(bitmap, false);
	FIBITMAP* converted = FreeImage_ConvertTo24Bits(bitmap);
	
	
	const bool result = !!FreeImage_Save(fileType, converted, filename);
	if(!result)
		printf("Failed to save to %s\n", filename);
	
	FreeImage_Unload(converted);
	FreeImage_Unload(bitmap);
	return result;
*/
	return true;
}

RGBAImage* RGBAImage::ReadFromFile(const char* filename)
{
  unsigned char* buffer;
  unsigned char* image;
  size_t buffersize, imagesize, i;
  LodePNG_Decoder decoder;
  
  LodePNG_loadFile(&buffer, &buffersize, filename); /*load the image file with given filename*/
  LodePNG_Decoder_init(&decoder);
  LodePNG_Decoder_decode(&decoder, &image, &imagesize, buffer, buffersize); /*decode the png*/
  
  /*load and decode*/
  /*if there's an error, display it, otherwise display information about the image*/
  if(decoder.error) printf("error %u: %s\n", decoder.error, LodePNG_error_text(decoder.error));

  int w = decoder.infoPng.width;
  int h = decoder.infoPng.height;
  

  RGBAImage* result = new RGBAImage(w, h, filename);
  // Copy the image over to our internal format, FreeImage has the scanlines bottom to top though.
  unsigned int* dest = result->Data;
  memcpy(dest, (void *)image, h*w*4);

  /*cleanup decoder*/
  free(image);
  free(buffer);
  LodePNG_Decoder_cleanup(&decoder);

  return result;
/*
	const FREE_IMAGE_FORMAT fileType = FreeImage_GetFileType(filename);
	if(FIF_UNKNOWN == fileType)
	{
		printf("Unknown filetype %s\n", filename);
		return 0;
	}
	
	FIBITMAP* freeImage = 0;
	if(FIBITMAP* temporary = FreeImage_Load(fileType, filename, 0))
	{
		freeImage = FreeImage_ConvertTo32Bits(temporary);
		FreeImage_Unload(temporary);
	}
	if(!freeImage)
	{
		printf( "Failed to load the image %s\n", filename);
		return 0;
	}

	const int w = FreeImage_GetWidth(freeImage);
	const int h = FreeImage_GetHeight(freeImage);

	RGBAImage* result = new RGBAImage(w, h, filename);
	// Copy the image over to our internal format, FreeImage has the scanlines bottom to top though.
	unsigned int* dest = result->Data;
	for( int y=0; y < h; y++, dest += w )
	{
		const unsigned int* scanline = (const unsigned int*)FreeImage_GetScanLine(freeImage, h - y - 1 );
		memcpy(dest, scanline, sizeof(dest[0]) * w);
	}	

	FreeImage_Unload(freeImage);
	return result;
	return NULL;
*/
}


int main(int argc, char **argv)
{
	CompareArgs args;
	
	if (!args.Parse_Args(argc, argv)) {
		printf("%s", args.ErrorStr.c_str());
		return -1;
	} else {
		if (args.Verbose) args.Print_Args();
	}
	
	const bool passed = Yee_Compare(args);
	if (passed) {
		if(args.Verbose)
			printf("PASS: %s\n", args.ErrorStr.c_str());
	} else {
		printf("FAIL: %s\n", args.ErrorStr.c_str());
	}

	return passed ? 0 : 1;
}

