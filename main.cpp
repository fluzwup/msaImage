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

	if(!LoadPNG("big_objects.png", width, height, bpl, depth, dpi, &data))
		return -1;

	msaImage input;
	input.UseExternalData(width, height, bpl, depth, data);

	msaAffineTransform t;
	t.SetTransform(1.0, DegreesToRadians(angle), input.Width(), input.Height());
	msaImage rotated;
	input.TransformImage(t, rotated, 10);
	SavePNG("rotated.png", rotated);

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

	msaPixel primaries[6];
	primaries[0].r = 255;
	primaries[0].g = 0;
	primaries[0].b = 0;
	primaries[0].a = 255;

	primaries[1].r = 0;
	primaries[1].g = 255;
	primaries[1].b = 0;
	primaries[1].a = 255;

	primaries[2].r = 0;
	primaries[2].g = 0;
	primaries[2].b = 255;
	primaries[2].a = 255;

	primaries[3].r = 127;
	primaries[3].g = 127;
	primaries[3].b = 0;
	primaries[3].a = 255;

	primaries[4].r = 0;
	primaries[4].g = 127;
	primaries[4].b = 127;
	primaries[4].a = 255;

	primaries[5].r = 127;
	primaries[5].g = 0;
	primaries[5].b = 127;
	primaries[5].a = 255;

	msaImage gray;
	rotated.SimpleConvert(8, white, gray);

	std::vector<std::list<size_t>  > runs;
	msaAnalysis analyze;
	analyze.RunlengthEncodeImage(gray, runs, 254, false);

	gray.CreateImageFromRuns(runs, 8, black, white);
	SavePNG("rle_output.png", gray);

	std::list<msaObject> objects;
	analyze.GenerateObjectList(gray, 254, false, objects);

	printf("Found %zu objects\n", objects.size());


	msaImage imgObject;
	imgObject.CreateImage(gray.Width(), gray.Height(), 32, white);

	for(msaObject &o : objects)
	{
		if(o.width > 15 && o.height > 15)
			o.AddObjectToImage(imgObject, primaries[o.index % 6]);
	}
	SavePNG("found_big_objects.png", imgObject);

	imgObject.CreateImage(gray.Width(), gray.Height(), 32, white);

	for(msaObject &o : objects)
	{
		if(o.width <= 15 || o.height <= 15)
			o.AddObjectToImage(imgObject, primaries[o.index % 6]);
	}
	SavePNG("found_small_objects.png", imgObject);
	delete[] data;
	return 0;
}


