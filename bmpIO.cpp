#include <cstdlib>
#include <cstdio>
#include "bmpIO.h"

// this only handles 8, 24, and 32 bit bitmaps, and treats all 8 bit as gray
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
	int bytesPerPixel = bitdepth / 8;

	// calculate BPL, 4 byte multiples per scanline
	width = bmih.biWidth;
	height = bmih.biHeight;
	bpl = ((width * bytesPerPixel + 3) / 4) * 4;
	int bytes = height * bpl;
	unsigned char *data = new unsigned char[bytes];

	// bitmaps are stored bottom up, so read in rightside up
	for(int y = height - 1; y >= 0; --y)
	{
		unsigned char *line = data + bpl * y;
		fread(line, bpl, 1, fp);

		// color bitmaps are also BGR, convert to RGB
		switch(bytesPerPixel)
		{
		case 3:	// 24 bit
			for(int x = 0; x < width  * 3; x += 3)
			{
				unsigned char temp = line[x];
				line[x] = line[x + 2];
				line[x + 2] = temp;
			}
			break;
		case 4: // 32 bit
			// bitmaps are also BGR, convert to RGB, leave alpha
			for(int x = 0; x < width  * 4; x += 4)
			{
				unsigned char temp = line[x];
				line[x] = line[x + 2];
				line[x + 2] = temp;
			}
			break;
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

bool SaveBitmap(const char *filename, int width, int height, int bpl, unsigned char *output)
{
	int depth = (bpl / width) * 8;
	switch(depth)
	{
	case 8:
		return SaveBitmap8(filename, width, height, bpl, output);
		break;
	case 24:
		return SaveBitmap24(filename, width, height, bpl, output);
		break;
	case 32:
		return SaveBitmap32(filename, width, height, bpl, output);
		break;
	default:
		throw "Unsupported depth";
	}
}


