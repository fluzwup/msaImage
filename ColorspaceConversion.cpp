
#define MAX3(a, b, c) a > b ? (a > c ? a : c) : (b > c ? b : c)
#define MIN3(a, b, c) a < b ? (a < c ? a : c) : (b < c ? b : c)

void RGBtoHSV(unsigned char R, unsigned char G, unsigned char B, unsigned char &H, unsigned char &S, unsigned char &V)
{
	// fluff up input values to preserve precision
	int r = R * 256;
	int g = G * 256;
	int b = B * 256;
	int s, v, h;

	// find the minimum and maximum RGB values
	int max = MAX3(r, g, b);
	int min = MIN3(r, g, b);

	// set volume to the peak color value
	v = max;

	// if not black, determine the saturation by determining the ratio of
	//  maximum color to minimum color
	if(max > 0)
		s = (max - min) * 256 / max * 255;
	else
		// black has no saturation
		s = 0;

	if(s == 0)
		// if there is no saturation, hue is undefined
		h = 0;
	else
	{
		// determine the hue
		int delta = max - min;
		if(r == max)
			h = (g - b) * 256 / delta;
		else if(g == max)
			h = (b - r) * 256 / delta + 2 * 256;
		else if(b == max)
			h = (r - g) * 256 / delta + 4 * 256;
		
		// h is in the range (-6 to 6) * 256
		h /= 6;
		if(h < 0) h += 256;
		
	}	

	// reduce values to 0-255
	H = (unsigned char)h;
	S = (unsigned char)(s / 256);
	V = (unsigned char)(v / 256);
}

void HSVtoRGB(unsigned char H, unsigned char S, unsigned char V, unsigned char &R, unsigned char &G, unsigned char &B)
{
	// no need to worry about out of range inputs; full range is valid

	if(S == 0)	// no saturation, therefore it's a shade of gray
	{
		R = V;
		G = V;
		B = V;		

		return;
	}

	// fluff input values up to preserve precision and speed thinsaturation up
	unsigned int h = H * 360;
	unsigned int s = S * 256;
	unsigned int v = V * 256;
	unsigned int r, g, b;

	// break the hue angle down and determine which sextant it falls in
	int i = h / 60 / 256;

	// determine where in the sextant it falls
	int f = (h % (60 * 256)) * 256 / 60;

	// find the location of the point in the tetraheadron
	int p = v * (65536 - s) / 65536;
	int q = v * (65536 - (s * f) / 65536) / 65536;
	int t = v * (65536 - (s * (65536 - f) / 65536)) / 65536;

	// now map the locations to the RGB cube
	switch(i)
	{
	case 0:
		r = v;
		g = t;
		b = p;
		break;
	case 1:
		r = q;
		g = v;
		b = p;
		break;
	case 2:
		r = p;
		g = v;
		b = t;
		break;
	case 3:
		r = p;
		g = q;
		b = v;
		break;
	case 4:
		r = t;
		g = p;
		b = v;
		break;
	case 5:
		r = v;
		g = p;
		b = q;
		break;
	}

	// convert down to unsigned chars for return
	R = (unsigned char)(r / 256);
	G = (unsigned char)(g / 256);
	B = (unsigned char)(b / 256);
}

#undef MIN3
#undef MAX3


// YIQ to RGB matrix  CCIR 601 standard
// 1.00  0.948  0.624
// 1.00 -0.276 -0.640
// 1.00 -1.105  1.730
void YIQtoRGB(unsigned char Y, unsigned char I, unsigned char Q, unsigned char &R, unsigned char &G, unsigned char &B)
{
	int r, g, b;

	r =  (int)(256 * 1.0) * Y + (int)(256 *   .948) * (I - 128) + (int)(256 *  .624) * (Q - 128);
	g =  (int)(256 * 1.0) * Y + (int)(256 *  -.276) * (I - 128) + (int)(256 * -.640) * (Q - 128);
	b =  (int)(256 * 1.0) * Y + (int)(256 * -1.105) * (I - 128) + (int)(256 * 1.730) * (Q - 128);

	r /= 256;
	g /= 256;
	b /= 256;

	if(r > 255) r = 255;
	if(r < 0) r = 0;
	if(g > 255) g = 255;
	if(g < 0) g = 0;
	if(b > 255) b = 255;
	if(b < 0) b = 0;

	R = (unsigned char)r;
	G = (unsigned char)g;
	B = (unsigned char)b;
}

// RGB to YIQ matrix  CCIR 601 standard
// 0.299  0.587  0.114
// 0.596 -0.257 -0.321
// 0.212 -0.528  0.311
void RGBtoYIQ(unsigned char R, unsigned char G, unsigned char B, unsigned char &Y, unsigned char &I, unsigned char &Q)
{
	int y, i, q;

	y =  (int)(256 * .299) * R + (int)(256 *  .587) * G + (int)(256 *  .114) * B;
	i =  (int)(256 * .596) * R + (int)(256 * -.257) * G + (int)(256 * -.321) * B;
	q =  (int)(256 * .212) * R + (int)(256 * -.528) * G + (int)(256 *  .311) * B;

	y /= 256;
	i /= 256;
	q /= 256;

	i += 128;
	q += 128;

	if(y > 255) y = 255;
	if(y < 0) y = 0;
	if(i > 255) i = 255;
	if(i < 0) i = 0;
	if(q > 255) q = 255;
	if(q < 0) q = 0;

	Y = (unsigned char)y;
	I = (unsigned char)i;
	Q = (unsigned char)q;
}

// RGB to YCbCr equations
// "JPEG - Still Image Data Compression Standard" by Pennebaker and Mitchell
// These assume coordinate values from 0.0 to 1.0
// Y = 0.3R + 0.6G + 0.1B
// V = R - Y
// U = B - Y
// Cb = U / 2 + 0.5
// Cr = V / 1.6 + 0.5
void RGBtoYCbCr(unsigned char R, unsigned char G, unsigned char B, unsigned char *Y, unsigned char *Cb, unsigned char *Cr)
{
	int y, cb, cr;

	y = (int)(256 * 0.3) * R + (int)(256 * 0.6) * G + (int)(256 * 0.1) * B;
	cb = (int)(256 * -.15) * R + (int)(256 * -.3) * G + (int)(256 *  .45) * B + 256 * 128;
	cr = (int)(256 *  0.4375)  * R + (int)(256 * -.375) * G + (int)(256 * -.0625) * B + 256 * 128;

	y = (y + 128) / 256;
	cb = (cb + 128) / 256;
	cr = (cr + 128) / 256;

	if(y > 255) y = 255;
	if(y < 0) y = 0;
	if(cb > 255) cb = 255;
	if(cb < 0) cb = 0;
	if(cr > 255) cr = 255;
	if(cr < 0) cr = 0;

	*Y = (unsigned char)y;
	*Cb = (unsigned char)cb;
	*Cr = (unsigned char)cr;
}


// RGB to YCbCr equations
// "JPEG - Still Image Data Compression Standard" by Pennebaker and Mitchell
// These assume coordinate values from 0.0 to 1.0
// Y = 0.3R + 0.6G + 0.1B
// V = R - Y
// U = B - Y
// Cb = U / 2 + 0.5
// Cr = V / 1.6 + 0.5
void YCbCrtoRGB(unsigned char Y, unsigned char Cb, unsigned char Cr, unsigned char &R, unsigned char &G, unsigned char &B)
{
	int r, g, b;

	int y = Y * 256 + 128;

	r = y + ((int)(256 * 1.6)) * Cr + ((int)(256 * -0.8 * 256));
	g = y + ((int)(256 * -0.8)) * Cr + ((int)(256 * -0.333333)) * Cb + ((int)(256 * 0.566666 * 256));
	b = y + ((int)(256 * 2)) * Cb + ((int)(256 * -1.0 * 256));

	r = (r + 128) / 256;
	g = (g + 128) / 256;
	b = (b + 128) / 256;

	if(r > 255) r = 255;
	if(r < 0) r = 0;
	if(g > 255) g = 255;
	if(g < 0) g = 0;
	if(b > 255) b = 255;
	if(b < 0) b = 0;

	R = (unsigned char)r;
	G = (unsigned char)g;
	B = (unsigned char)b;
}

// lookup tables for a 30/70/10 RGB to gray conversion
unsigned char RedToGray[] = 
{
  0,   0,   0,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   3,   3,   3, 
  3,   3,   4,   4,   4,   4,   4,   5,   5,   5,   5,   5,   6,   6,   6,   6, 
  6,   7,   7,   7,   7,   7,   8,   8,   8,   8,   8,   9,   9,   9,   9,   9, 
 10,  10,  10,  10,  10,  11,  11,  11,  11,  11,  12,  12,  12,  12,  12,  13, 
 13,  13,  13,  13,  14,  14,  14,  14,  14,  15,  15,  15,  15,  15,  16,  16, 
 16,  16,  16,  17,  17,  17,  17,  17,  18,  18,  18,  18,  18,  19,  19,  19, 
 19,  19,  20,  20,  20,  20,  20,  21,  21,  21,  21,  21,  22,  22,  22,  22, 
 22,  23,  23,  23,  23,  23,  24,  24,  24,  24,  24,  25,  25,  25,  25,  25, 
 26,  26,  26,  26,  26,  27,  27,  27,  27,  27,  28,  28,  28,  28,  28,  29, 
 29,  29,  29,  29,  30,  30,  30,  30,  30,  31,  31,  31,  31,  31,  32,  32, 
 32,  32,  32,  33,  33,  33,  33,  33,  34,  34,  34,  34,  34,  35,  35,  35, 
 35,  35,  36,  36,  36,  36,  36,  37,  37,  37,  37,  37,  38,  38,  38,  38, 
 38,  39,  39,  39,  39,  39,  40,  40,  40,  40,  40,  41,  41,  41,  41,  41, 
 42,  42,  42,  42,  42,  43,  43,  43,  43,  43,  44,  44,  44,  44,  44,  45, 
 45,  45,  45,  45,  46,  46,  46,  46,  46,  47,  47,  47,  47,  47,  48,  48, 
 48,  48,  48,  49,  49,  49,  49,  49,  50,  50,  50,  50,  50,  51,  51,  51 
};

unsigned char GreenToGray[] = 
{
  0,   1,   1,   2,   3,   4,   4,   5,   6,   6,   7,   8,   8,   9,  10,  11, 
 11,  12,  13,  13,  14,  15,  15,  16,  17,  18,  18,  19,  20,  20,  21,  22, 
 22,  23,  24,  25,  25,  26,  27,  27,  28,  29,  29,  30,  31,  31,  32,  33, 
 34,  34,  35,  36,  36,  37,  38,  39,  39,  40,  41,  41,  42,  43,  43,  44, 
 45,  46,  46,  47,  48,  48,  49,  50,  50,  51,  52,  53,  53,  54,  55,  55, 
 56,  57,  57,  58,  59,  59,  60,  61,  62,  62,  63,  64,  64,  65,  66,  67, 
 67,  68,  69,  69,  70,  71,  71,  72,  73,  74,  74,  75,  76,  76,  77,  78, 
 78,  79,  80,  81,  81,  82,  83,  83,  84,  85,  85,  86,  87,  88,  88,  89, 
 90,  90,  91,  92,  92,  93,  94,  95,  95,  96,  97,  97,  98,  99,  99, 100, 
101, 102, 102, 103, 104, 104, 105, 106, 106, 107, 108, 109, 109, 110, 111, 111, 
112, 113, 113, 114, 115, 115, 116, 117, 118, 118, 119, 120, 120, 121, 122, 122, 
123, 124, 125, 125, 126, 127, 127, 128, 129, 130, 130, 131, 132, 132, 133, 134, 
134, 135, 136, 137, 137, 138, 139, 139, 140, 141, 141, 142, 143, 144, 144, 145, 
146, 146, 147, 148, 148, 149, 150, 151, 151, 152, 153, 153, 154, 155, 155, 156, 
157, 158, 158, 159, 160, 160, 161, 162, 162, 163, 164, 165, 165, 166, 167, 167, 
168, 169, 169, 170, 171, 172, 172, 173, 174, 174, 175, 176, 176, 177, 178, 179 
};

unsigned char BlueToGray[] = 
{
  0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2, 
  2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3, 
  3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   5,   5,   5, 
  5,   5,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,   6,   6,   6,   6, 
  6,   7,   7,   7,   7,   7,   7,   7,   7,   7,   7,   8,   8,   8,   8,   8, 
  8,   8,   8,   8,   8,   9,   9,   9,   9,   9,   9,   9,   9,   9,   9,  10, 
 10,  10,  10,  10,  10,  10,  10,  10,  10,  11,  11,  11,  11,  11,  11,  11, 
 11,  11,  11,  12,  12,  12,  12,  12,  12,  12,  12,  12,  12,  13,  13,  13, 
 13,  13,  13,  13,  13,  13,  13,  14,  14,  14,  14,  14,  14,  14,  14,  14, 
 14,  15,  15,  15,  15,  15,  15,  15,  15,  15,  15,  16,  16,  16,  16,  16, 
 16,  16,  16,  16,  16,  17,  17,  17,  17,  17,  17,  17,  17,  17,  17,  18, 
 18,  18,  18,  18,  18,  18,  18,  18,  18,  19,  19,  19,  19,  19,  19,  19, 
 19,  19,  19,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  21,  21,  21, 
 21,  21,  21,  21,  21,  21,  21,  22,  22,  22,  22,  22,  22,  22,  22,  22, 
 22,  23,  23,  23,  23,  23,  23,  23,  23,  23,  23,  24,  24,  24,  24,  24, 
 24,  24,  24,  24,  24,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25,  25 
};


