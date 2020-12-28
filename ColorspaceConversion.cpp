
#define MAX3(a, b, c) a > b ? (a > c ? a : c) : (b > c ? b : c)
#define MIN3(a, b, c) a < b ? (a < c ? a : c) : (b < c ? b : c)

void RGBtoHSV(unsigned char R, unsigned char G, unsigned char B, unsigned char &H, unsigned char &S, unsigned char &V)
{
	// fluff up input values to preserve precision
	int r = R * 65536;
	int g = G * 65536;
	int b = B * 65536;
	int s, v, h;

	// find the minimum and maximum RGB values
	int max = MAX3(r, g, b);
	int min = MIN3(r, g, b);

	// set volume to the peak color value
	v = max;

	// if not black, determine the saturation by determining the ratio of
	//  maximum color to minimum color
	if(max > 0)
		s = (max - min) * 65536 / max * 65535;
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
			h = (g - b) * 65536 / delta;
		else if(g == max)
			h = (b - r) * 65536 / delta + 2 * 65536;
		else if(b == max)
			h = (r - g) * 65536 / delta + 4 * 65536;
		
		// h is in the range (-6 to 6) * 65536
		h /= 6;
		if(h < 0) h += 65536;
		
	}	

	// reduce values to 0-255
	H = (unsigned char)h / 256;
	S = (unsigned char)(s / 65536);
	V = (unsigned char)(v / 65536);
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

	r =  (int)(65536 * 1.0)    * Y;
	r += (int)(65536 * .948)   * (I - 32768);
	r += (int)(65536 * .624)   * (Q - 32768);
	g =  (int)(65536 * 1.0)    * Y;
	g += (int)(65536 * -.276)  * (I - 32768);
	g += (int)(65536 * -.640)  * (Q - 32768);
	b =  (int)(65536 * 1.0)    * Y;
	b += (int)(65536 * -1.105) * (I - 32768);
	b += (int)(65536 * 1.730)  * (Q - 32768);

	r /= 65536;
	g /= 65536;
	b /= 65536;

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

	y =  (int)(65536 * .299)  * R;
	y += (int)(65536 * .587)  * G;
	y += (int)(65536 * .114)  * B;
	i =  (int)(65536 * .596)  * R;
	i += (int)(65536 * -.257) * G;
	i += (int)(65536 * -.321) * B;
	q =  (int)(65536 * .212)  * R;
	q += (int)(65536 * -.528) * G;
	q += (int)(65536 * .311)  * B;

	y /= 65536;
	i /= 65536;
	q /= 65536;

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

void RGBtoYCbCr(unsigned char R, unsigned char G, unsigned char B, unsigned char *Y, unsigned char *Cb, unsigned char *Cr)
{
	int y, cb, cr;

	// RGB to YCbCr equations
	// "JPEG - Still Image Data Compression Standard" by Pennebaker and Mitchell
	// These assume coordinate values from 0.0 to 1.0
	// Y = 0.3R + 0.6G + 0.1B
	// V = R - Y
	// U = B - Y
	// Cb = U / 2 + 0.5
	// Cr = V / 1.6 + 0.5

	y =   (int)(65536 * 0.3)  * R;
	y +=  (int)(65536 * 0.6)  * G;
	y +=  (int)(65536 * 0.1)  * B;

	cb =  (int)(65536 * -0.15)  * R;
	cb += (int)(65536 * -0.3) * G;
	cb += (int)(65536 * 0.45)  * B;
	cb += (int)(65536 * 128);

	cr =  (int)(65536 * 0.4375)  * R;
	cr += (int)(65536 * -0.375) * G;
	cr += (int)(65536 * -0.0625) * B;
	cr += (int)(65536 * 128);

	y = (y + 32768) / 65536;
	cb = (cb + 32768) / 65536;
	cr = (cr + 32768) / 65536;

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


void YCbCrtoRGB(unsigned char Y, unsigned char Cb, unsigned char Cr, unsigned char &R, unsigned char &G, unsigned char &B)
{
	int r, g, b;

	// RGB to YCbCr equations
	// "JPEG - Still Image Data Compression Standard" by Pennebaker and Mitchell
	// These assume coordinate values from 0.0 to 1.0
	// Y = 0.3R + 0.6G + 0.1B
	// V = R - Y
	// U = B - Y
	// Cb = U / 2 + 0.5
	// Cr = V / 1.6 + 0.5
	int y = Y * 65536 + 32768;

	r = y;
	r += ((int)(65536 * 1.6)) * Cr;
	r += ((int)(65536 * -0.8 * 65536));

	g = y;
	g += ((int)(65536 * -0.8)) * Cr;
	g += ((int)(65536 * -0.333333)) * Cb;
	g += ((int)(65536 * 0.566666 * 65536));

	b = y;
	b += ((int)(65536 * 2)) * Cb;
	b += ((int)(65536 * -1.0 * 65536));

	r = (r + 32768) / 65536;
	g = (g + 32768) / 65536;
	b = (b + 32768) / 65536;

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

