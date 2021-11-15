#include "msaImage.h"
#include "ColorspaceConversion.h"


msaImage::msaImage()
{
	ownsData = false;
	width = 0;
	height = 0;
	bytesPerLine = 0;
	data = NULL;
	depth = 0;
}

msaImage::~msaImage()
{
	if(ownsData && data != NULL) delete[] data;
	ownsData = false;
	width = 0;
	height = 0;
	bytesPerLine = 0;
	data = NULL;
	depth = 0;
}

void msaImage::MoveData(msaImage &other)
{
	// delete data, if any
	if(ownsData && data != NULL)
		delete[] data;

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

int msaImage::Width()
{
	return width;
}

int msaImage::Height()
{
	return height;
}

int msaImage::Depth()
{
	return depth;
}

int msaImage::BytesPerLine()
{
	return bytesPerLine;
}

unsigned char *msaImage::Data()
{
	return data;
}

void msaImage::UseExternalData(int w, int h, int bpl, int d, unsigned char *pd)
{
	if(ownsData) delete[] data;
	data = NULL;

	width = w;
	height = h;
	depth = d;
	bytesPerLine = bpl;
	data = pd;
	ownsData = false;
}

void msaImage::TakeExternalData(int w, int h, int bpl, int d, unsigned char *pd)
{
	if(ownsData) delete[] data;
	data = NULL;

	width = w;
	height = h;
	depth = d;
	bytesPerLine = bpl;
	data = pd;
	ownsData = true;
}

void msaImage::SetCopyData(int w, int h, int bpl, int d, unsigned char *pd)
{
	if(ownsData) delete[] data;
	data = NULL;

	width = w;
	height = h;
	depth = d;
	bytesPerLine = ((w * depth / 8) + 3) / 4 * 4; // round up to 4 byte multiple
	ownsData = true;

	data = new unsigned char[height * bytesPerLine];

	// copy in line by line so we can adjust to bytesPerLine if needed
	for(int y = 0; y < height; ++y)
	{
		memcpy(&data[y * bytesPerLine], &pd[y * bpl], bpl);
	}
}

void msaImage::CreateImage(int w, int h, int d)
{
	if(ownsData) delete[] data;
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

void msaImage::CreateImage(int w, int h, int d, const msaPixel &fill)
{
	// create uninitialized image
	CreateImage(w, h, d);

	// fill with color
	switch(d)
	{
		case 8:
			for(int y = 0; y < height; ++y)
			{
				memset(&data[y * bytesPerLine], fill.r, width);
			}
			break;
		case 24:
			for(int y = 0; y < height; ++y)
			{
				for(int x = 0; x < width; ++x)
				{
					data[y * bytesPerLine + x * 3] = fill.r;
					data[y * bytesPerLine + x * 3 + 1] = fill.g;
					data[y * bytesPerLine + x * 3 + 2] = fill.b;
				}
			}
			break;
		case 32:
			for(int y = 0; y < height; ++y)
			{
				for(int x = 0; x < width; ++x)
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

void msaImage::TransformImage(msaAffineTransform &trans, msaImage &outimg, int quality)
{
	unsigned char *output;
	int newW;
	int newH;
	int newBPL;

	// create output data
	switch(depth)
	{
		case 8:
			if(quality < 34)
				output = transformFast8(trans, newW, newH, newBPL, data);
			else if(quality < 67)
				output = transformBetter8(trans, newW, newH, newBPL, data);
			else
				output = transformBest8(trans, newW, newH, newBPL, data);
			break;
		case 24:
			if(quality < 34)
				output = transformFast24(trans, newW, newH, newBPL, data);
			else if(quality < 67)
				output = transformBetter24(trans, newW, newH, newBPL, data);
			else
				output = transformBest24(trans, newW, newH, newBPL, data);
			break;
		case 32:
			if(quality < 34)
				output = transformFast32(trans, newW, newH, newBPL, data);
			else if(quality < 67)
				output = transformBetter32(trans, newW, newH, newBPL, data);
			else
				output = transformBest32(trans, newW, newH, newBPL, data);
			break;
		default:
			throw "Invalid bit depth";
	}
	// create output image
	outimg.TakeExternalData(newW, newH, newBPL, depth, output);
}


// use nearest source pixel, no interpolation
unsigned char *msaImage::transformFast32(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input)
{
	int newH = height;
	int newW = width;
	int newBPL = newW * 4;

	// allocate space for output data
	unsigned char *output = new unsigned char[newH * newBPL];

	int x, y;

	for(y = 0; y < newH; ++y)
	{
		for(x = 0; x < newW; ++x)
		{
			double nx = x;
			double ny = y;
			transform.InvTransform(nx, ny);

			if(nx < 0 || ny < 0 || (int)nx >= width || (int)ny >= height)
			{
				output[y * newBPL + x * 4] = transform.oob_r;
				output[y * newBPL + x * 4 + 1] = transform.oob_g;
				output[y * newBPL + x * 4 + 2] = transform.oob_b;
				output[y * newBPL + x * 4 + 3] = transform.oob_a;
			}
			else
			{
				long index = (int)ny * bpl + (int)nx * 4;
				output[y * newBPL + x * 4] = input[index++];
				output[y * newBPL + x * 4 + 1] = input[index++];
				output[y * newBPL + x * 4 + 2] = input[index++];
				output[y * newBPL + x * 4 + 3] = input[index];
			}
		}
	}

	// return new values
	width = newW;
	height = newH;
	bpl = newBPL;
	return output;
}

unsigned char *msaImage::transformFast24(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input)
{
	int newH = height;
	int newW = width;
	int newBPL = (newW * 3 + 3) / 4 * 4;

	// allocate space for output data
	unsigned char *output = new unsigned char[newH * newBPL];

	int x, y;

	for(y = 0; y < newH; ++y)
	{
		for(x = 0; x < newW; ++x)
		{
			double nx = x;
			double ny = y;
			transform.InvTransform(nx, ny);

			if(nx < 0 || ny < 0 || (int)nx >= width || (int)ny >= height)
			{
				output[y * newBPL + x * 3] = transform.oob_r;
				output[y * newBPL + x * 3 + 1] = transform.oob_g;
				output[y * newBPL + x * 3 + 2] = transform.oob_b;
			}
			else
			{
				long index = (int)ny * bpl + (int)nx * 3;
				output[y * newBPL + x * 3] = input[index++];
				output[y * newBPL + x * 3 + 1] = input[index++];
				output[y * newBPL + x * 3 + 2] = input[index];
			}
		}
	}

	// return new values
	width = newW;
	height = newH;
	bpl = newBPL;
	return output;
}

unsigned char *msaImage::transformFast8(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input)
{
	int newH = height;
	int newW = width;
	int newBPL = (newW + 3) / 4 * 4;

	// allocate space for output data
	unsigned char *output = new unsigned char[newH * newBPL];

	int x, y;

	for(y = 0; y < newH; ++y)
	{
		for(x = 0; x < newW; ++x)
		{
			double nx = x;
			double ny = y;
			transform.InvTransform(nx, ny);

			if(nx < 0 || ny < 0 || (int)nx >= width || (int)ny >= height)
			{
				output[y * newBPL + x] = transform.oob_r;
			}
			else
			{
				long index = (int)ny * bpl + (int)nx;
				output[y * newBPL + x] = input[index++];
			}
		}
	}

	// return new values
	width = newW;
	height = newH;
	bpl = newBPL;
	return output;
}

// this returns a cosine curve, scaled for 64k, for the input range of [0 .. 255] for use in cosine interpolation
const int fastCos[] = 
{
	65535, 65533, 65525, 65513, 65496, 65473, 65446, 65414, 65377, 65335, 65289, 65237, 65180,
	65119, 65053, 64981, 64905, 64825, 64739, 64648, 64553, 64453, 64348, 64238, 64124, 64005,
	63881, 63753, 63620, 63482, 63339, 63192, 63041, 62885, 62724, 62559, 62389, 62215, 62036,
	61853, 61666, 61474, 61278, 61078, 60873, 60664, 60451, 60234, 60013, 59787, 59558, 59324,
	59087, 58845, 58600, 58351, 58097, 57840, 57580, 57315, 57047, 56775, 56500, 56220, 55938,
	55652, 55362, 55069, 54773, 54474, 54171, 53865, 53555, 53243, 52927, 52609, 52287, 51963,
	51636, 51306, 50973, 50637, 50298, 49957, 49614, 49268, 48919, 48568, 48214, 47859, 47501,
	47140, 46778, 46413, 46047, 45678, 45308, 44935, 44561, 44185, 43807, 43428, 43047, 42664,
	42280, 41895, 41508, 41119, 40730, 40339, 39948, 39555, 39161, 38766, 38370, 37974, 37576,
	37178, 36779, 36380, 35980, 35580, 35179, 34778, 34376, 33974, 33572, 33170, 32768, 32366,
	31964, 31562, 31160, 30759, 30358, 29957, 29557, 29157, 28757, 28358, 27960, 27563, 27166,
	26771, 26376, 25982, 25589, 25197, 24806, 24417, 24029, 23642, 23256, 22872, 22490, 22109,
	21729, 21351, 20975, 20601, 20229, 19858, 19490, 19123, 18758, 18396, 18036, 17678, 17322,
	16968, 16617, 16269, 15922, 15579, 15238, 14899, 14564, 14231, 13900, 13573, 13249, 12927,
	12609, 12293, 11981, 11671, 11365, 11062, 10763, 10466, 10173, 9884, 9598, 9315, 9036,
	8761, 8489, 8221, 7956, 7695, 7438, 7185, 6936, 6690, 6449, 6211, 5978, 5748,
	5523, 5301, 5084, 4871, 4662, 4457, 4257, 4061, 3869, 3682, 3499, 3320, 3146,
	2976, 2811, 2650, 2494, 2343, 2195, 2053, 1915, 1782, 1654, 1530, 1411, 1296,
	1187, 1082, 982, 886, 796, 710, 629, 553, 482, 415, 354, 297, 246,
	199, 157, 120, 88, 61, 39, 21, 9, 1 
};

// inverse of above
const int fastCosInv[] = 
{
	0, 2, 10, 22, 39, 62, 89, 121, 158, 200, 246, 298, 355,
	416, 482, 554, 630, 710, 796, 887, 982, 1082, 1187, 1297, 1411, 1530,
	1654, 1782, 1915, 2053, 2196, 2343, 2494, 2650, 2811, 2976, 3146, 3320, 3499,
	3682, 3869, 4061, 4257, 4457, 4662, 4871, 5084, 5301, 5522, 5748, 5977, 6211,
	6448, 6690, 6935, 7184, 7438, 7695, 7955, 8220, 8488, 8760, 9035, 9315, 9597,
	9883, 10173, 10466, 10762, 11061, 11364, 11670, 11980, 12292, 12608, 12926, 13248, 13572,
	13899, 14229, 14562, 14898, 15237, 15578, 15921, 16267, 16616, 16967, 17321, 17676, 18034,
	18395, 18757, 19122, 19488, 19857, 20227, 20600, 20974, 21350, 21728, 22107, 22488, 22871,
	23255, 23640, 24027, 24416, 24805, 25196, 25587, 25980, 26374, 26769, 27165, 27561, 27959,
	28357, 28756, 29155, 29555, 29955, 30356, 30757, 31159, 31561, 31963, 32365, 32767, 33169,
	33571, 33973, 34375, 34776, 35177, 35578, 35978, 36378, 36778, 37177, 37575, 37972, 38369,
	38764, 39159, 39553, 39946, 40338, 40729, 41118, 41506, 41893, 42279, 42663, 43045, 43426,
	43806, 44184, 44560, 44934, 45306, 45677, 46045, 46412, 46777, 47139, 47499, 47857, 48213,
	48567, 48918, 49266, 49613, 49956, 50297, 50636, 50971, 51304, 51635, 51962, 52286, 52608,
	52926, 53242, 53554, 53864, 54170, 54473, 54772, 55069, 55362, 55651, 55937, 56220, 56499,
	56774, 57046, 57314, 57579, 57840, 58097, 58350, 58599, 58845, 59086, 59324, 59557, 59787,
	60012, 60234, 60451, 60664, 60873, 61078, 61278, 61474, 61666, 61853, 62036, 62215, 62389,
	62559, 62724, 62885, 63041, 63192, 63340, 63482, 63620, 63753, 63881, 64005, 64124, 64239,
	64348, 64453, 64553, 64649, 64739, 64825, 64906, 64982, 65053, 65120, 65181, 65238, 65289,
	65336, 65378, 65415, 65447, 65474, 65496, 65514, 65526, 65534
};

// cosine curve for interpolation between two points on each axis
unsigned char *msaImage::transformBetter32(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input)
{
	int newH = height;
	int newW = width;
	int newBPL = newW * 4;

	// allocate space for output data
	unsigned char *output = new unsigned char[newH * newBPL];

	int x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		int startX = 0;
		int endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 0 || ny < 0 || (int)nx > width - 2 || (int)ny > height - 2))			
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
			if(nx < 0 || ny < 0 || (int)nx > width - 2 || (int)ny > height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &input[wholeY * bpl + wholeX * 4];
			int r1 = ptr[0];	// grab two pixels of data
			int g1 = ptr[1];
			int b1 = ptr[2];
			int a1 = ptr[3];
			int r2 = ptr[4];
			int g2 = ptr[5];
			int b2 = ptr[6];
			int a2 = ptr[7];

			ptr += bpl;			// move down a line
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

	// return new values
	width = newW;
	height = newH;
	bpl = newBPL;
	return output;
}

unsigned char *msaImage::transformBetter24(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input)
{
	int newH = height;
	int newW = width;
	int newBPL = (newW * 3 + 3) / 4 * 4;

	// allocate space for output data
	unsigned char *output = new unsigned char[newH * newBPL];

	int x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		int startX = 0;
		int endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 0 || ny < 0 || (int)nx > width - 2 || (int)ny > height - 2))			
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
			if(nx < 0 || ny < 0 || (int)nx > width - 2 || (int)ny > height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &input[wholeY * bpl + wholeX * 3];
			int r1 = ptr[0];	// grab two pixels of data
			int r2 = ptr[3];
			int g1 = ptr[1];
			int g2 = ptr[4];
			int b1 = ptr[2];
			int b2 = ptr[5];

			ptr += bpl;			// move down a line
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

	// return new values
	width = newW;
	height = newH;
	bpl = newBPL;
	return output;
}

unsigned char *msaImage::transformBetter8(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input)
{
	int newH = height;
	int newW = width;
	int newBPL = (newW + 3) / 4 * 4;

	// allocate space for output data
	unsigned char *output = new unsigned char[newH * newBPL];

	int x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		int startX = 0;
		int endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 0 || ny < 0 || (int)nx > width - 2 || (int)ny > height - 2))			
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
			if(nx < 0 || ny < 0 || (int)nx > width - 2 || (int)ny > height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &input[wholeY * bpl + wholeX];
			int r1 = ptr[0];	// grab two pixels of data
			int r2 = ptr[1];

			ptr += bpl;			// move down a line
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

	// return new values
	width = newW;
	height = newH;
	bpl = newBPL;
	return output;
}

// lookup tables for bicubic interpolation, 64k based output, [0..255] input
int bcint1[] =
{
	0, -85, -169, -252, -333, -414, -494, -573, -651, -728, -804, -879, -953, -1026, -1098, -1170, 
	-1240, -1309, -1378, -1445, -1512, -1578, -1642, -1706, -1769, -1831, -1892, -1952, -2012, -2070, -2128, -2184, 
	-2240, -2295, -2349, -2402, -2454, -2506, -2556, -2606, -2655, -2703, -2750, -2797, -2842, -2887, -2931, -2974, 
	-3016, -3057, -3098, -3138, -3177, -3215, -3252, -3289, -3325, -3360, -3394, -3428, -3461, -3493, -3524, -3554, 
	-3584, -3613, -3641, -3669, -3695, -3721, -3747, -3771, -3795, -3818, -3840, -3862, -3883, -3903, -3923, -3942, 
	-3960, -3977, -3994, -4010, -4026, -4041, -4055, -4068, -4081, -4093, -4105, -4115, -4126, -4135, -4144, -4152, 
	-4160, -4167, -4173, -4179, -4184, -4189, -4193, -4196, -4199, -4201, -4203, -4204, -4204, -4204, -4203, -4202, 
	-4200, -4197, -4194, -4191, -4187, -4182, -4177, -4171, -4165, -4158, -4151, -4143, -4135, -4126, -4116, -4106, 
	-4096, -4085, -4074, -4062, -4049, -4036, -4023, -4009, -3995, -3980, -3965, -3949, -3933, -3916, -3899, -3882, 
	-3864, -3846, -3827, -3807, -3788, -3768, -3747, -3726, -3705, -3683, -3661, -3639, -3616, -3592, -3569, -3544, 
	-3520, -3495, -3470, -3444, -3418, -3392, -3365, -3338, -3311, -3283, -3255, -3227, -3198, -3169, -3140, -3110, 
	-3080, -3050, -3019, -2988, -2957, -2925, -2893, -2861, -2829, -2796, -2763, -2730, -2697, -2663, -2629, -2595, 
	-2560, -2525, -2490, -2455, -2419, -2384, -2348, -2311, -2275, -2238, -2201, -2164, -2127, -2090, -2052, -2014, 
	-1976, -1938, -1899, -1861, -1822, -1783, -1744, -1704, -1665, -1625, -1586, -1546, -1506, -1465, -1425, -1385, 
	-1344, -1303, -1262, -1221, -1180, -1139, -1098, -1056, -1015, -973, -932, -890, -848, -806, -764, -722, 
	-680, -638, -596, -553, -511, -468, -426, -384, -341, -298, -256, -213, -171, -128, -85, -43, 
};
int bcint2[] =
{
	65535, 65406, 65275, 65142, 65007, 64870, 64731, 64591, 64448, 64303, 64157, 64009, 63858, 63706, 63552, 63397, 
	63239, 63080, 62918, 62755, 62591, 62424, 62256, 62086, 61914, 61741, 61565, 61389, 61210, 61030, 60848, 60664, 
	60479, 60292, 60104, 59914, 59722, 59529, 59334, 59138, 58940, 58741, 58540, 58337, 58133, 57928, 57721, 57513, 
	57303, 57092, 56879, 56665, 56450, 56233, 56015, 55795, 55574, 55352, 55128, 54903, 54677, 54449, 54221, 53991, 
	53759, 53527, 53293, 53058, 52821, 52584, 52345, 52105, 51864, 51622, 51379, 51134, 50889, 50642, 50394, 50145, 
	49895, 49644, 49392, 49139, 48885, 48630, 48374, 48116, 47858, 47599, 47339, 47078, 46816, 46553, 46290, 46025, 
	45759, 45493, 45226, 44957, 44688, 44419, 44148, 43877, 43604, 43331, 43058, 42783, 42508, 42232, 41955, 41678, 
	41399, 41121, 40841, 40561, 40280, 39999, 39716, 39434, 39150, 38866, 38582, 38297, 38011, 37725, 37438, 37151, 
	36863, 36575, 36286, 35997, 35708, 35417, 35127, 34836, 34544, 34253, 33960, 33668, 33375, 33082, 32788, 32494, 
	32200, 31905, 31610, 31315, 31019, 30723, 30427, 30131, 29835, 29538, 29241, 28944, 28646, 28349, 28051, 27754, 
	27456, 27158, 26859, 26561, 26263, 25964, 25666, 25367, 25069, 24770, 24471, 24173, 23874, 23575, 23277, 22978, 
	22680, 22381, 22083, 21785, 21486, 21188, 20890, 20592, 20295, 19997, 19700, 19403, 19106, 18809, 18512, 18216, 
	17920, 17624, 17328, 17033, 16738, 16443, 16149, 15855, 15561, 15267, 14974, 14682, 14389, 14097, 13806, 13515, 
	13224, 12934, 12644, 12354, 12065, 11777, 11489, 11202, 10915, 10628, 10343, 10057, 9773, 9489, 9205, 8922, 
	8640, 8358, 8077, 7797, 7517, 7238, 6960, 6682, 6405, 6129, 5853, 5578, 5304, 5031, 4759, 4487, 
	4216, 3946, 3677, 3408, 3141, 2874, 2608, 2343, 2079, 1816, 1554, 1292, 1032, 772, 514, 256, 
};
int bcint3[] =
{
	0, 256, 514, 772, 1032, 1292, 1554, 1816, 2079, 2343, 2608, 2874, 3141, 3408, 3677, 3946, 
	4216, 4487, 4759, 5031, 5304, 5578, 5853, 6129, 6405, 6682, 6960, 7238, 7517, 7797, 8077, 8358, 
	8640, 8922, 9205, 9489, 9773, 10057, 10343, 10628, 10915, 11202, 11489, 11777, 12065, 12354, 12644, 12934, 
	13224, 13515, 13806, 14097, 14389, 14682, 14974, 15267, 15561, 15855, 16149, 16443, 16738, 17033, 17328, 17624, 
	17920, 18216, 18512, 18809, 19106, 19403, 19700, 19997, 20295, 20592, 20890, 21188, 21486, 21785, 22083, 22381, 
	22680, 22978, 23277, 23575, 23874, 24173, 24471, 24770, 25069, 25367, 25666, 25964, 26263, 26561, 26859, 27158, 
	27456, 27754, 28051, 28349, 28646, 28944, 29241, 29538, 29835, 30131, 30427, 30723, 31019, 31315, 31610, 31905, 
	32200, 32494, 32788, 33082, 33375, 33668, 33960, 34253, 34544, 34836, 35127, 35417, 35708, 35997, 36286, 36575, 
	36863, 37151, 37438, 37725, 38011, 38297, 38582, 38866, 39150, 39434, 39716, 39999, 40280, 40561, 40841, 41121, 
	41399, 41678, 41955, 42232, 42508, 42783, 43058, 43331, 43604, 43877, 44148, 44419, 44688, 44957, 45226, 45493, 
	45759, 46025, 46290, 46553, 46816, 47078, 47339, 47599, 47858, 48116, 48374, 48630, 48885, 49139, 49392, 49644, 
	49895, 50145, 50394, 50642, 50889, 51134, 51379, 51622, 51864, 52105, 52345, 52584, 52821, 53058, 53293, 53527, 
	53759, 53991, 54221, 54449, 54677, 54903, 55128, 55352, 55574, 55795, 56015, 56233, 56450, 56665, 56879, 57092, 
	57303, 57513, 57721, 57928, 58133, 58337, 58540, 58741, 58940, 59138, 59334, 59529, 59722, 59914, 60104, 60292, 
	60479, 60664, 60848, 61030, 61210, 61389, 61565, 61741, 61914, 62086, 62256, 62424, 62591, 62755, 62918, 63080, 
	63239, 63397, 63552, 63706, 63858, 64009, 64157, 64303, 64448, 64591, 64731, 64870, 65007, 65142, 65275, 65406, 
};
int bcint4[] =
{
	0, -43, -85, -128, -171, -213, -256, -298, -341, -384, -426, -468, -511, -553, -596, -638, 
	-680, -722, -764, -806, -848, -890, -932, -973, -1015, -1056, -1098, -1139, -1180, -1221, -1262, -1303, 
	-1344, -1385, -1425, -1465, -1506, -1546, -1586, -1625, -1665, -1704, -1744, -1783, -1822, -1861, -1899, -1938, 
	-1976, -2014, -2052, -2090, -2127, -2164, -2201, -2238, -2275, -2311, -2348, -2384, -2419, -2455, -2490, -2525, 
	-2560, -2595, -2629, -2663, -2697, -2730, -2763, -2796, -2829, -2861, -2893, -2925, -2957, -2988, -3019, -3050, 
	-3080, -3110, -3140, -3169, -3198, -3227, -3255, -3283, -3311, -3338, -3365, -3392, -3418, -3444, -3470, -3495, 
	-3520, -3544, -3569, -3592, -3616, -3639, -3661, -3683, -3705, -3726, -3747, -3768, -3788, -3807, -3827, -3846, 
	-3864, -3882, -3899, -3916, -3933, -3949, -3965, -3980, -3995, -4009, -4023, -4036, -4049, -4062, -4074, -4085, 
	-4096, -4106, -4116, -4126, -4135, -4143, -4151, -4158, -4165, -4171, -4177, -4182, -4187, -4191, -4194, -4197, 
	-4200, -4202, -4203, -4204, -4204, -4204, -4203, -4201, -4199, -4196, -4193, -4189, -4184, -4179, -4173, -4167, 
	-4160, -4152, -4144, -4135, -4126, -4115, -4105, -4093, -4081, -4068, -4055, -4041, -4026, -4010, -3994, -3977, 
	-3960, -3942, -3923, -3903, -3883, -3862, -3840, -3818, -3795, -3771, -3747, -3721, -3695, -3669, -3641, -3613, 
	-3584, -3554, -3524, -3493, -3461, -3428, -3394, -3360, -3325, -3289, -3252, -3215, -3177, -3138, -3098, -3057, 
	-3016, -2974, -2931, -2887, -2842, -2797, -2750, -2703, -2655, -2606, -2556, -2506, -2454, -2402, -2349, -2295, 
	-2240, -2184, -2128, -2070, -2012, -1952, -1892, -1831, -1769, -1706, -1642, -1578, -1512, -1445, -1378, -1309, 
	-1240, -1170, -1098, -1026, -953, -879, -804, -728, -651, -573, -494, -414, -333, -252, -169, -85, 
};

// bicubic interpolation between four points along each axis
unsigned char *msaImage::transformBest32(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input)
{
	int newH = height;
	int newW = width;
	int newBPL = newW * 4;

	// allocate space for output data
	unsigned char *output = new unsigned char[newH * newBPL];

	int x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		int startX = 0;
		int endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 1 || ny < 1 || (int)nx >= width - 2 || (int)ny >= height - 2))
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

			if(nx < 1 || ny < 1 || (int)nx >= width - 2 || (int)ny >= height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &input[(wholeY - 1) * bpl + (wholeX - 1) * 4];
			int r11 = *ptr++; 	// grab 4 pixels of data
			int g11 = *ptr++; 
			int b11 = *ptr++; 
			int a11 = *ptr++;

			int r12 = *ptr++; 
			int g12 = *ptr++; 
			int b12 = *ptr++; 
			int a12 = *ptr++;

			int r13 = *ptr++; 
			int g13 = *ptr++; 
			int b13 = *ptr++; 
			int a13 = *ptr++;

			int r14 = *ptr++; 
			int g14 = *ptr++; 
			int b14 = *ptr++; 
			int a14 = *ptr++; 

			ptr += bpl;		// move down one line
			int r21 = *ptr++; 	// grab 4 pixels of data
			int g21 = *ptr++;
			int b21 = *ptr++;
			int a21 = *ptr++;

			int r22 = *ptr++;
			int g22 = *ptr++;
			int b22 = *ptr++;
			int a22 = *ptr++;

			int r23 = *ptr++;
			int g23 = *ptr++;
			int b23 = *ptr++;
			int a23 = *ptr++;

			int r24 = *ptr++;
			int g24 = *ptr++;
			int b24 = *ptr++;
			int a24 = *ptr++;

			ptr += bpl;		// move down one line
			int r31 = *ptr++; 	// grab 4 pixels of data
			int g31 = *ptr++;
			int b31 = *ptr++;
			int a31 = *ptr++;

			int r32 = *ptr++;
			int g32 = *ptr++;
			int b32 = *ptr++;
			int a32 = *ptr++;

			int r33 = *ptr++;
			int g33 = *ptr++;
			int b33 = *ptr++;
			int a33 = *ptr++;

			int r34 = *ptr++;
			int g34 = *ptr++;
			int b34 = *ptr++;
			int a34 = *ptr++;

			ptr += bpl;		// move down one line
			int r41 = *ptr++; 	// grab 4 pixels of data
			int g41 = *ptr++;
			int b41 = *ptr++;
			int a41 = *ptr++;

			int r42 = *ptr++;
			int g42 = *ptr++;
			int b42 = *ptr++;
			int a42 = *ptr++;

			int r43 = *ptr++;
			int g43 = *ptr++;
			int b43 = *ptr++;
			int a43 = *ptr++;

			int r44 = *ptr++;
			int g44 = *ptr++;
			int b44 = *ptr++;
			int a44 = *ptr++;

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

	// return new values
	width = newW;
	height = newH;
	bpl = newBPL;
	return output;
}

// bicubic interpolation between four points along each axis
unsigned char *msaImage::transformBest24(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input)
{
	int newH = height;
	int newW = width;
	int newBPL = (newW * 3 + 3) / 4 * 4;

	// allocate space for output data
	unsigned char *output = new unsigned char[newH * newBPL];

	int x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		int startX = 0;
		int endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 1 || ny < 1 || (int)nx >= width - 2 || (int)ny >= height - 2))
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

			if(nx < 1 || ny < 1 || (int)nx >= width - 2 || (int)ny >= height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &input[(wholeY - 1) * bpl + (wholeX - 1) * 3];
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

			ptr += bpl;		// move down one line
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

			ptr += bpl;		// move down one line
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

			ptr += bpl;		// move down one line
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

	// return new values
	width = newW;
	height = newH;
	bpl = newBPL;
	return output;
}

// bicubic interpolation between four points along each axis
unsigned char *msaImage::transformBest8(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input)
{
	int newH = height;
	int newW = width;
	int newBPL = (newW + 3) / 4 * 4;

	// allocate space for output data
	unsigned char *output = new unsigned char[newH * newBPL];

	int x, y;
	double nx;
	double ny;

	for(y = 0; y < newH; ++y)
	{
		int startX = 0;
		int endX = newW;

		// start out and zip along until we find a bit that transforms into a valid location
		//  on the input image
		for(x = startX; x < endX; ++x)
		{
			ny = y;
			nx = x;

			transform.InvTransform(nx, ny);

			if(!(nx < 1 || ny < 1 || (int)nx >= width - 2 || (int)ny >= height - 2))
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

			if(nx < 1 || ny < 1 || (int)nx >= width - 2 || (int)ny >= height - 2)
				break;

			int wholeX = (int)nx;
			int fracX = (int)(256.0 * nx) - 256 * wholeX;
			int wholeY = (int)ny;
			int fracY = (int)(256.0 * ny) - 256 * wholeY;

			unsigned char *ptr = &input[(wholeY - 1) * bpl + (wholeX - 1)];
			int r11 = ptr[0];	// grab 4 pixels of data
			int r12 = ptr[1];
			int r13 = ptr[2];
			int r14 = ptr[3];

			ptr += bpl;		// move down one line
			int r21 = ptr[0];	// grab 4 pixels of data
			int r22 = ptr[1];
			int r23 = ptr[2];
			int r24 = ptr[3];

			ptr += bpl;		// move down one line
			int r31 = ptr[0];	// grab 4 pixels of data
			int r32 = ptr[1];
			int r33 = ptr[2];
			int r34 = ptr[3];

			ptr += bpl;		// move down one line
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

	// return new values
	width = newW;
	height = newH;
	bpl = newBPL;
	return output;
}

void msaImage::SimpleConvert(int newDepth, msaPixel &color, msaImage &outputref)
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
			for(int y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(int x = 0; x < width; ++x)
				{
					// multiply RGB values times color values to get relative brightness
					*outLine++ = RGBtoGray(inLine[0], inLine[1], inLine[2]);
					inLine += 3;
				}
			}
		}
		else // depth == 32
		{
			for(int y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(int x = 0; x < width; ++x)
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
			for(int y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(int x = 0; x < width; ++x)
				{
					// assume color corresponds to white, scale between that and black
					int grey = *inLine++;
					*outLine++ = (unsigned char)(grey * color.r) / 255;
					*outLine++ = (unsigned char)(grey * color.g) / 255;
					*outLine++ = (unsigned char)(grey * color.b) / 255;
				}
			}
		}
		else // depth == 32
		{
			for(int y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(int x = 0; x < width; ++x)
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
			for(int y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(int x = 0; x < width; ++x)
				{
					// assume color corresponds to white, scale between that and black
					int grey = *inLine++;
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
			for(int y = 0; y < height; ++y)
			{
				unsigned char *outLine = &(output.Data())[y * output.BytesPerLine()];
				unsigned char *inLine = &data[y * bytesPerLine];
				for(int x = 0; x < width; ++x)
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
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *line = &output.Data()[y * output.BytesPerLine()];
		for(int x = 0; x < width; ++x)
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
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *line = &output.Data()[y * output.BytesPerLine()];
		for(int x = 0; x < width; ++x)
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
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *inLine = &data[y * bytesPerLine];
		unsigned char *outLine = &output.Data()[y * output.BytesPerLine()];
		unsigned char *alphaLine = &alpha.Data()[y * alpha.BytesPerLine()];
		for(int x = 0; x < width; ++x)
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

	int width = red.Width();
	int height = red.Height();

	if(green.Width() != width || blue.Width() != width ||
			green.Height() != height || blue.Height() != height)
		throw "Input image dimensions must match.";

	// set up this image as output image
	CreateImage(width, height, 24);
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *rLine = &red.Data()[y * red.BytesPerLine()];
		unsigned char *gLine = &green.Data()[y * blue.BytesPerLine()];
		unsigned char *bLine = &blue.Data()[y * blue.BytesPerLine()];
		unsigned char *outLine = &data[y * bytesPerLine];
		for(int x = 0; x < width; ++x)
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

	int width = red.Width();
	int height = red.Height();

	if(green.Width() != width || blue.Width() != width || green.Height() != height || blue.Height() != height 
			|| alpha.Width() != width || alpha.Height() != height)
		throw "Input image dimensions must match.";

	// set up this image as output image
	CreateImage(width, height, 32);
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *rLine = &red.Data()[y * red.BytesPerLine()];
		unsigned char *gLine = &green.Data()[y * blue.BytesPerLine()];
		unsigned char *bLine = &blue.Data()[y * blue.BytesPerLine()];
		unsigned char *aLine = &alpha.Data()[y * alpha.BytesPerLine()];
		unsigned char *outLine = &data[y * bytesPerLine];
		for(int x = 0; x < width; ++x)
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
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *rLine = &red.Data()[y * red.BytesPerLine()];
		unsigned char *gLine = &green.Data()[y * green.BytesPerLine()];
		unsigned char *bLine = &blue.Data()[y * blue.BytesPerLine()];
		unsigned char *rgbLine = &data[y * bytesPerLine];
		for(int x = 0; x < width; ++x)
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
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *rLine = &red.Data()[y * red.BytesPerLine()];
		unsigned char *gLine = &green.Data()[y * green.BytesPerLine()];
		unsigned char *bLine = &blue.Data()[y * blue.BytesPerLine()];
		unsigned char *aLine = &alpha.Data()[y * alpha.BytesPerLine()];
		unsigned char *rgbaLine = &data[y * bytesPerLine];
		for(int x = 0; x < width; ++x)
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

	int width = hue.Width();
	int height = hue.Height();

	if(sat.Width() != width || vol.Width() != width ||
			sat.Height() != height || vol.Height() != height)
		throw "Input image dimensions must match.";

	// set up this image as output image
	CreateImage(width, height, 24);
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *hLine = &hue.Data()[y * hue.BytesPerLine()];
		unsigned char *sLine = &sat.Data()[y * sat.BytesPerLine()];
		unsigned char *vLine = &vol.Data()[y * vol.BytesPerLine()];
		unsigned char *rgbLine = &data[y * bytesPerLine];
		for(int x = 0; x < width; ++x)
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

	int width = hue.Width();
	int height = hue.Height();

	if(sat.Width() != width || vol.Width() != width || sat.Height() != height || 
			vol.Height() != height || alpha.Height() != height)
		throw "Input image dimensions must match.";

	// set up this image as output image
	CreateImage(width, height, 32);
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *hLine = &hue.Data()[y * hue.BytesPerLine()];
		unsigned char *sLine = &sat.Data()[y * sat.BytesPerLine()];
		unsigned char *vLine = &vol.Data()[y * vol.BytesPerLine()];
		unsigned char *aLine = &alpha.Data()[y * alpha.BytesPerLine()];
		unsigned char *rgbaLine = &data[y * bytesPerLine];
		for(int x = 0; x < width; ++x)
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
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *hLine = &hue.Data()[y * hue.BytesPerLine()];
		unsigned char *sLine = &sat.Data()[y * sat.BytesPerLine()];
		unsigned char *vLine = &vol.Data()[y * vol.BytesPerLine()];
		unsigned char *rgbLine = &data[y * bytesPerLine];
		for(int x = 0; x < width; ++x)
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
	
	for(int y = 0; y < height; ++y)
	{
		unsigned char *hLine = &hue.Data()[y * hue.BytesPerLine()];
		unsigned char *sLine = &sat.Data()[y * sat.BytesPerLine()];
		unsigned char *vLine = &vol.Data()[y * vol.BytesPerLine()];
		unsigned char *aLine = &alpha.Data()[y * vol.BytesPerLine()];
		unsigned char *rgbaLine = &data[y * bytesPerLine];
		for(int x = 0; x < width; ++x)
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

	for(int y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(int x = 0; x < width; ++x)
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

	for(int y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(int x = 0; x < width; ++x)
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

	for(int y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(int x = 0; x < width; ++x)
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

	for(int y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(int x = 0; x < width; ++x)
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

	for(int y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(int x = 0; x < width; ++x)
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

	for(int y = 0; y < height; ++y)
	{
		unsigned char *in1 = &data[y * bytesPerLine];
		unsigned char *in2 = &input.Data()[y * input.BytesPerLine()];
		unsigned char *out = &output.Data()[y * output.BytesPerLine()];
		for(int x = 0; x < width; ++x)
		{
			// add one to divisor to avoid divide by zero
			int d = *in1++ * 256 / (1 + *in2++);
			*out++ = (unsigned char)d;
		}
	}
	outputref.MoveData(output);
}

void msaImage::OverlayImage(msaImage &overlay, int destx, int desty, int w, int h)
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
		for(int y = 0; y < h; ++y)
			memcpy(&data[(desty + y) * bytesPerLine + destx], &overlay.Data()[y * overlay.BytesPerLine()], w); 
	}
	else if(depth == 24)
	{
		for(int y = 0; y < h; ++y)
			memcpy(&data[(desty + y) * bytesPerLine + destx * 3], &overlay.Data()[y * overlay.BytesPerLine()], w * 3); 
	}
	else if(depth == 32)
	{
		for(int y = 0; y < h; ++y)
		{
			for(int x = 0; x < w; ++x)
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

void msaImage::OverlayImage(msaImage &overlay, msaImage &mask, int destx, int desty, int w, int h)
{
	if(depth != overlay.Depth())
		throw "Overlay image must match depth of base image";

	if(depth == 8)
	{
		if(mask.Depth() != 8)
			throw "Mask must be 8 bit depth";

		for(int y = 0; y < h; ++y)
		{
			for(int x = 0; x < w; ++x)
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
		for(int y = 0; y < h; ++y)
		{
			for(int x = 0; x < w; ++x)
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
		for(int y = 0; y < h; ++y)
		{
			for(int x = 0; x < w; ++x)
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

void msaImage::CreateImageFromRuns(std::vector< std::list <size_t> > &runs, int bg, int fg)
{
	// calculate width by adding up runs in first scanline
	size_t width = 0;
	for(size_t len : runs[0])
		width += len;

	CreateImage(width, runs.size(), 8);
	for(int y = 0; y < runs.size(); ++y)
	{
		// each set of runs starts with background
		unsigned char color = bg;
		unsigned char *scanline = GetLine(y);
		int x = 0;
		for(size_t len : runs[y])
		{
			if(len > 0)
				memset(scanline + x, color, len);
			
			// swap colors
			if(color == bg) 
					color = fg;
			else
					color = bg;
			x += len;
		}
	}
}

