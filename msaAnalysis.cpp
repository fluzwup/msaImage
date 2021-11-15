#include <list>
#include "msaAnalysis.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

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

void msaAnalysis::RunlengthEncodeImage(msaImage &img, std::vector< std::list<size_t> > runs, int threshold, bool startBlack)
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

void msaAnalysis::GenerateObjectList(msaImage &img, int threshold, bool findLight, std::list<msaObject> &objects)
{
	// don't want to return an incorrect list, to clear out the vector
	objects.clear();

	// first convert the image to a set of runs, split on the threshold value
	// for now, assume we're looking for light objects on a dark background
	// runs will start light, then alternate dark/light/dark...
	std::vector< std::list<size_t> > runs;
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
				// pointer to the last object hit by a run
				msaObject *lastObj = NULL;
				for(msaObject &obj : newObjects)
				{
					// if a run intersects an open object, add the run to that object
					// run goes from x to x + len
					if((x + len < obj.x) || (x > obj.x + obj.width)) continue;

					// in the bounding box, look at obj.runs.back() runs for overlap
					std::list<size_t> &obj_runs_back = obj.runs.back();
					bool obj_bg = true;
					size_t obj_x = 0;
					for(size_t obj_len : obj_runs_back)
					{
						// ignore bits of run that are part of the background
						if(obj_bg) continue;

						// object run goes from obj_x to obj_x + obj_len
						// candidate run goes from x to x + len
						if((obj_x < x + len) || (obj_x + obj_len > x))
						{
							// if this is the first run for this line in this object, 
							if(hits == 0)
							{
								// expand the bounding box
								++obj.height;
								//  then set up the new entry in the vector
								obj.runs.resize(obj.runs.size() + 1);
								//  and set to the length of the background run to this point
								obj.runs.back().push_back(x);
	
								// keep a pointer handy for additional hits
								lastObj = &obj;
							}
							// see if this is a second or later hit on an object; if so, merge them
							if(++hits > 1)
							{
								// merge runs first; new run has already been added to obj
								// TODO: need to line up runs, and combine them in horizontal order
								// probably need to conver to start/end, since they may be interleaved between objects
	
								// now find new bounding box
								size_t new_left = MIN(obj.x, lastObj->x);
								size_t new_top = MIN(obj.y, lastObj->y);
								size_t new_right = MAX(obj.x + obj.width, lastObj->x + lastObj->width);
								size_t new_bottom = MAX(obj.y + obj.height, lastObj->y + lastObj->height);
								obj.x = new_left;
								obj.y = new_top;
								obj.width = new_right - new_left;
								obj.height = new_bottom - new_top;
	
								// now zero out lastObj
								lastObj->x = 0;
								lastObj->y = 0;
								lastObj->width = 0;
								lastObj->height = 0;
								lastObj->runs.resize(0);
							}
							else
							{
								// obj.runs.back now points to scanline y of the image
								obj.runs.back().push_back(len);
							}
						}
						obj_bg = !obj_bg;
						obj_x += len;
					} 	// end of loop through obj_runs_back
				}
				// if a run does not intersect any open object, create a new object with that run
				msaObject newObj;
				newObj.x = x;
				newObj.y = y;
				newObj.width = len;
				newObj.height = 1;
				newObj.runs.resize(1);
				newObj.runs[0].push_back(x); 	// background run
				newObj.runs[0].push_back(len);	// top left run of object
				newObjects.push_back(newObj);
			}

			// swap color, update x
			bg = !bg;
			x += len;
		}

		// close any open objects with no runs at the current y coordinate

	}
}

