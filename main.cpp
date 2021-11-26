#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <memory.h>
#include <string>

#include "msaImage.h"
#include "msaAffine.h"
#include "ColorspaceConversion.h"
#include "msaFilters.h"
#include "msaAnalysis.h"
#include "pngIO.h"
#include "bmpIO.h"

// returns a number of microseconds
unsigned long GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return 1000000 * tv.tv_sec + tv.tv_usec;
}

bool LoadPNG(const char *filename, size_t &width, size_t &height, size_t &bpl, 
				size_t &depth, size_t &dpi, unsigned char **data)
{
	try
	{
		ReadPNG(filename, width, height, bpl, dpi, depth, data);
	}
	catch(const char *e)
	{
		printf("Exception thrown:  %s\n", e); 
		return false;
	}
	return true;
}

bool SavePNG(const char *filename, msaImage &img)
{
	try
	{
		WritePNG(filename, img.Width(), img.Height(), img.BytesPerLine(), 100, 
				img.Depth(), img.Data());
	}
	catch(const char *e)
	{
		printf("Exception thrown:  %s\n", e); 
		return false;
	}
	return true;
}

bool SaveBitmap(const char *filename, msaImage &img)
{
	return SaveBitmap(filename, img.Width(), img.Height(), img.BytesPerLine(), img.Data());
}

int main(int argc, char **argv)
{
	size_t width, height, bpl, depth, dpi;
	unsigned char *data = NULL;
	//double angle = std::stof(std::string(argv[1]));

	msaPixel white;
	white.r = 255;
	white.g = 255;
	white.b = 255;
	white.a = 255;
	
	msaPixel black;
	black.r = 0;
	black.g = 0;
	black.b = 0;
	black.a = 255;

	msaPixel spectrum[36];
	for(int i = 0; i < 36; ++i)
	{
		unsigned char r, g, b, h, s, v;
		h = i * 255 / 36;
		s = 255;
		v = 255;
		HSVtoRGB(h, s, v, r, g, b);
		spectrum[i].r = r;
		spectrum[i].g = g;
		spectrum[i].b = b;
		spectrum[i].a = 255;
	}

	if(!LoadPNG("colorscan.png", width, height, bpl, depth, dpi, &data))
		return -1;

	msaImage input;
	input.UseExternalData(width, height, bpl, depth, data);

	msaFilters filter;
	filter.SetType(msaFilters::FilterType::Gaussian, 21, 3);
	msaImage smooth;
	filter.FilterImage(input, smooth);
	filter.SetType(msaFilters::FilterType::Gaussian, 3, 21);
	filter.FilterImage(smooth, smooth);
	SavePNG("smooth.png", smooth);

	msaImage highPass;
	input.DiffImages(smooth, highPass);
	SavePNG("highpass.png", highPass);

	delete[] data;
	return 0;
}


