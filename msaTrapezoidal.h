#ifndef msaTrapezoid_included
#define msaTrapezoid_included

#include "msaAffine.h"

class msaPoint
{
public;
	double x;
	double y;

	msaPoint(double x_, double y_)
	{
			x = x_;
			y = y_;
	};

	// returns a value in the range of -pi to pi; 0 three o'clock, positive values are 3 to 9 o'clock
	inline void GetAngleAndIntercept(const msaPoint &other, double &radians, double &yIntercept)
	{
		double dx = other.x - x;
		double dy = other.y - y;
		return atan2(dy, dx);
	};

};

class msaTrapezoid
{
public:
	msaPoint p1;
	msaPoint p2;
	msaPoint p3;
	msaPoint p4;

	// invalid if points are colinear, or if trapezoid is concave, or forms a bow-tie
	bool IsValid();

};

class msaTrapezoidalTransform
{
protected:
	msaTrapezoid source;
	msaTrapezoid destination;

	msaPixel fill;

public:
	inline bool SetSource(const msaTrapezoid &t) 
	{ 
		t.IsValid() ? source = t : return false;
		return true;
	};
	inline bool SetDestination(const msaTrapezoid &t); 
	{ 
		t.IsValid() ? destination = t : return false;
		return true;
	};


	// for iterating through lines in output image
	inline void GetVerticalExtent(size_t &min_y, size_t &max_y)
	{
		min_y = min(min(destination.p1.y, destination.p2.y),
					min(destination.p3.y, destination.p4.y));
		max_y = max(max(destination.p1.y, destination.p2.y),
					max(destination.p3.y, destination.p4.y));
	};
	// for iterating through pixels in output image
	void GetEndpoints(size_t y, size_t &start_x, size_t &end_x, double &step);
};
#endif

