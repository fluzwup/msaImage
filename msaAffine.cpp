#include "msaAffine.h"

void msaAffineTransform::Add(const msaAffineTransform &t)
{
	double na, nb, nc, nd, ne, nf;

	na = a * t.a + c * t.b;
	nb = b * t.a + d * t.b;
	nc = a * t.c + c * t.d;
	nd = b * t.c + d * t.d;
	ne = e * t.a + f * t.b + t.e;
	nf = e * t.c + f * t.d + t.f;

	a = na;
	b = nb;
	c = nc;
	d = nd;
	e = ne;
	f = nf;
	UpdateInverse();
};

void msaAffineTransform::GetNewSize(size_t w, size_t h, size_t &wNew, size_t &hNew)
{
	// transform corners of image, and track bounding box
	double minX, maxX, minY, maxY;
	minX = maxX = minY = maxY = 0.0;
	double x, y;
	x = 0.0;
	y = 0.0;
	Transform(x, y);
	if(x < minX) minX = x;
	if(x > maxX) maxX = x;
	if(y < minY) minY = y;
	if(y > maxY) maxY = y;
	x = 0.0;
	y = h;
	Transform(x, y);
	if(x < minX) minX = x;
	if(x > maxX) maxX = x;
	if(y < minY) minY = y;
	if(y > maxY) maxY = y;
	x = w;
	y = 0.0;
	Transform(x, y);
	if(x < minX) minX = x;
	if(x > maxX) maxX = x;
	if(y < minY) minY = y;
	if(y > maxY) maxY = y;
	x = w;
	y = h;
	Transform(x, y);
	if(x < minX) minX = x;
	if(x > maxX) maxX = x;
	if(y < minY) minY = y;
	if(y > maxY) maxY = y;

	wNew = (size_t)(maxX - minX + .99999);
	hNew = (size_t)(maxY - minY + .99999);
};

void msaAffineTransform::Rotate(double rotation, double center_x, double center_y)
{
	// shift center to origin
	Translate(-center_x, -center_y);

	// cache these so we only calculate them once
	double cosrot = cos(rotation);
	double sinrot = sin(rotation);

	// if values are near zero, set to zero
	if(abs(cosrot) < 1.0e-15) cosrot = 0;
	if(abs(sinrot) < 1.0e-15) sinrot = 0;

	msaAffineTransform temp;
	temp.SetTransform(cosrot, cosrot, -sinrot, sinrot, 0.0, 0.0);
	Add(temp);

	// shift back
	Translate(center_x, center_y);
	UpdateInverse();
};



