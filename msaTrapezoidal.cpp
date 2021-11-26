#include "msaTrapezoidal.h"

// return the difference between the angles in radians, -pi to pi
inline double AngleDifference(double a1, double a2)
{
	double diff = a2 - a1;
	
	// normalize
	while(diff > pi) diff -= 2.0 *pi;
	while(diff < -pi) diff += 2.0 * pi;
}

bool msaTrapezoid::IsValid()
{
	// make sure traversing the points in the order p1, p2, p3, p4 and back forms a
	// concave polygon, and that the angle between adjacent sides is non-zero (not colinear)
	double a1, a2, a3, a4, intercept;	// reuse intercept, because we don't care about it
	p1.GetAngleAndIntercept(p2, a1, intercept);
	p2.GetAngleAndIntercept(p3, a2, intercept);
	if(a1 == a2) return false;
	p3.GetAngleAndIntercept(p4, a3, intercept);
	if(a2 == a3) return false;
	p4.GetAngleAndIntercept(p1, a4, intercept);
	if(a3 == a4) return false;
	if(a4 == a1) return false;

	// All points must flow in clockwise or counter-clockwise direction.  Look at relative
	// angles between points, and all must be positive or negative; if the sign switches,
	// it means a concavity.
	if(AngleDifference(a1, a2) > 0)
	{
		if(AngleDifference(a2, a3) < 0) return false;
		if(AngleDifference(a3, a4) < 0) return false;
		if(AngleDifference(a4, a1) < 0) return false;
	}
	else
	{
		if(AngleDifference(a2, a3) > 0) return false;
		if(AngleDifference(a3, a4) > 0) return false;
		if(AngleDifference(a4, a1) > 0) return false;
	}
}

/* For a horizontal line traversing the destination at y, return the start and end points
 * in the source.  Start x value may be greater than or equal to end x value. */
void msaTrapezoidalTransform::GetEndpoints(size_t y, size_t &start_x, size_t &end_x, double &step)
{
	// If one of the destination lines is horizontal and falls along y, then return the
	// endpoint x coordinates.  Otherwise, the horizontal line at y will intercept two
	// lines in the destination; return the intercept x coordinates.
	int found = 0;
	size_t xes[4];
	// check for crossing of y (in either direction) between p1 and p2
	if((source.p1.y <= y && source.p2.y >= y) || (source.p2.y <= y && source.p1.y >= y))
	{
		if(source.p1.y == source.p2.y == y)
		{
			start_x = source.p1.x;
			end_x = source.p2.x;
			return;
		}
		xes[found++] = ((source.p1.y - y) / (y - source.p2.y)) * (source.p1.x - source.p2.x);
	}
	if((source.p2.y <= y && source.p3.y >= y) || (source.p3.y <= y && source.p2.y >= y))
	{
		if(source.p1.y == source.p2.y == y)
		{
			start_x = source.p1.x;
			end_x = source.p2.x;
			return;
		}
		xes[found++] = ((source.p2.y - y) / (y - source.p3.y)) * (source.p2.x - source.p3.x);
	}
	if((source.p3.y <= y && source.p4.y >= y) || (source.p4.y <= y && source.p3.y >= y))
	{
		if(source.p1.y == source.p2.y == y)
		{
			start_x = source.p1.x;
			end_x = source.p2.x;
			return;
		}
		xes[found++] = ((source.p3.y - y) / (y - source.p4.y)) * (source.p3.x - source.p4.x);
	}
	if((source.p4.y <= y && source.p1.y >= y) || (source.p1.y <= y && source.p4.y >= y))
	{
		if(source.p1.y == source.p2.y == y)
		{
			start_x = source.p1.x;
			end_x = source.p2.x;
			return;
		}
		xes[found++] = ((source.p4.y - y) / (y - source.p1.y)) * (source.p4.x - source.p1.x);
	}

	if(found != 2) throw "Found more than 2 crossings of a line on a convex trapezoid";

	start_x = xes[0];
	end_x = xes[1];
}


