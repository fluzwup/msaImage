#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <memory.h>

#include "msaImage.h"
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

bool LoadPNG(const char *filename, int &width, int &height, int &bpl, int &depth, int &dpi, 
		unsigned char **data)
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
	int width, height, bpl, depth, dpi;
	unsigned char *data = NULL;

	if(!LoadPNG("blackbox.png", width, height, bpl, depth, dpi, &data))
		return -1;

	msaImage input;
	input.UseExternalData(width, height, bpl, depth, data);

	msaPixel p;
	p.r = 255;
	p.g = 255;
	p.b = 255;
	p.a = 255;
	msaImage gray;
	input.SimpleConvert(8, p, gray);

	SavePNG("gray.png", gray);

	msaAnalysis analyze;
	std::vector<long> vproj, hproj;
	long vmax, hmax;
	analyze.GenerateProjection(gray, true, vproj, vmax);
	analyze.GenerateProjection(input, false, hproj, hmax);

	delete[] data;

	return 0;
}
/*
int main(int argc, char **argv)
{
	int width, height, bpl, depth, dpi;
	unsigned char *data = NULL;

	if(!LoadPNG("colorscan.png", width, height, bpl, depth, dpi, &data))
		return -1;

	msaImage input;
	input.UseExternalData(width, height, bpl, depth, data);
	SavePNG("read.png", input);
	//SaveBitmap("read.bmp", input);

	msaPixel p;
	p.r = 255;
	p.g = 255;
	p.b = 255;
	p.a = 255;
	msaImage gray;
	input.SimpleConvert(8, p, gray);

	msaFilters filter;

	const int upperleft[] = 
	{
		 0,  0,  1,  0,  0,
		 0,  0,  1,  0,  0,
		 1,  1,  0, -1, -1,
		 0,  0, -1,  0,  0,
		 0,  0, -1,  0,  0,
	};
	const int lowerright[] = 
	{
		 0,  0, -1,  0,  0,
	 	 0,  0, -1,  0,  0,
		-1, -1,  0,  1,  1,
		 0,  0,  1,  0,  0,
		 0,  0,  1,  0,  0,
	};
	msaImage out1, out2;

	filter.SetUserDefined((const int *)upperleft, 5, 5, 2, 2, 0);
	filter.FilterImage(gray, out1);
	filter.SetUserDefined((const int *)lowerright, 5, 5, 2, 2, 0);
	filter.FilterImage(gray, out2);

	out1.MaxImages(out2);

	filter.SetType(msaFilters::FilterType::Dilate, 3, 3);
	filter.FilterImage(out1, out1);
	SavePNG("edges.png", out1);

	msaImage smooth;
	filter.SetType(msaFilters::FilterType::Median, 10, 10);
	filter.FilterImage(input, smooth);
	SavePNG("smooth.png", smooth);

	input.OverlayImage(smooth, out1, 0, 0, out1.Width(), out1.Height());

	SavePNG("output.png", out1);

	msaAnalysis analyze;
	std::vector<long> vproj, hproj;
	long vmax, hmax;
	analyze.GenerateProjection(out1, true, vproj, vmax);
	analyze.GenerateProjection(out1, false, hproj, hmax);

	delete[] data;

	return 0;
}
*/
