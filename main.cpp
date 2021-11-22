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
	double angle = std::stof(std::string(argv[1]));

	if(!LoadPNG("objects.png", width, height, bpl, depth, dpi, &data))
		return -1;

	msaImage input;
	input.UseExternalData(width, height, bpl, depth, data);

	msaAffineTransform t;
	t.SetTransform(1.0, DegreesToRadians(angle), input.Width(), input.Height());
	msaImage rotated;
	input.TransformImage(t, rotated, 10);
	SavePNG("rotated.png", rotated);

	delete[] data;
	return 0;
}


