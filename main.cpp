#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <memory.h>

#include "msaImage.h"

#define DWORD int
#define LONG int
#define WORD short

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

// returns a number of microseconds
unsigned long GetTickCount()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return 1000000 * tv.tv_sec + tv.tv_usec;
}

bool LoadBitmap(const char *filename, int &width, int &height, int &bpl, unsigned char **pdata)
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

bool SaveBitmap(const char *filename, int width, int height, int bpl, unsigned char *output)
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

	bmfh.bfSize = sizeof(bmfh) + sizeof(bmih) + bmih.biSizeImage;

	// write out data
	FILE *fp = fopen(filename, "wb");
	if(fp == NULL)
	{
		printf("Cannot open %s for writing\n", filename);
		return false;
	}

	fwrite(&bmfh, sizeof(bmfh), 1, fp);
	fwrite(&bmih, sizeof(bmih), 1, fp);

	// flip to upside down
	for(int y = height - 1; y >= 0; --y)
	{
		unsigned char *line = output + bpl * y;

		// convert to RGB to BGR
		for(int x = 0; x < width  * 3; x += 3)
		{
			unsigned char temp = line[x];
			line[x] = line[x + 2];
			line[x + 2] = temp;
		}

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

	if(!LoadBitmap("input24.bmp", width, height, bpl, &data))
		return -1;

	msaAffineTransform affine;
	affine.SetTransform((double)1, DegreesToRadians(45.0), width, height);

	msaImage image;
	image.UseExternalData(width, height, bpl, 24, data);

	msaImage output;
	image.TransformImage(affine, output, 50);

	// write out data
	SaveBitmap("output24.bmp", output.Width(), output.Height(), output.BytesPerLine(), output.Data());

	delete[] data;

	return 0;
}

