#include "msaImage.h"
#include "msaImageLookupTables.h"
#include "ColorspaceConversion.h"

msaImage::msaImage()
{
	ownsData = false;
	width = 0;
	height = 0;
	bytesPerLine = 0;
	data = NULL;
	depth = 0;

	fill.r = 255;
	fill.g = 255;
	fill.b = 255;
	fill.a = 255;
}

msaImage::~msaImage()
{
	if(ownsData && data != NULL) delete[] data;
	data = NULL;
	ownsData = false;
	width = 0;
	height = 0;
	bytesPerLine = 0;
	depth = 0;
}

void msaImage::MoveData(msaImage &other)
{
	// delete data, if any
	if(ownsData && data != NULL) delete[] data;
	data = NULL;

	// move all the attributes of the other image over
	ownsData = other.OwnsData();
	data = other.Data();
	width = other.Width();
	depth = other.Depth();
	height = other.Height();
	bytesPerLine = other.BytesPerLine();

	// clear out the other image, but don't let it delete data
	other.ownsData = false;
	other.CreateImage(0, 0, 0);
}
bool msaImage::OwnsData()
{
	return ownsData;
}

size_t msaImage::Width()
{
	return width;
}

size_t msaImage::Height()
{
	return height;
}

size_t msaImage::Depth()
{
	return depth;
}

size_t msaImage::BytesPerLine()
{
	return bytesPerLine;
}

unsigned char *msaImage::Data()
{
	return data;
}

void msaImage::UseExternalData(size_t w, size_t h, size_t bpl, size_t d, unsigned char *pd)
{
	if(ownsData && data != NULL) delete[] data;
	data = NULL;

	width = w;
	height = h;
	depth = d;
	bytesPerLine = bpl;
	data = pd;
	ownsData = false;
}

void msaImage::TakeExternalData(size_t w, size_t h, size_t bpl, size_t d, unsigned char *pd)
{
	if(ownsData && data != NULL) delete[] data;
	data = NULL;

	width = w;
	height = h;
	depth = d;
	bytesPerLine = bpl;
	data = pd;
	ownsData = true;
}

void msaImage::SetCopyData(size_t w, size_t h, size_t bpl, size_t d, unsigned char *pd)
{
	if(ownsData && data != NULL) delete[] data;
	data = NULL;

	width = w;
	height = h;
	depth = d;
	bytesPerLine = ((w * depth / 8) + 3) / 4 * 4; // round up to 4 byte multiple
	ownsData = true;

	data = new unsigned char[height * bytesPerLine];

	// copy in line by line so we can adjust to bytesPerLine if needed
	for(size_t y = 0; y < height; ++y)
	{
		memcpy(&data[y * bytesPerLine], &pd[y * bpl], bpl);
	}
}

void msaImage::SetFill(const msaPixel &p)
{
	fill.r = p.r;
	fill.g = p.g;
	fill.b = p.b;
	fill.a = p.a;
}

void msaImage::CreateImage(size_t w, size_t h, size_t d)
{
	if(ownsData && data != NULL) delete[] data;
	data = NULL;
	bytesPerLine = 0;

	width = w;
	height = h;
	depth = d;
	ownsData = true;

	// if width or height are zero, don't allocate data, just leave the image empty
	if(w > 0 && h > 0)
	{
		bytesPerLine = ((w * depth / 8) + 3) / 4 * 4; // round up to 4 byte multiple
		data = new unsigned char[height * bytesPerLine];
	}
}

void msaImage::CreateImage(size_t w, size_t h, size_t d, const msaPixel &fill)
{
	// create uninitialized image
	CreateImage(w, h, d);

	// fill with color
	switch(d)
	{
		case 8:
			for(size_t y = 0; y < height; ++y)
			{
				memset(&data[y * bytesPerLine], fill.r, width);
			}
			break;
		case 24:
			for(size_t y = 0; y < height; ++y)
			{
				for(size_t x = 0; x < width; ++x)
				{
					data[y * bytesPerLine + x * 3] = fill.r;
					data[y * bytesPerLine + x * 3 + 1] = fill.g;
					data[y * bytesPerLine + x * 3 + 2] = fill.b;
				}
			}
			break;
		case 32:
			for(size_t y = 0; y < height; ++y)
			{
				for(size_t x = 0; x < width; ++x)
				{
					data[y * bytesPerLine + x * 4] = fill.r;
					data[y * bytesPerLine + x * 4 + 1] = fill.g;
					data[y * bytesPerLine + x * 4 + 2] = fill.b;
					data[y * bytesPerLine + x * 4 + 3] = fill.a;
				}
			}
			break;
		default:
			throw "Invalid image depth";
	}
}

void msaImage::Rotate(double radians, size_t quality, msaImage &output)
{
	msaAffineTransform t;

	t.SetFill(fill);

	// move center of image to origin, calculate rotation about that point
	t.Translate(width / -2.0, height / -2.0);
	t.Rotate(radians, 0.0, 0.0); 

	// get new size, translate center of image to center of new image
	size_t outW, outH;
	t.GetNewSize(width, height, outW, outH);
	t.Translate(outW / 2.0, outH / 2.0);

	// create the output image and transform into it
	output.CreateImage(outW, outH, depth);
	TransformImage(t, output, quality);
}

void msaImage::Rotate(double radians, size_t quality)
{
	msaImage rotated;
	Rotate(radians, quality, rotated);

	// take over the data
	MoveData(rotated);
};

void msaImage::TransformImage(msaAffineTransform &trans, msaImage &outimg, size_t quality)
{
	size_t newW = outimg.Width();
	size_t newH = outimg.Height();
	size_t newBPL = outimg.BytesPerLine();
	unsigned char *output = outimg.Data();

	// transform data into output using the appropriate depth and speed
	switch(depth)
	{
		case 8:
			if(quality < 34)
				transformFast8(trans, newW, newH, newBPL, output);
			else if(quality < 67)
				transformBetter8(trans, newW, newH, newBPL, output);
			else
				transformBest8(trans, newW, newH, newBPL, output);
			break;
		case 24:
			if(quality < 34)
				transformFast24(trans, newW, newH, newBPL, output);
			else if(quality < 67)
				transformBetter24(trans, newW, newH, newBPL, output);
			else
				transformBest24(trans, newW, newH, newBPL, output);
			break;
		case 32:
			if(quality < 34)
				transformFast32(trans, newW, newH, newBPL, output);
			else if(quality < 67)
				transformBetter32(trans, newW, newH, newBPL, output);
			else
				transformBest32(trans, newW, newH, newBPL, output);
			break;
		default:
			throw "Invalid bit depth";
	}
}


// use nearest source pixel, no interpolation
void msaImage::transformFast32(msaAffineTransform &transform, size_t newW, size_t newH, size_t newBPL, unsigned char *output)
{
	size_t x, y;

	for(y = 0; y < newH; ++y)
	{
		for(x = 0; x < newW; ++x)
		{
			double nx = x;
			double ny = y;
			transform.InvTransform(nx, ny);

			if(nx < 0 || ny < 0 || (size_t)nx >= width || (size_t)ny >= height)
			{
				output[y * newBPL + x * 4] = transform.oob_r;
				output[y * newBPL + x * 4 + 1] = transform.oob_g;
				output[y * newBPL + x * 4 + 2] = transform.oob_b;
				output[y * newBPL + x * 4 + 3] = transform.oob_a;
			}
			else
			{
				size_t index = (size_t)ny * bytesPerLine + (size_t)nx * 4;
				output[y * newBPL + x * 4] = data[index++];
				output[y * newBPL + x * 4 + 1] = data[index++];
				output[y * newBPL + x * 4 + 2] = data[index++];
				output[y * newBPL + x * 4 + 3] = data[index];
			}
		}
	}
}

void msaImage::transformFast24(msaAffineTransform &transform, size_t newW, size_t newH, size_t newBPL, unsigned char *output)
{
	size_t x, y;

	for(y = 0; y < newH; ++y)
	{
		for(x = 0; x < newW; ++x)
		{
			double nx = x;
			double ny = y;
			transform.InvTransform(nx, ny);

			if(nx < 0 || ny < 0 || (size_t)nx >= width || (size_t)ny >= height)
			{
				output[y * newBPL + x * 3] = transform.oob_r;
				output[y * newBPL + x * 3 + 1] = transform.oob_g;
				output[y * newBPL + x * 3 + 2] = transform.oob_b;
			}
			else
			{
				size_t index = (size_t)ny * bytesPerLine + (size_t)nx * 3;
				output[y * newBPL + x * 3] = data[index++];
				output[y * newBPL + x * 3 + 1] = data[index++];
				output[y * newBPL + x * 3 + 2] = data[index];
			}
		}
	}
}

void msaImage::transformFast8(msaAffineTransform &transform, size_t newW, size_t newH, size_t newBPL, unsigned char *output)
{
	size_t x, y;

	for(y = 0; y < newH; ++y)
	{
		for(x = 0; x < newW; ++x)
		{
			double nx = x;
			double ny = y;
			transform.InvTransform(nx, ny);

			if(nx < 0 || ny < 0 || (size_t)nx >= width || (size_t)ny >= height)
			{
				output[y * newBPL + x] = transform.oob_r;
			}
			else
			{
				size_t index = (size_t)ny * bytesPerLine + (size_t)nx;
				output[y * newBPL + x] = data[index++];
			}
		}
	}
}
// cosine curve for interpolation between two points on each axis
void msaImage::transformBetter32(msaAffineTransform &transform, size_t newW, size_t newH, size_t newBPL, unsigned char *output)
{
	size_t x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		size_t startX = 0;
		size_t endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 0 || ny < 0 || (size_t)nx > width - 2 || (size_t)ny > height - 2))			
				break;

			// else fill with overflow
			output[y * newBPL + x * 4] = transform.oob_r;
			output[y * newBPL + x * 4 + 1] = transform.oob_g;
			output[y * newBPL + x * 4 + 2] = transform.oob_b;
			output[y * newBPL + x * 4 + 3] = transform.oob_a;
		}

		// set buffer pointer to point to start of line
		unsigned char *outbuffer = &output[y * newBPL + x * 4];

		for(; x < endX; ++x)
		{
			nx = x;
			ny = y;
			transform.InvTransform(nx, ny);

			// if we're out of bounds, then bail out and finish filling with overflow color
			if(nx < 0 || ny < 0 || (size_t)nx > width - 2 || (size_t)ny > height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &data[wholeY * bytesPerLine + wholeX * 4];
			int r1 = ptr[0];	// grab two pixels of data
			int g1 = ptr[1];
			int b1 = ptr[2];
			int a1 = ptr[3];
			int r2 = ptr[4];
			int g2 = ptr[5];
			int b2 = ptr[6];
			int a2 = ptr[7];

			ptr += bytesPerLine;			// move down a line
			int r3 = ptr[0];	// grab two pixels of data
			int g3 = ptr[1];
			int b3 = ptr[2];
			int a3 = ptr[3];
			int r4 = ptr[4];
			int g4 = ptr[5];
			int b4 = ptr[6];
			int a4 = ptr[7];

			r1 = (r1 * fastCos[fracX] + r2 * fastCosInv[fracX]) >> 16;
			g1 = (g1 * fastCos[fracX] + g2 * fastCosInv[fracX]) >> 16;
			b1 = (b1 * fastCos[fracX] + b2 * fastCosInv[fracX]) >> 16;
			a1 = (a1 * fastCos[fracX] + a2 * fastCosInv[fracX]) >> 16;

			r3 = (r3 * fastCos[fracX] + r4 * fastCosInv[fracX]) >> 16;
			g3 = (g3 * fastCos[fracX] + g4 * fastCosInv[fracX]) >> 16;
			b3 = (b3 * fastCos[fracX] + b4 * fastCosInv[fracX]) >> 16;
			a3 = (a3 * fastCos[fracX] + a4 * fastCosInv[fracX]) >> 16;

			*(outbuffer++) = (r1 * fastCos[fracY] + r3 * fastCosInv[fracY]) >> 16;
			*(outbuffer++) = (g1 * fastCos[fracY] + g3 * fastCosInv[fracY]) >> 16;
			*(outbuffer++) = (b1 * fastCos[fracY] + b3 * fastCosInv[fracY]) >> 16;
			*(outbuffer++) = (a1 * fastCos[fracY] + a3 * fastCosInv[fracY]) >> 16;
		}

		// clear out the rest of the line
		for(; x < endX; ++x)
		{
			output[y * newBPL + x * 4] = transform.oob_r;
			output[y * newBPL + x * 4 + 1] = transform.oob_g;
			output[y * newBPL + x * 4 + 2] = transform.oob_b;
			output[y * newBPL + x * 4 + 3] = transform.oob_a;
		}
	}
}

void msaImage::transformBetter24(msaAffineTransform &transform, size_t newW, size_t newH, size_t newBPL, unsigned char *output)
{
	size_t x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		size_t startX = 0;
		size_t endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 0 || ny < 0 || (size_t)nx > width - 2 || (size_t)ny > height - 2))			
				break;

			// else fill with overflow
			output[y * newBPL + x * 3] = transform.oob_r;
			output[y * newBPL + x * 3 + 1] = transform.oob_g;
			output[y * newBPL + x * 3 + 2] = transform.oob_b;
		}

		// set buffer pointer to point to start of line
		unsigned char *outbuffer = &output[y * newBPL + x * 3];

		for(; x < endX; ++x)
		{
			nx = x;
			ny = y;
			transform.InvTransform(nx, ny);

			// if we're out of bounds, then bail out and finish filling with overflow color
			if(nx < 0 || ny < 0 || (size_t)nx > width - 2 || (size_t)ny > height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &data[wholeY * bytesPerLine + wholeX * 3];
			int r1 = ptr[0];	// grab two pixels of data
			int r2 = ptr[3];
			int g1 = ptr[1];
			int g2 = ptr[4];
			int b1 = ptr[2];
			int b2 = ptr[5];

			ptr += bytesPerLine;			// move down a line
			int r3 = ptr[0];	// grab two pixels of data
			int r4 = ptr[3];
			int g3 = ptr[1];
			int g4 = ptr[4];
			int b3 = ptr[2];
			int b4 = ptr[5];

			r1 = (r1 * fastCos[fracX] + r2 * fastCosInv[fracX]) >> 16;
			g1 = (g1 * fastCos[fracX] + g2 * fastCosInv[fracX]) >> 16;
			b1 = (b1 * fastCos[fracX] + b2 * fastCosInv[fracX]) >> 16;

			r3 = (r3 * fastCos[fracX] + r4 * fastCosInv[fracX]) >> 16;
			g3 = (g3 * fastCos[fracX] + g4 * fastCosInv[fracX]) >> 16;
			b3 = (b3 * fastCos[fracX] + b4 * fastCosInv[fracX]) >> 16;

			*(outbuffer++) = (r1 * fastCos[fracY] + r3 * fastCosInv[fracY]) >> 16;
			*(outbuffer++) = (g1 * fastCos[fracY] + g3 * fastCosInv[fracY]) >> 16;
			*(outbuffer++) = (b1 * fastCos[fracY] + b3 * fastCosInv[fracY]) >> 16;
		}

		// clear out the rest of the line
		for(; x < endX; ++x)
		{
			output[y * newBPL + x * 3] = transform.oob_r;
			output[y * newBPL + x * 3 + 1] = transform.oob_g;
			output[y * newBPL + x * 3 + 2] = transform.oob_b;
		}
	}
}

void msaImage::transformBetter8(msaAffineTransform &transform, size_t newW, size_t newH, size_t newBPL, unsigned char *output)
{
	size_t x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		size_t startX = 0;
		size_t endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 0 || ny < 0 || (size_t)nx > width - 2 || (size_t)ny > height - 2))			
				break;

			// else fill with overflow
			output[y * newBPL + x] = transform.oob_r;
		}

		// set buffer pointer to point to start of line
		unsigned char *outbuffer = &output[y * newBPL + x];

		for(; x < endX; ++x)
		{
			nx = x;
			ny = y;
			transform.InvTransform(nx, ny);

			// if we're out of bounds, then bail out and finish filling with overflow color
			if(nx < 0 || ny < 0 || (size_t)nx > width - 2 || (size_t)ny > height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &data[wholeY * bytesPerLine + wholeX];
			int r1 = ptr[0];	// grab two pixels of data
			int r2 = ptr[1];

			ptr += bytesPerLine;			// move down a line
			int r3 = ptr[0];	// grab two pixels of data
			int r4 = ptr[1];

			r1 = (r1 * fastCos[fracX] + r2 * fastCosInv[fracX]) >> 16;

			r3 = (r3 * fastCos[fracX] + r4 * fastCosInv[fracX]) >> 16;

			*(outbuffer++) = (r1 * fastCos[fracY] + r3 * fastCosInv[fracY]) >> 16;
		}

		// clear out the rest of the line
		for(; x < endX; ++x)
		{
			output[y * newBPL + x] = transform.oob_r;
		}
	}
}

// bicubic interpolation between four points along each axis
void msaImage::transformBest32(msaAffineTransform &transform, size_t newW, size_t newH, size_t newBPL, unsigned char *output)
{
	size_t x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		size_t startX = 0;
		size_t endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 1 || ny < 1 || (size_t)nx >= width - 2 || (size_t)ny >= height - 2))
				break;

			// else fill with overflow
			output[y * newBPL + x * 4] = transform.oob_r;
			output[y * newBPL + x * 4 + 1] = transform.oob_g;
			output[y * newBPL + x * 4 + 2] = transform.oob_b;
			output[y * newBPL + x * 4 + 3] = transform.oob_a;
		}

		// set buffer pointer to point to start of line
		unsigned char *outbuffer = &output[y * newBPL + x * 4];

		for(; x < endX; ++x)
		{
			nx = x;
			ny = y;
			transform.InvTransform(nx, ny);

			if(nx < 1 || ny < 1 || (size_t)nx >= width - 2 || (size_t)ny >= height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &data[(wholeY - 1) * bytesPerLine + (wholeX - 1) * 4];
			int r11 = ptr[0];	// grab 4 pixels of data
			int r12 = ptr[4];
			int r13 = ptr[8];
			int r14 = ptr[12];
			int g11 = ptr[1];
			int g12 = ptr[5];
			int g13 = ptr[9];
			int g14 = ptr[13];
			int b11 = ptr[2];
			int b12 = ptr[6];
			int b13 = ptr[10];
			int b14 = ptr[14];
			int a11 = ptr[3];
			int a12 = ptr[7];
			int a13 = ptr[11];
			int a14 = ptr[15];

			ptr += bytesPerLine;		// move down one line
			int r21 = ptr[0];	// grab 4 pixels of data
			int r22 = ptr[4];
			int r23 = ptr[8];
			int r24 = ptr[12];
			int g21 = ptr[1];
			int g22 = ptr[5];
			int g23 = ptr[9];
			int g24 = ptr[13];
			int b21 = ptr[2];
			int b22 = ptr[6];
			int b23 = ptr[10];
			int b24 = ptr[14];
			int a21 = ptr[3];
			int a22 = ptr[7];
			int a23 = ptr[11];
			int a24 = ptr[15];

			ptr += bytesPerLine;		// move down one line
			int r31 = ptr[0];	// grab 4 pixels of data
			int r32 = ptr[4];
			int r33 = ptr[8];
			int r34 = ptr[12];
			int g31 = ptr[1];
			int g32 = ptr[5];
			int g33 = ptr[9];
			int g34 = ptr[13];
			int b31 = ptr[2];
			int b32 = ptr[6];
			int b33 = ptr[10];
			int b34 = ptr[14];
			int a31 = ptr[3];
			int a32 = ptr[7];
			int a33 = ptr[11];
			int a34 = ptr[15];

			ptr += bytesPerLine;		// move down one line
			int r41 = ptr[0];	// grab 4 pixels of data
			int r42 = ptr[4];
			int r43 = ptr[8];
			int r44 = ptr[12];
			int g41 = ptr[1];
			int g42 = ptr[5];
			int g43 = ptr[9];
			int g44 = ptr[13];
			int b41 = ptr[2];
			int b42 = ptr[6];
			int b43 = ptr[10];
			int b44 = ptr[14];
			int a41 = ptr[3];
			int a42 = ptr[7];
			int a43 = ptr[11];
			int a44 = ptr[15];

			// do horizontal interpolation
			r11 = (r11 * bcint1[fracX] + r12 * bcint2[fracX] + r13 * bcint3[fracX] + r14 * bcint4[fracX]) >> 16;
			r21 = (r21 * bcint1[fracX] + r22 * bcint2[fracX] + r23 * bcint3[fracX] + r24 * bcint4[fracX]) >> 16;
			r31 = (r31 * bcint1[fracX] + r32 * bcint2[fracX] + r33 * bcint3[fracX] + r34 * bcint4[fracX]) >> 16;
			r41 = (r41 * bcint1[fracX] + r42 * bcint2[fracX] + r43 * bcint3[fracX] + r44 * bcint4[fracX]) >> 16;

			g11 = (g11 * bcint1[fracX] + g12 * bcint2[fracX] + g13 * bcint3[fracX] + g14 * bcint4[fracX]) >> 16;
			g21 = (g21 * bcint1[fracX] + g22 * bcint2[fracX] + g23 * bcint3[fracX] + g24 * bcint4[fracX]) >> 16;
			g31 = (g31 * bcint1[fracX] + g32 * bcint2[fracX] + g33 * bcint3[fracX] + g34 * bcint4[fracX]) >> 16;
			g41 = (g41 * bcint1[fracX] + g42 * bcint2[fracX] + g43 * bcint3[fracX] + g44 * bcint4[fracX]) >> 16;

			b11 = (b11 * bcint1[fracX] + b12 * bcint2[fracX] + b13 * bcint3[fracX] + b14 * bcint4[fracX]) >> 16;
			b21 = (b21 * bcint1[fracX] + b22 * bcint2[fracX] + b23 * bcint3[fracX] + b24 * bcint4[fracX]) >> 16;
			b31 = (b31 * bcint1[fracX] + b32 * bcint2[fracX] + b33 * bcint3[fracX] + b34 * bcint4[fracX]) >> 16;
			b41 = (b41 * bcint1[fracX] + b42 * bcint2[fracX] + b43 * bcint3[fracX] + b44 * bcint4[fracX]) >> 16;

			a11 = (a11 * bcint1[fracX] + a12 * bcint2[fracX] + a13 * bcint3[fracX] + a14 * bcint4[fracX]) >> 16;
			a21 = (a21 * bcint1[fracX] + a22 * bcint2[fracX] + a23 * bcint3[fracX] + a24 * bcint4[fracX]) >> 16;
			a31 = (a31 * bcint1[fracX] + a32 * bcint2[fracX] + a33 * bcint3[fracX] + a34 * bcint4[fracX]) >> 16;
			a41 = (a41 * bcint1[fracX] + a42 * bcint2[fracX] + a43 * bcint3[fracX] + a44 * bcint4[fracX]) >> 16;

			// do vertical interpolation
			r11 = (r11 * bcint1[fracY] + r21 * bcint2[fracY] + r31 * bcint3[fracY] + r41 * bcint4[fracY]) >> 16;
			g11 = (g11 * bcint1[fracY] + g21 * bcint2[fracY] + g31 * bcint3[fracY] + g41 * bcint4[fracY]) >> 16;
			b11 = (b11 * bcint1[fracY] + b21 * bcint2[fracY] + b31 * bcint3[fracY] + b41 * bcint4[fracY]) >> 16;
			a11 = (a11 * bcint1[fracY] + a21 * bcint2[fracY] + a31 * bcint3[fracY] + a41 * bcint4[fracY]) >> 16;
			
			// clip values (bicubic interpolation can overflow and underflow)
			if(r11 < 0) r11 = 0;
			if(g11 < 0) g11 = 0;
			if(b11 < 0) b11 = 0;
			if(a11 < 0) a11 = 0;

			if(r11 > 255) r11 = 255;
			if(g11 > 255) g11 = 255;
			if(b11 > 255) b11 = 255;
			if(a11 > 255) a11 = 255;

			*(outbuffer++) = r11;
			*(outbuffer++) = g11;
			*(outbuffer++) = b11;
			*(outbuffer++) = a11;
		}

		// clear out the rest of the line
		for(; x < endX; ++x)
		{
			output[y * newBPL + x * 4] = transform.oob_r;
			output[y * newBPL + x * 4 + 1] = transform.oob_g;
			output[y * newBPL + x * 4 + 2] = transform.oob_b;
			output[y * newBPL + x * 4 + 3] = transform.oob_a;
		}
	}
}


// bicubic interpolation between four points along each axis
void msaImage::transformBest24(msaAffineTransform &transform, size_t newW, size_t newH, size_t newBPL, unsigned char *output)
{
	size_t x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		size_t startX = 0;
		size_t endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 1 || ny < 1 || (size_t)nx >= width - 2 || (size_t)ny >= height - 2))
				break;

			// else fill with overflow
			output[y * newBPL + x * 3] = transform.oob_r;
			output[y * newBPL + x * 3 + 1] = transform.oob_g;
			output[y * newBPL + x * 3 + 2] = transform.oob_b;
		}

		// set buffer pointer to point to start of line
		unsigned char *outbuffer = &output[y * newBPL + x * 3];

		for(; x < endX; ++x)
		{
			nx = x;
			ny = y;
			transform.InvTransform(nx, ny);

			if(nx < 1 || ny < 1 || (size_t)nx >= width - 2 || (size_t)ny >= height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &data[(wholeY - 1) * bytesPerLine + (wholeX - 1) * 3];
			int r11 = ptr[0];	// grab 4 pixels of data
			int r12 = ptr[3];
			int r13 = ptr[6];
			int r14 = ptr[9];
			int g11 = ptr[1];
			int g12 = ptr[4];
			int g13 = ptr[7];
			int g14 = ptr[10];
			int b11 = ptr[2];
			int b12 = ptr[5];
			int b13 = ptr[8];
			int b14 = ptr[11];

			ptr += bytesPerLine;		// move down one line
			int r21 = ptr[0];	// grab 4 pixels of data
			int r22 = ptr[3];
			int r23 = ptr[6];
			int r24 = ptr[9];
			int g21 = ptr[1];
			int g22 = ptr[4];
			int g23 = ptr[7];
			int g24 = ptr[10];
			int b21 = ptr[2];
			int b22 = ptr[5];
			int b23 = ptr[8];
			int b24 = ptr[11];

			ptr += bytesPerLine;		// move down one line
			int r31 = ptr[0];	// grab 4 pixels of data
			int r32 = ptr[3];
			int r33 = ptr[6];
			int r34 = ptr[9];
			int g31 = ptr[1];
			int g32 = ptr[4];
			int g33 = ptr[7];
			int g34 = ptr[10];
			int b31 = ptr[2];
			int b32 = ptr[5];
			int b33 = ptr[8];
			int b34 = ptr[11];

			ptr += bytesPerLine;		// move down one line
			int r41 = ptr[0];	// grab 4 pixels of data
			int r42 = ptr[3];
			int r43 = ptr[6];
			int r44 = ptr[9];
			int g41 = ptr[1];
			int g42 = ptr[4];
			int g43 = ptr[7];
			int g44 = ptr[10];
			int b41 = ptr[2];
			int b42 = ptr[5];
			int b43 = ptr[8];
			int b44 = ptr[11];

			// do horizontal interpolation
			r11 = (r11 * bcint1[fracX] + r12 * bcint2[fracX] + r13 * bcint3[fracX] + r14 * bcint4[fracX]) >> 16;
			r21 = (r21 * bcint1[fracX] + r22 * bcint2[fracX] + r23 * bcint3[fracX] + r24 * bcint4[fracX]) >> 16;
			r31 = (r31 * bcint1[fracX] + r32 * bcint2[fracX] + r33 * bcint3[fracX] + r34 * bcint4[fracX]) >> 16;
			r41 = (r41 * bcint1[fracX] + r42 * bcint2[fracX] + r43 * bcint3[fracX] + r44 * bcint4[fracX]) >> 16;

			g11 = (g11 * bcint1[fracX] + g12 * bcint2[fracX] + g13 * bcint3[fracX] + g14 * bcint4[fracX]) >> 16;
			g21 = (g21 * bcint1[fracX] + g22 * bcint2[fracX] + g23 * bcint3[fracX] + g24 * bcint4[fracX]) >> 16;
			g31 = (g31 * bcint1[fracX] + g32 * bcint2[fracX] + g33 * bcint3[fracX] + g34 * bcint4[fracX]) >> 16;
			g41 = (g41 * bcint1[fracX] + g42 * bcint2[fracX] + g43 * bcint3[fracX] + g44 * bcint4[fracX]) >> 16;

			b11 = (b11 * bcint1[fracX] + b12 * bcint2[fracX] + b13 * bcint3[fracX] + b14 * bcint4[fracX]) >> 16;
			b21 = (b21 * bcint1[fracX] + b22 * bcint2[fracX] + b23 * bcint3[fracX] + b24 * bcint4[fracX]) >> 16;
			b31 = (b31 * bcint1[fracX] + b32 * bcint2[fracX] + b33 * bcint3[fracX] + b34 * bcint4[fracX]) >> 16;
			b41 = (b41 * bcint1[fracX] + b42 * bcint2[fracX] + b43 * bcint3[fracX] + b44 * bcint4[fracX]) >> 16;

			// do vertical interpolation
			r11 = (r11 * bcint1[fracY] + r21 * bcint2[fracY] + r31 * bcint3[fracY] + r41 * bcint4[fracY]) >> 16;
			g11 = (g11 * bcint1[fracY] + g21 * bcint2[fracY] + g31 * bcint3[fracY] + g41 * bcint4[fracY]) >> 16;
			b11 = (b11 * bcint1[fracY] + b21 * bcint2[fracY] + b31 * bcint3[fracY] + b41 * bcint4[fracY]) >> 16;
			

			// clip values (bicubic interpolation can overflow and underflow)
			if(r11 < 0) r11 = 0;
			if(g11 < 0) g11 = 0;
			if(b11 < 0) b11 = 0;

			if(r11 > 255) r11 = 255;
			if(g11 > 255) g11 = 255;
			if(b11 > 255) b11 = 255;
	

			*(outbuffer++) = r11;
			*(outbuffer++) = g11;
			*(outbuffer++) = b11;
		}

		// clear out the rest of the line
		for(; x < endX; ++x)
		{
			output[y * newBPL + x * 3] = transform.oob_r;
			output[y * newBPL + x * 3 + 1] = transform.oob_g;
			output[y * newBPL + x * 3 + 2] = transform.oob_b;
		}
	}
}

// bicubic interpolation between four points along each axis
void msaImage::transformBest8(msaAffineTransform &transform, size_t newW, size_t newH, size_t newBPL, unsigned char *output)
{
	size_t x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		size_t startX = 0;
		size_t endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 1 || ny < 1 || (size_t)nx >= width - 2 || (size_t)ny >= height - 2))
				break;

			// else fill with overflow
			output[y * newBPL + x] = transform.oob_r;
		}

		// set buffer pointer to point to start of line
		unsigned char *outbuffer = &output[y * newBPL + x];

		for(; x < endX; ++x)
		{
			nx = x;
			ny = y;
			transform.InvTransform(nx, ny);

			if(nx < 1 || ny < 1 || (size_t)nx >= width - 2 || (size_t)ny >= height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &data[(wholeY - 1) * bytesPerLine + (wholeX - 1)];
			int r11 = ptr[0];	// grab 4 pixels of data
			int r12 = ptr[1];
			int r13 = ptr[2];
			int r14 = ptr[3];

			ptr += bytesPerLine;		// move down one line
			int r21 = ptr[0];	// grab 4 pixels of data
			int r22 = ptr[1];
			int r23 = ptr[2];
			int r24 = ptr[3];

			ptr += bytesPerLine;		// move down one line
			int r31 = ptr[0];	// grab 4 pixels of data
			int r32 = ptr[1];
			int r33 = ptr[2];
			int r34 = ptr[3];

			ptr += bytesPerLine;		// move down one line
			int r41 = ptr[0];	// grab 4 pixels of data
			int r42 = ptr[1];
			int r43 = ptr[2];
			int r44 = ptr[3];

			// do horizontal interpolation
			r11 = (r11 * bcint1[fracX] + r12 * bcint2[fracX] + r13 * bcint3[fracX] + r14 * bcint4[fracX]) >> 16;
			r21 = (r21 * bcint1[fracX] + r22 * bcint2[fracX] + r23 * bcint3[fracX] + r24 * bcint4[fracX]) >> 16;
			r31 = (r31 * bcint1[fracX] + r32 * bcint2[fracX] + r33 * bcint3[fracX] + r34 * bcint4[fracX]) >> 16;
			r41 = (r41 * bcint1[fracX] + r42 * bcint2[fracX] + r43 * bcint3[fracX] + r44 * bcint4[fracX]) >> 16;

			// do vertical interpolation
			r11 = (r11 * bcint1[fracY] + r21 * bcint2[fracY] + r31 * bcint3[fracY] + r41 * bcint4[fracY]) >> 16;
			

			// clip values (bicubic interpolation can overflow and underflow)
			if(r11 < 0) r11 = 0;
			if(r11 > 255) r11 = 255;

			*(outbuffer++) = r11;
		}

		// clear out the rest of the line
		for(; x < endX; ++x)
		{
			output[y * newBPL + x] = transform.oob_r;
		}
	}
}

void msaImage::SimpleConvert(size_t newDepth, msaPixel &color, msaImage &outputref)
{
	msaImage output;

	// if no change in depth, just copy the image
	if(depth == newDepth)
	{
		output.SetCopyData(width, height, bytesPerLine, depth, data);
		return;
	}

	// otherwise create an uninitialized image of the correct depth
	output.CreateImage(width, height, newDepth);

	if(newDepth == 8)
	{
		if(depth == 24)
		{
			for(size_t y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(size_t x = 0; x < width; ++x)
				{
					// multiply RGB values times color values to get relative brightness
					*outLine++ = RGBtoGray(inLine[0], inLine[1], inLine[2]);
					inLine += 3;
				}
			}
		}
		else // depth == 32
		{
			for(size_t y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(size_t x = 0; x < width; ++x)
				{
					// multiply RGB values times color values to get relative brightness
					*outLine++ = RGBtoGray(inLine[0], inLine[1], inLine[2]);
					inLine += 4;	// skip alpha channel
				}
			}
		}
	}
	else if(newDepth == 24)
	{
		if(depth == 8)
		{
			for(size_t y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(size_t x = 0; x < width; ++x)
				{
					// assume color corresponds to white, scale between that and black
					size_t grey = *inLine++;
					*outLine++ = (unsigned char)(grey * color.r) / 255;
					*outLine++ = (unsigned char)(grey * color.g) / 255;
					*outLine++ = (unsigned char)(grey * color.b) / 255;
				}
			}
		}
		else // depth == 32
		{
			for(size_t y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(size_t x = 0; x < width; ++x)
				{
					// copy r, g, b, drop alpha
					*outLine++ = *inLine++;
					*outLine++ = *inLine++;
					*outLine++ = *inLine++;
					inLine++;
				}
			}
		}
	}
	else if(newDepth == 32)
	{
		if(depth == 8)
		{
			for(size_t y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(size_t x = 0; x < width; ++x)
				{
					// assume color corresponds to white, scale between that and black
					size_t grey = *inLine++;
					*outLine++ = (unsigned char)(grey * color.r) / 255;
					*outLine++ = (unsigned char)(grey * color.g) / 255;
					*outLine++ = (unsigned char)(grey * color.b) / 255;
					// copy in alpha
					*outLine++ = color.a;
				}
			}

		}
		else // depth == 24
		{
			for(size_t y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(size_t x = 0; x < width; ++x)
				{
					// copy r, g, b, add in alpha
					*outLine++ = *inLine++;
					*outLine++ = *inLine++;
					*outLine++ = *inLine++;
					*outLine++ = color.a;
				}
			}

		}
	}
	else
		throw "Invalid image depth";

	outputref.MoveData(output);
}

void msaImage::ColorMap(msaPixel map[256], msaImage &outputref)
{
	if(depth != 8)
		throw "ColorMap can only be applied to an 8 bit image.";

	msaImage output;
	output.CreateImage(width, height, 24);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *line = &output.Data()[y * output.BytesPerLine()];
		for(size_t x = 0; x < width; ++x)
		{
			msaPixel &pixel = map[data[y * bytesPerLine + x]];
			*line++ = pixel.r;
			*line++ = pixel.g;
			*line++ = pixel.b;
		}
	}
	outputref.MoveData(output);
}

void msaImage::RemapBrightness(unsigned char map[256], msaImage &outputref)
{
	if(depth != 8)
		throw "RemapBrightness can only be applied to an 8 bit image.";

	msaImage output;
	output.CreateImage(width, height, 8);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *line = &output.Data()[y * output.BytesPerLine()];
		for(size_t x = 0; x < width; ++x)
		{
			*line++ = map[data[y * bytesPerLine + x]];
		}
	}
	outputref.MoveData(output);
}

void msaImage::AddAlphaChannel(msaImage &alpha, msaImage &outputref)
{
	if(depth != 24)
		throw "Alpha channel can only be applied to a 24 bit image.";

	if(alpha.Depth() != 8)
		throw "Alpha channel must be an 8 bit image.";

	msaImage output;
	output.CreateImage(width, height, 32);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *inLine = &data[y * bytesPerLine];
		unsigned char *outLine = &output.Data()[y * output.BytesPerLine()];
		unsigned char *alphaLine = &alpha.Data()[y * alpha.BytesPerLine()];
		for(size_t x = 0; x < width; ++x)
		{
			*outLine++ = *inLine++;     // r
			*outLine++ = *inLine++;     // g
			*outLine++ = *inLine++;     // b
			*outLine++ = *alphaLine++;  // alpha
		}
	}
	outputref.MoveData(output);
}

void msaImage::ComposeRGB(msaImage &red, msaImage &green, msaImage &blue)
{
	if(red.Depth() != 8 || green.Depth() != 8 || blue.Depth() != 8)
		throw "All composite inputs must be an 8 bit images.";

	size_t width = red.Width();
	size_t height = red.Height();

	if(green.Width() != width || blue.Width() != width ||
			green.Height() != height || blue.Height() != height)
		throw "Input image dimensions must match.";

	// set up this image as output image
	CreateImage(width, height, 24);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *rLine = &red.Data()[y * red.BytesPerLine()];
		unsigned char *gLine = &green.Data()[y * blue.BytesPerLine()];
		unsigned char *bLine = &blue.Data()[y * blue.BytesPerLine()];
		unsigned char *outLine = &data[y * bytesPerLine];
		for(size_t x = 0; x < width; ++x)
		{
			*outLine++ = *rLine++;
			*outLine++ = *gLine++;
			*outLine++ = *bLine++;
		}
	}
}

void msaImage::ComposeRGBA(msaImage &red, msaImage &green, msaImage &blue, msaImage &alpha)
{
	if(red.Depth() != 8 || green.Depth() != 8 || blue.Depth() != 8 || alpha.Depth() != 8)
		throw "All composite inputs must be an 8 bit images.";

	size_t width = red.Width();
	size_t height = red.Height();

	if(green.Width() != width || blue.Width() != width || green.Height() != height || blue.Height() != height 
			|| alpha.Width() != width || alpha.Height() != height)
		throw "Input image dimensions must match.";

	// set up this image as output image
	CreateImage(width, height, 32);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *rLine = &red.Data()[y * red.BytesPerLine()];
		unsigned char *gLine = &green.Data()[y * blue.BytesPerLine()];
		unsigned char *bLine = &blue.Data()[y * blue.BytesPerLine()];
		unsigned char *aLine = &alpha.Data()[y * alpha.BytesPerLine()];
		unsigned char *outLine = &data[y * bytesPerLine];
		for(size_t x = 0; x < width; ++x)
		{
			*outLine++ = *rLine++;
			*outLine++ = *gLine++;
			*outLine++ = *bLine++;
			*outLine++ = *aLine++;
		}
	}
}


void msaImage::SplitRGB(msaImage &red, msaImage &green, msaImage &blue)
{
	if(depth != 24)
		throw "SplitRGB must be used on a 24 bit image.";

	// set up output images
	red.CreateImage(width, height, 8);
	green.CreateImage(width, height, 8);
	blue.CreateImage(width, height, 8);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *rLine = &red.Data()[y * red.BytesPerLine()];
		unsigned char *gLine = &green.Data()[y * green.BytesPerLine()];
		unsigned char *bLine = &blue.Data()[y * blue.BytesPerLine()];
		unsigned char *rgbLine = &data[y * bytesPerLine];
		for(size_t x = 0; x < width; ++x)
		{
			*rLine++ = *rgbLine++;
			*gLine++ = *rgbLine++;
			*bLine++ = *rgbLine++;
		}
	}
}

void msaImage::SplitRGBA(msaImage &red, msaImage &green, msaImage &blue, msaImage &alpha)
{
	if(depth != 24)
		throw "SplitRGBA must be used on a 32 bit image.";

	// set up output images
	red.CreateImage(width, height, 8);
	green.CreateImage(width, height, 8);
	blue.CreateImage(width, height, 8);
	alpha.CreateImage(width, height, 8);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *rLine = &red.Data()[y * red.BytesPerLine()];
		unsigned char *gLine = &green.Data()[y * green.BytesPerLine()];
		unsigned char *bLine = &blue.Data()[y * blue.BytesPerLine()];
		unsigned char *aLine = &alpha.Data()[y * alpha.BytesPerLine()];
		unsigned char *rgbaLine = &data[y * bytesPerLine];
		for(size_t x = 0; x < width; ++x)
		{
			*rLine++ = *rgbaLine++;
			*gLine++ = *rgbaLine++;
			*bLine++ = *rgbaLine++;
			*aLine++ = *rgbaLine++;
		}
	}
}

void msaImage::ComposeHSV(msaImage &hue, msaImage &sat, msaImage &vol)
{
	if(hue.Depth() != 8 || sat.Depth() != 8 || vol.Depth() != 8)
		throw "All composite inputs must be an 8 bit images.";

	size_t width = hue.Width();
	size_t height = hue.Height();

	if(sat.Width() != width || vol.Width() != width ||
			sat.Height() != height || vol.Height() != height)
		throw "Input image dimensions must match.";

	// set up this image as output image
	CreateImage(width, height, 24);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *hLine = &hue.Data()[y * hue.BytesPerLine()];
		unsigned char *sLine = &sat.Data()[y * sat.BytesPerLine()];
		unsigned char *vLine = &vol.Data()[y * vol.BytesPerLine()];
		unsigned char *rgbLine = &data[y * bytesPerLine];
		for(size_t x = 0; x < width; ++x)
		{
			// grab references to all the values we need for the conversion
			unsigned char &r = *rgbLine++;
			unsigned char &g = *rgbLine++;
			unsigned char &b = *rgbLine++;
			unsigned char &h = *hLine++;
			unsigned char &s = *sLine++;
			unsigned char &v = *vLine++;

			// do colorpsace conversion
			HSVtoRGB(h, s, v, r, g, b);
		}
	}
}

void msaImage::ComposeHSVA(msaImage &hue, msaImage &sat, msaImage &vol, msaImage &alpha)
{
	if(hue.Depth() != 8 || sat.Depth() != 8 || vol.Depth() != 8 || alpha.Depth() != 8)
		throw "All composite inputs must be an 8 bit images.";

	size_t width = hue.Width();
	size_t height = hue.Height();

	if(sat.Width() != width || vol.Width() != width || sat.Height() != height || 
			vol.Height() != height || alpha.Height() != height)
		throw "Input image dimensions must match.";

	// set up this image as output image
	CreateImage(width, height, 32);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *hLine = &hue.Data()[y * hue.BytesPerLine()];
		unsigned char *sLine = &sat.Data()[y * sat.BytesPerLine()];
		unsigned char *vLine = &vol.Data()[y * vol.BytesPerLine()];
		unsigned char *aLine = &alpha.Data()[y * alpha.BytesPerLine()];
		unsigned char *rgbaLine = &data[y * bytesPerLine];
		for(size_t x = 0; x < width; ++x)
		{
			// grab references to all the values we need for the conversion
			unsigned char &r = *rgbaLine++;
			unsigned char &g = *rgbaLine++;
			unsigned char &b = *rgbaLine++;
			unsigned char &h = *hLine++;
			unsigned char &s = *sLine++;
			unsigned char &v = *vLine++;
			// copy the alpha channel straight across
			*rgbaLine++ = *aLine++;

			// do colorpsace conversion
			HSVtoRGB(h, s, v, r, g, b);
		}
	}
}

void msaImage::SplitHSV(msaImage &hue, msaImage &sat, msaImage &vol)
{	
	if(depth != 24)
		throw "SplitHSV must be used on a 24 bit image.";

	// set up output images
	hue.CreateImage(width, height, 8);
	sat.CreateImage(width, height, 8);
	vol.CreateImage(width, height, 8);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *hLine = &hue.Data()[y * hue.BytesPerLine()];
		unsigned char *sLine = &sat.Data()[y * sat.BytesPerLine()];
		unsigned char *vLine = &vol.Data()[y * vol.BytesPerLine()];
		unsigned char *rgbLine = &data[y * bytesPerLine];
		for(size_t x = 0; x < width; ++x)
		{
			// grab references to all the values we need for the conversion
			unsigned char &r = *rgbLine++;
			unsigned char &g = *rgbLine++;
			unsigned char &b = *rgbLine++;
			unsigned char &h = *hLine++;
			unsigned char &s = *sLine++;
			unsigned char &v = *vLine++;

			// do colorpsace conversion
			RGBtoHSV(r, g, b, h, s, v);
		}
	}
}

void msaImage::SplitHSVA(msaImage &hue, msaImage &sat, msaImage &vol, msaImage &alpha)
{
	if(depth != 32)
		throw "SplitHSVA must be used on a 32 bit image.";

	// set up output images
	hue.CreateImage(width, height, 8);
	sat.CreateImage(width, height, 8);
	vol.CreateImage(width, height, 8);
	alpha.CreateImage(width, height, 8);
	
	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *hLine = &hue.Data()[y * hue.BytesPerLine()];
		unsigned char *sLine = &sat.Data()[y * sat.BytesPerLine()];
		unsigned char *vLine = &vol.Data()[y * vol.BytesPerLine()];
		unsigned char *aLine = &alpha.Data()[y * vol.BytesPerLine()];
		unsigned char *rgbaLine = &data[y * bytesPerLine];
		for(size_t x = 0; x < width; ++x)
		{
			// grab references to all the values we need for the conversion
			unsigned char &r = *rgbaLine++;
			unsigned char &g = *rgbaLine++;
			unsigned char &b = *rgbaLine++;
			unsigned char &h = *hLine++;
			unsigned char &s = *sLine++;
			unsigned char &v = *vLine++;
			// copy the alpha channel straight across
			*aLine++ = *rgbaLine++;

			// do colorpsace conversion on the rest
			RGBtoHSV(r, g, b, h, s, v);
		}
	}
}

void msaImage::MinImages(msaImage &input, msaImage &outputref)
{
	// make sure input image matches dimensions
	if(depth != input.Depth() || width != input.Width() || height != input.Height())
		throw "Input images must match in size and color depth.";

	msaImage output;
	output.CreateImage(width, height, depth);

	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(size_t x = 0; x < bytesPerLine; ++x)
		{
			unsigned char c1 = *in1++;
			unsigned char c2 = *in2++;
			*out++ = c1 > c2 ? c2 : c1;
		}
	}
	outputref.MoveData(output);
}

void msaImage::MaxImages(msaImage &input, msaImage &outputref)
{
	// make sure input image matches dimensions
	if(depth != input.Depth() || width != input.Width() || height != input.Height())
		throw "Input images must match in size and color depth.";

	msaImage output;
	output.CreateImage(width, height, depth);

	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(size_t x = 0; x < bytesPerLine; ++x)
		{
			unsigned char c1 = *in1++;
			unsigned char c2 = *in2++;
			*out++ = c1 > c2 ? c1 : c2;
		}
	}
	outputref.MoveData(output);
}

void msaImage::SumImages(msaImage &input, msaImage &outputref)
{
	// make sure input image matches dimensions
	if(depth != input.Depth() || width != input.Width() || height != input.Height())
		throw "Input images must match in size and color depth.";

	msaImage output;
	output.CreateImage(width, height, depth);

	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(size_t x = 0; x < bytesPerLine; ++x)
		{
			int sum = *in1++ + *in2++;
			*out++ = (unsigned char)sum / 2;
		}
	}
	outputref.MoveData(output);
}

void msaImage::DiffImages(msaImage &input, msaImage &outputref)
{
	// make sure input image matches dimensions
	if(depth != input.Depth() || width != input.Width() || height != input.Height())
		throw "Input images must match in size and color depth.";

	msaImage output;
	output.CreateImage(width, height, depth);

	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(size_t x = 0; x < bytesPerLine; ++x)
		{
			int diff = *in1++ - *in2++;
			*out++ = (unsigned char)(127 + diff / 2);
		}
	}
	outputref.MoveData(output);
}

void msaImage::MultiplyImages(msaImage &input, msaImage &outputref)
{
	// make sure input image matches dimensions
	if(depth != input.Depth() || width != input.Width() || height != input.Height())
		throw "Input images must match in size and color depth.";

	msaImage output;
	output.CreateImage(width, height, depth);

	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(size_t x = 0; x < bytesPerLine; ++x)
		{
			int m = *in1++ * *in2++;
			*out++ = (unsigned char)(m / 256);
		}
	}
	outputref.MoveData(output);
}

void msaImage::DivideImages(msaImage &input, msaImage &outputref)
{
	// make sure input image matches dimensions
	if(depth != input.Depth() || width != input.Width() || height != input.Height())
		throw "Input images must match in size and color depth.";

	msaImage output;
	output.CreateImage(width, height, depth);

	for(size_t y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(size_t x = 0; x < bytesPerLine; ++x)
		{
			// add one to divisor to avoid divide by zero
			int d = *in1++ * 256 / (1 + *in2++);
			*out++ = (unsigned char)d;
		}
	}
	outputref.MoveData(output);
}

void msaImage::OverlayImage(msaImage &overlay, size_t destx, size_t desty, size_t w, size_t h)
{
	if(depth != overlay.Depth())
		throw "Overlay image must match depth of base image";

	// clip width and height
	if(destx + w >= width) w = width - destx - 1;
	if(desty + h >= height) h = height - desty - 1;

	if(w <= 0 || h <= 0) 
		throw "Overlay will not fit on image";


	// for 8 and 24 bit, just copy the data from the overlay image into the destination rectangle
	if(depth == 8)
	{
		for(size_t y = 0; y < h; ++y)
			memcpy(&data[(desty + y) * bytesPerLine + destx], &overlay.Data()[y * overlay.BytesPerLine()], w); 
	}
	else if(depth == 24)
	{
		for(size_t y = 0; y < h; ++y)
			memcpy(&data[(desty + y) * bytesPerLine + destx * 3], &overlay.Data()[y * overlay.BytesPerLine()], w * 3); 
	}
	else if(depth == 32)
	{
		for(size_t y = 0; y < h; ++y)
		{
			for(size_t x = 0; x < w; ++x)
			{
				// grab data from overlay and base
				unsigned char &r1 = data[(desty + y) * bytesPerLine + (destx + x) * 4];
				unsigned char &g1 = data[(desty + y) * bytesPerLine + (destx + x) * 4 + 1];
				unsigned char &b1 = data[(desty + y) * bytesPerLine + (destx + x) * 4 + 2];

				int r2 = overlay.Data()[y * overlay.BytesPerLine() + x * 4];
				int g2 = overlay.Data()[y * overlay.BytesPerLine() + x * 4 + 1];
				int b2 = overlay.Data()[y * overlay.BytesPerLine() + x * 4 + 2];
				int alpha = overlay.Data()[y * overlay.BytesPerLine() + x * 4 + 3];

				// combine colors in ratio given by alpha channel
				r2 = ((int)r1 * (255 - alpha) + r2 * alpha) / 255;
				g2 = ((int)g1 * (255 - alpha) + g2 * alpha) / 255;
				b2 = ((int)b1 * (255 - alpha) + b2 * alpha) / 255;

				// put combined data into base
				r1 = (unsigned char)r2;
				g1 = (unsigned char)g2;
				b1 = (unsigned char)b2;
			}
		}
	}
}

void msaImage::OverlayImage(msaImage &overlay, msaImage &mask, size_t destx, size_t desty, size_t w, size_t h)
{
	if(depth != overlay.Depth())
		throw "Overlay image must match depth of base image";

	if(depth == 8)
	{
		if(mask.Depth() != 8)
			throw "Mask must be 8 bit depth";

		for(size_t y = 0; y < h; ++y)
		{
			for(size_t x = 0; x < w; ++x)
			{
				// grab data from overlay, base, and mask
				unsigned char &g1 = data[(desty + y) * bytesPerLine + (destx + x)];
				int g2 = overlay.Data()[y * overlay.BytesPerLine() + x];
				int alpha = overlay.Data()[y * overlay.BytesPerLine() + x];

				// combine colors in ratio given by alpha channel
				g2 = ((int)g1 * (255 - alpha) + g2 * alpha) / 255;

				// put combined data into base
				g1 = (unsigned char)g2;
			}
		}
	}
	else if(depth == 24)
	{
		for(size_t y = 0; y < h; ++y)
		{
			for(size_t x = 0; x < w; ++x)
			{
				// grab data from overlay and base
				unsigned char &r1 = data[(desty + y) * bytesPerLine + (destx + x) * 3];
				unsigned char &g1 = data[(desty + y) * bytesPerLine + (destx + x) * 3 + 1];
				unsigned char &b1 = data[(desty + y) * bytesPerLine + (destx + x) * 3 + 2];

				int r2 = overlay.Data()[y * overlay.BytesPerLine() + x * 3];
				int g2 = overlay.Data()[y * overlay.BytesPerLine() + x * 3 + 1];
				int b2 = overlay.Data()[y * overlay.BytesPerLine() + x * 3 + 2];

				int alpha1, alpha2, alpha3;
			       
				if(mask.Depth() == 32)
				{
					alpha1 = mask.Data()[y * mask.BytesPerLine() + x * 4];
					alpha2 = mask.Data()[y * mask.BytesPerLine() + x * 4 + 1];
					alpha3 = mask.Data()[y * mask.BytesPerLine() + x * 4 + 2];
				}
				else if(mask.Depth() == 24)
				{
					alpha1 = mask.Data()[y * mask.BytesPerLine() + x * 3];
					alpha2 = mask.Data()[y * mask.BytesPerLine() + x * 3 + 1];
					alpha3 = mask.Data()[y * mask.BytesPerLine() + x * 3 + 2];
				}
				else if(mask.Depth() == 8)
				{
					alpha1 = alpha2 = alpha3 = 
						mask.Data()[y * mask.BytesPerLine() + x];
				}

				// combine colors in ratio given by alpha channel
				r2 = ((int)r1 * (255 - alpha1) + r2 * alpha1) / 255;
				g2 = ((int)g1 * (255 - alpha2) + g2 * alpha2) / 255;
				b2 = ((int)b1 * (255 - alpha3) + b2 * alpha3) / 255;

				// put combined data into base
				r1 = (unsigned char)r2;
				g1 = (unsigned char)g2;
				b1 = (unsigned char)b2;
			}
		}
	}
	else if(depth == 32)
	{
		for(size_t y = 0; y < h; ++y)
		{
			for(size_t x = 0; x < w; ++x)
			{
				// grab data from overlay and base
				unsigned char &r1 = data[(desty + y) * bytesPerLine + (destx + x) * 4];
				unsigned char &g1 = data[(desty + y) * bytesPerLine + (destx + x) * 4 + 1];
				unsigned char &b1 = data[(desty + y) * bytesPerLine + (destx + x) * 4 + 2];

				int r2 = overlay.Data()[y * overlay.BytesPerLine() + x * 4];
				int g2 = overlay.Data()[y * overlay.BytesPerLine() + x * 4 + 1];
				int b2 = overlay.Data()[y * overlay.BytesPerLine() + x * 4 + 2];

				int alpha1, alpha2, alpha3;
			       
				if(mask.Depth() == 32)
				{
					alpha1 = mask.Data()[y * mask.BytesPerLine() + x * 4];
					alpha2 = mask.Data()[y * mask.BytesPerLine() + x * 4 + 1];
					alpha3 = mask.Data()[y * mask.BytesPerLine() + x * 4 + 2];
				}
				else if(mask.Depth() == 24)
				{
					alpha1 = mask.Data()[y * mask.BytesPerLine() + x * 3];
					alpha2 = mask.Data()[y * mask.BytesPerLine() + x * 3 + 1];
					alpha3 = mask.Data()[y * mask.BytesPerLine() + x * 3 + 2];
				}
				else if(mask.Depth() == 8)
				{
					alpha1 = alpha2 = alpha3 = 
						mask.Data()[y * mask.BytesPerLine() + x];
				}

				// combine colors in ratio given by alpha channel
				r2 = ((int)r1 * (255 - alpha1) + r2 * alpha1) / 255;
				g2 = ((int)g1 * (255 - alpha2) + g2 * alpha2) / 255;
				b2 = ((int)b1 * (255 - alpha3) + b2 * alpha3) / 255;

				// put combined data into base
				r1 = (unsigned char)r2;
				g1 = (unsigned char)g2;
				b1 = (unsigned char)b2;
			}
		}
	}
}

void msaImage::CreateImageFromRuns(std::vector< std::list <size_t> > &runs, size_t depth, msaPixel bg, msaPixel fg)
{
	// calculate width by adding up runs in first scanline
	size_t width = 0;
	for(size_t len : runs[0])
		width += len;

	// gray is fast, we can use memset
	if(depth == 8)
	{
		CreateImage(width, runs.size(), 8);
		for(size_t y = 0; y < runs.size(); ++y)
		{
			// each set of runs starts with background
			unsigned char color = bg.r;
			unsigned char *scanline = GetRawLine(y);
			size_t x = 0;
			for(size_t len : runs[y])
			{
				if(len > 0)
					memset(scanline + x, color, len);
				
				// swap colors
				if(color == bg.r) 
						color = fg.r;
				else
						color = bg.r;
				x += len;
			}
		}
	}
	else if(depth == 24 || depth == 32)
	{
		CreateImage(width, runs.size(), depth);
		for(size_t y = 0; y < runs.size(); ++y)
		{
			// each set of runs starts with background
			msaPixel color = bg;
			size_t x = 0;
			for(size_t len : runs[y])
			{
				// set each pixel
				while(len > 0)
				{
					SetPixel(x++, y, color);
					--len;
				}
				
				// swap colors
				if(color == bg) 
						color = fg;
				else
						color = bg;
				x += len;
			}
		}
	}
	else
		throw "Invalid depth";
}

