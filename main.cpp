#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <memory.h>

#include "msaImage.h"
#include "ColorspaceConversion.h"
#include "msaFilters.h"

#define DWORD int
#define LONG int
#define WORD short
#define BYTE unsigned char

#pragma pack(1)
typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagRGBQUAD {
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
} RGBQUAD;

// returns a number of microseconds
unsigned long GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return 1000000 * tv.tv_sec + tv.tv_usec;
}

bool LoadBitmap24(const char *filename, int &width, int &height, int &bpl, unsigned char **pdata)
{
	BITMAPINFOHEADER bmih;
	BITMAPFILEHEADER bmfh;

	FILE *fp = fopen(filename, "rb");
	if(fp == NULL)
	{
		printf("Cannot open %s for reading\n", filename);
		return false;
	}

	fread(&bmfh, sizeof(bmfh), 1, fp);
	fread(&bmih, sizeof(bmih), 1, fp);

	int bitdepth = bmih.biBitCount;  // biPlanes is always 1

	if(bitdepth != 24)
	{
		printf("Input file MUST BE 24 BIT!\n\n");
		return false;
	}

	// calculate BPL, 4 byte multiples per scanline
	width = bmih.biWidth;
	height = bmih.biHeight;
	bpl = ((width * 3 + 3) / 4) * 4;
	int bytes = height * bpl;
	unsigned char *data = new unsigned char[bytes];

	// bitmaps are stored bottom up, so read in rightside up
	for(int y = height - 1; y >= 0; --y)
	{
		unsigned char *line = data + bpl * y;
		fread(line, bpl, 1, fp);

		// bitmaps are also BGR, convert to RGB
		// calculate a histogram while we go
		for(int x = 0; x < width  * 3; x += 3)
		{
			unsigned char temp = line[x];
			line[x] = line[x + 2];
			line[x + 2] = temp;
		}
	}
	fclose(fp);

	*pdata = data;

	return true;
}

bool SaveBitmap24(const char *filename, int width, int height, int bpl, unsigned char *output)
{
	BITMAPINFOHEADER bmih;
	BITMAPFILEHEADER bmfh;

	bmfh.bfType = 0x4d42;
	bmfh.bfOffBits = sizeof(bmfh) + sizeof(bmih);
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;

	// set up changed file header values
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = width;
	bmih.biHeight = height;
	bmih.biPlanes = 1;
	bmih.biBitCount = 24;
	bmih.biCompression = 0;
	bmih.biSizeImage = 0;
        bmih.biXPelsPerMeter = 3937;
        bmih.biYPelsPerMeter = 3937;
        bmih.biClrUsed = 0;
        bmih.biClrImportant = 0;

	bmfh.bfSize = sizeof(bmfh) + sizeof(bmih) + height * bpl;

	// write out data
	FILE *fp = fopen(filename, "wb");
	if(fp == NULL)
	{
		printf("Cannot open %s for writing\n", filename);
		return false;
	}

	fwrite(&bmfh, sizeof(bmfh), 1, fp);
	fwrite(&bmih, sizeof(bmih), 1, fp);

	// set up a line for BGR conversion
	int outbpl = (bmih.biBitCount * width / 8 + 3) / 4 * 4;
	unsigned char outline[outbpl];

	// flip to upside down
	for(int y = height - 1; y >= 0; --y)
	{
		unsigned char *line = output + bpl * y;

		// convert to RGB to BGR
		for(int x = 0; x < width  * 3; x += 3)
		{
			outline[x + 2] =  line[x];
			outline[x + 1] =  line[x + 1];
			outline[x] =  line[x + 2];
		}

		fwrite(outline, outbpl, 1, fp);
	}
	fclose(fp);

	return true;
}

bool SaveBitmap32(const char *filename, int width, int height, int bpl, unsigned char *output)
{
	BITMAPINFOHEADER bmih;
	BITMAPFILEHEADER bmfh;

	bmfh.bfType = 0x4d42;
	bmfh.bfOffBits = sizeof(bmfh) + sizeof(bmih);
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;

	// set up changed file header values
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = width;
	bmih.biHeight = height;
	bmih.biPlanes = 1;
	bmih.biBitCount = 32;
	bmih.biCompression = 0;
	bmih.biSizeImage = 0;
        bmih.biXPelsPerMeter = 3937;
        bmih.biYPelsPerMeter = 3937;
        bmih.biClrUsed = 0;
        bmih.biClrImportant = 0;

	bmfh.bfSize = sizeof(bmfh) + sizeof(bmih) + height * bpl;

	// write out data
	FILE *fp = fopen(filename, "wb");
	if(fp == NULL)
	{
		printf("Cannot open %s for writing\n", filename);
		return false;
	}

	fwrite(&bmfh, sizeof(bmfh), 1, fp);
	fwrite(&bmih, sizeof(bmih), 1, fp);

	// set up a line for BGR conversion
	int outbpl = bmih.biBitCount * width / 8;
	unsigned char outline[outbpl];

	// flip to upside down
	for(int y = height - 1; y >= 0; --y)
	{
		unsigned char *line = output + bpl * y;

		// convert to RGB to BGR
		for(int x = 0; x < width  * 4; x += 4)
		{
			outline[x + 2] =  line[x];
			outline[x + 1] =  line[x + 1];
			outline[x] =  line[x + 2];
			// alpha stays in the same spot
			outline[x + 3] =  line[x + 3];
		}

		fwrite(outline, outbpl, 1, fp);
	}
	fclose(fp);

	return true;
}

bool SaveBitmap8(const char *filename, int width, int height, int bpl, unsigned char *output)
{
	BITMAPINFOHEADER bmih;
	BITMAPFILEHEADER bmfh;
	RGBQUAD palette[256];

	for(int i = 0; i < 256; ++i)
	{
		palette[i].rgbBlue = (unsigned char)i;
		palette[i].rgbGreen = (unsigned char)i;
		palette[i].rgbRed = (unsigned char)i;
		palette[i].rgbReserved = 0;
	}

	bmfh.bfType = 0x4d42;
	bmfh.bfOffBits = sizeof(bmfh) + sizeof(bmih) + sizeof(palette);
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;

	// set up changed file header values
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = width;
	bmih.biHeight = height;
	bmih.biPlanes = 1;
	bmih.biBitCount = 8;
	bmih.biCompression = 0;
	bmih.biSizeImage = 0;
        bmih.biXPelsPerMeter = 3937;
        bmih.biYPelsPerMeter = 3937;
        bmih.biClrUsed = 256;
        bmih.biClrImportant = 0;

	bmfh.bfSize = sizeof(bmfh) + sizeof(bmih) + sizeof(palette) + height * bpl;

	// write out data
	FILE *fp = fopen(filename, "wb");
	if(fp == NULL)
	{
		printf("Cannot open %s for writing\n", filename);
		return false;
	}

	fwrite(&bmfh, sizeof(bmfh), 1, fp);
	fwrite(&bmih, sizeof(bmih), 1, fp);
	fwrite(&palette, sizeof(palette), 1, fp);

	// flip to upside down
	for(int y = height - 1; y >= 0; --y)
	{
		unsigned char *line = output + bpl * y;

		fwrite(line, bpl, 1, fp);

		// pad line to 4 byte multiple if needed
		if(bpl % 4 > 0)
		{
			unsigned char buffer[3] = {0};
			fwrite(buffer, bpl % 3, 1, fp);
		}
	}
	fclose(fp);

	return true;
}

int main(int argc, char **argv)
{
	int width, height, bpl;
	unsigned char *data = NULL;

	if(!LoadBitmap24("input24.bmp", width, height, bpl, &data))
		return -1;

	msaImage input, image, output;
	input.UseExternalData(width, height, bpl, 24, data);

	msaPixel p;
	p.r = 255;
	p.g = 255;
	p.b = 255;
	p.a = 255;
	

	msaFilters filter;

	filter.SetType(msaFilters::FilterType::Erode, 5, 5);
	filter.FilterImage(input, output);
	SaveBitmap24("erode24.bmp", output.Width(), output.Height(), output.BytesPerLine(), output.Data());

	msaImage alpha;
	p.r = 127;
	p.g = 127;
	p.b = 127;
	alpha.CreateImage(input.Width(), input.Height(), 8, p);
	input.AddAlphaChannel(alpha, output);

	output.SimpleConvert(32, p, image);
	image.OverlayImage(output, 50, 50, 100, 100);
	SaveBitmap32("overlay32.bmp", image.Width(), image.Height(), image.BytesPerLine(), image.Data());

	delete[] data;

	return 0;
}

