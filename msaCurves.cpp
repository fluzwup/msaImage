// t and slope are the parameters, xout and yout are the points returned
void BezierCurve(double t, double slope, double &xout, double &yout)
{
	double P1x = 0;
	double P1y = 0;
	double P2x = 0;
	double P2y = 0;
	double P3x = 0;
	double P3y = 0;
	double P4x = 1;
	double P4y = 1;
	double x, y;

	if(slope < .5)
	{
		P2x = (.5 - slope) * 2;
	}
	else
	{
		P2y = (slope - .5) * 2;
	}

	P3x = 1 - P2x;
	P3y = 1 - P2y;

	x = P1x * pow(1 - t, 3) + 
			P2x * 3 * t * pow(1 - t, 2) + 
			P3x * 3 * pow(t, 2) * (1 - t) + 
			P4x * pow(t, 3);
	y = P1y * pow(1 - t, 3) + 
			P2y * 3 * t * pow(1 - t, 2) + 
			P3y * 3 * pow(t, 2) * (1 - t) + 
			P4y * pow(t, 3);

	xout = x;
	yout = y;
}

// use an Bezier S curve to generate a remapping curve
void Compander(double x, double y, std::vector<unsigned char> &outarray)
{
	double t;
	size_t last;

	int array[256] = {-1};

	// sweep the parameter to get the parametric curve
	for(t = 0; t < 1; t += .01)
	{
		double dx, dy;

		// use y for the slope
		BezierCurve(t, y, &dx, &dy);
		
		// now shift along the tangental axis by x
		if(y < .5)
			dx += (x - .5) * 2;
		else
			dy += (x - .5) * 2;

		// clip values to the range 0-1
		if(dx > 1) dx = 1;
		if(dy > 1) dy = 1;
		if(dx < 0) dx = 0;
		if(dy < 0) dy = 0;

		// convert to 0-255
		dx *= 255;
		dy *= 255;

		// if there is not already a value at this point, enter it
		if(array[(size_t)dx] == -1)	array[(size_t)dx] = (int)dy;
	}

	// the array may be sparsely filled, so fill in blank spots
	// this finds the first valid value
	for(size_t i = 1; i < 256; ++i)
		if(array[i] != -1)
		{
			last = array[i];
			i = 256;
		}

	// set up output array
	outarray.resize(256);

	// fill in blank spots with copies of last valid value
	for(size_t i = 0; i < 256; ++i)
	{
		if(array[i] == -1)
			array[i] = last;
		else
			last = array[i];

		// convert to unsigned char for output
		outarray[i] = (unsigned char)array[i];
	}
}

void Gamma(double x, double, std::vector<unsigned char> array)
{
	// .50 will generate a straight line
	x = x * 2;

	if(x == 0) x = .001;

	array.resize(256);
	for(size_t i = 0; i < 256; ++i)
	{
		int v = (int)(255.0 * pow(i / 255.0, x));
		if(v > 255) v = 255;
		if(v < 0) v = 0;
		array[i] = v;
	}
}



