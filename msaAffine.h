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

public:
	// out of bounds color value
	unsigned char oob_r;
	unsigned char oob_g;
	unsigned char oob_b;
	unsigned char oob_a;

	inline void SetToIdentity()
	{
		// maginification of 1.0, no shear, no translation
		a = 1.0;
		b = 0.0;
		c = 0.0;
		d = 1.0;
		e = 0.0;
		f = 0.0;
		UpdateInverse();
	};

	msaAffineTransform()
	{
		oob_r = 127;
		oob_g = 127;
		oob_b = 127;
		oob_a = 255;

		SetToIdentity();
	};

	void Add(const msaAffineTransform &t);

	void GetNewSize(size_t w, size_t h, size_t &wNew, size_t &hNew);

	inline void SetTransform(double Hscale, double Vscale, double Hshear, double Vshear,
					double Htranslate, double Vtranslate)
	{
		a = Hscale;
		d = Vscale;
		b = Hshear;
		c = Vshear;
		e = Htranslate;
		f = Vtranslate;
		UpdateInverse();
	};

	inline void Translate(double horizontal, double vertical)
	{
		e += horizontal;
		f += vertical;
		UpdateInverse();
	};

	inline void Scale(double scale)
	{
		// magnification values center on 1.0, not zero
		a = (a - 1.0) * scale + 1.0;
		d = (d - 1.0) * scale + 1.0;
		b *= scale;
		c *= scale;
		e *= scale;
		f *= scale;
		UpdateInverse();
	};

	void Rotate(double rotation, double center_x, double center_y);

	inline void UpdateInverse()
	{
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

