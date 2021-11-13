#include <list>
#include "msaAnalysis.h"

// temporary, for debugging
bool SavePNG(const char *filename, msaImage &img);

void msaAnalysis::FindEdges(msaImage &input, msaImage &edgemap)
{
	if(input.Depth() > 8)
	{
		// split into RGB planes, run each plane separately and combine the results
		msaImage red, green, blue;
		input.SplitRGB(red, green, blue);

		msaImage redge, gedge, bedge;
		FindEdges(red, redge);
		FindEdges(green, gedge);
		FindEdges(blue, bedge);

		redge.MaxImages(gedge, edgemap);
		edgemap.MaxImages(bedge, edgemap);

		return;
	}

	// 
}

void msaAnalysis::GenerateProjection(msaImage &input, bool vertical, 
		std::vector<long> &projection, long &maximum)
{
	projection.clear();

	int width = input.Width();
	int height = input.Height();
	int bytesPerPixel = input.Depth() / 8;
	int bytesWide = width * bytesPerPixel;

	if(vertical)
	{
		projection.resize(width);
		for(int y = height - 1; y >= 0; --y)
		{
			unsigned char *byte = input.GetLine(y);
			for(int x = bytesWide - 1; x >= 0; --x)
			{
				// sum up r + g + b + a
				projection[x / bytesPerPixel] += byte[x];
			}
		}
	}
	else
	{
		projection.resize(height);
		for(int y = height - 1; y >= 0; --y)
		{
			unsigned char *byte = input.GetLine(y);
			for(int x = bytesWide - 1; x >= 0; --x)
			{
				// sum up r + g + b + a
				projection[y] += byte[x];
			}
		}
	}
	maximum = 0;
	for(long l : projection)
		if(l > maximum) maximum = l;
}

void msaAnalysis::RunlengthEncodeImage(msaImage, std::vector< std::list<size_t> > runs, int threshold, bool startBlack)
{
	runs.clear();

	// only works on gray images
	if(img.Depth() != 8) throw "Object lists can only be generated on gray images";
	if(threshold >= 255) throw "Threshold must be below 255";
	if(threshold < 0) throw "Threshold must be positive";
	// each element of the outer vector holds a scanline
	// each element of the inner list describes a transition, light to dark or back
	// each scanline starts with dark (and may be zero)
	runs.resize(img.Height());
	for(int y = 0; y < img.Height(); ++y)
	{
		unsigned char *scanline = img.GetLine(y);
		int last = 0;
		int x = 0;
		// set up first run (may end up being zero length)
		while(x < img.Width())
		{
			// iterate until we find a pixel over the threshold
			while(x < img.Width() && scanline[x] <= threshold) ++x;
			// log a transition from dark to light
			runs[y].push_back(x - last);
			last = x;
			while(x < img.Width() && scanline[x] > threshold) ++x;
			// log a transition from light to dark
			runs[y].push_back(x - last);
			last = x;
		}
	}
	// NOTE:  Scanlines may start or end with a zero length run
}

void msaAnalysis::GenerateObjectList(msaImage &img, int threshold, 
	bool findLight, std::list<msaObject> &objects)
{
	// don't want to return an incorrect list, to clear out the vector
	objects.clear();

	// first convert the image to a set of runs, split on the threshold value
	// for now, assume we're looking for light objects on a dark background
	// runs will start light, then alternate dark/light/dark...
	RunlengthEncodeImage(img, runs, threshold, findLight);

	if(!findLight)
	{
		// invert each run set by adding or removing zero length run to the front
		for(int y = 0; y < img.Height(); ++y)
		{
			if(runs[y].front() == 0) 
				runs[y].pop_front();
			else
				runs[y].push_front(0);
		}
	}

	// set up a list for new objects; they will be added to the objects list as they're finished
	std::list<msaObject> newObjects;
	for(size_t y = 0; y < runs.size(); ++y)
	{
		size_t x = 0;
		bool bg = false;  // if bg is true, it's a background run, so skip it
		// go through runs and compare each run with each object
		for(size_t len : runs[y])
		{
			if(!bg)
			{
				// count number of objects a run intersects
				int hits = 0;
				for(msaObject obj : newObjects)
				{
					// if a run intersects an open object, add the run to that object
					// run goes from x to x + len
					if(
					// work from back to front in object's list of runs
					
					// if a run that has been added to an object intersects another object, merge the objects
					if(hits > 1)
					{
					}
				}
				// if a run does not intersect any open object, create a new object with that run
			}

			// swap color, update x
			bg = !bg;
			x += len;
		}

		// close any open objects with no runs at the current y coordinate

	}

}

