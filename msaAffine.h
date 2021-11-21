#ifndef _msaAffine_included
#define _msaAffine_included
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>
#include <memory.h>

/*
	Affine transforms have the form: 
	x' = ax + by + e 
	y' = cx + dy + f

	a = scaleX * cos(rotation)
	b = scaleY * -sin(rotation)
	c = scaleX * sin(rotation)
	d = scaleY * cos(rotation)
	e = translateX
	f = translateY
*/

inline double DegreesToRadians(double degrees)
{
	return degrees * (double)3.14159265358979323846 / (double)180.0;
}

class msaAffineTransform
{
protected:
	double a;	// horz mag
	double b;	// horz shear
	double c;	// vert shear
	double d;	// vert mag
	double e;	// horz trans
	double f;	// vert trans

	// inverse values
	double a_;
	double b_;
	double c_;
	double d_;
	double e_;
	double f_;

	// fixed point values, 65536 multiplier
	long fa;	// horz mag
	long fb;	// horz shear
	long fc;	// vert shear
	long fd;	// vert mag
	long fe;	// horz trans
	long ff;	// vert trans

	// inverse values
	long fa_;
	long fb_;
	long fc_;
	long fd_;
	long fe_;
	long ff_;
public:
	// out of bounds color value
	unsigned char oob_r;
	unsigned char oob_g;
	unsigned char oob_b;
	unsigned char oob_a;

	msaAffineTransform()
	{
		oob_r = 127;
		oob_g = 127;
		oob_b = 127;
		oob_a = 255;
	};

	inline void GetNewSize(int w, int h, double scaling, double rotation, int &wNew, int &hNew)
	{
		// set up temporary transform
		msaAffineTransform temp;
		SetTransform(scaling, rotation, 0, 0);

		// transform corners of image, and track bounding box
		double minX, maxX, minY, maxY;
		minX = maxX = minY = maxY = 0.0;
		double x, y;
		x = 0.0;
		y = 0.0;
		temp.Transform(x, y);
		if(x < minX) minX = x;
		if(x > maxX) maxX = x;
		if(y < minY) minY = y;
		if(y > maxY) maxY = y;
		x = 0.0;
		y = h;
		temp.Transform(x, y);
		if(x < minX) minX = x;
		if(x > maxX) maxX = x;
		if(y < minY) minY = y;
		if(y > maxY) maxY = y;
		x = w;
		y = 0.0;
		temp.Transform(x, y);
		if(x < minX) minX = x;
		if(x > maxX) maxX = x;
		if(y < minY) minY = y;
		if(y > maxY) maxY = y;
		x = w;
		y = h;
		temp.Transform(x, y);
		if(x < minX) minX = x;
		if(x > maxX) maxX = x;
		if(y < minY) minY = y;
		if(y > maxY) maxY = y;

		wNew = (int)(maxX - minX + .99999);
		hNew = (int)(maxY - minY + .99999);
	};

	inline void SetTransform(double scaling, double rotation, size_t w, size_t h)
	{
		// cache these so we only calculate them once
		double cosrot = cos(rotation);
		double sinrot = sin(rotation);

		// if values are near zero, set to zero
		if(abs(cosrot) < 1.0e-15) cosrot = 0;
		if(abs(sinrot) < 1.0e-15) sinrot = 0;

		a = scaling * cosrot;
		b = scaling * -sinrot;
		c = scaling * sinrot;
		d = scaling * cosrot;
		e = (double)w / 2.0 * ((double)1.0 - cosrot) + (double)h / (double)2.0 * sinrot;
		f = (double)h / 2.0 * ((double)1.0 - cosrot) - (double)w / (double)2.0 * sinrot;

		// calculate inverse
		double temp = a * d - b * c;

		// don't invert if it'll cause an overflow or divide by zero
		if (fabs(temp)  < 1.0e-15)
			return;

		a_ = d / temp;
		d_ = a / temp;

		b_ = -b / temp;
		c_ = -c / temp;

		e_ = (f * b - e * d) / temp;
		f_ = (e * c - f * a) / temp;
	};

	inline void Transform(double &x, double &y)
	{
		double nx;
		double ny;
		nx = a * x + b * y + e;
		ny = c * x + d * y + f;
		x = nx;
		y = ny;
	};
	
	inline void InvTransform(double &x, double &y)
	{
		double nx;
		double ny;
		nx = a_ * x + b_ * y + e_;
		ny = c_ * x + d_ * y + f_;
		x = nx;
		y = ny;
	};
};

#endif

