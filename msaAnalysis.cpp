#include <list>
#include "msaAnalysis.h"

inline size_t MIN(size_t x, size_t y)
{
	if(x > y) return y;
	return x;
}

inline size_t MAX(size_t x, size_t y)
{
	if(x > y) return x;
	return y;
}

inline size_t MIN3(size_t x, size_t y, size_t z)
{
	if(z > x && y > x) return x;
	if(z > y && x > y) return y;
	return z;
}

inline size_t MAX3(size_t x, size_t y, size_t z)
{
	if(x > z && x > y) return x;
	if(y > z && y > x) return y;
	return z;
}

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

void msaAnalysis::RunlengthEncodeImage(msaImage &img, std::vector< std::list<size_t> > &runs, int threshold, bool startBlack)
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
		bool bg = true;  // if bg is true, it's a background run, so skip it
		// go through runs and compare each run with each object
		for(size_t len : runs[y])
		{
			if(!bg)
			{
				printf("Run %li, %li to %li  ", y, x, len);
				// count number of objects a run intersects
				int hits = 0;
				// pointer to the last object hit by a run
				msaObject *lastObj = NULL;
				for(msaObject &obj : newObjects)
				{
					// if a run intersects an open object, add the run to that object
					// run goes from x to x + len
					if((x + len < obj.x) || (x > obj.x + obj.width)) continue;

					// we overlay the bounding box, look at the last runs in the object
					std::vector<std::pair<size_t, size_t> > &bottom_runs = obj.runs[y - 1];
					for(std::pair<size_t, size_t> &obj_run : bottom_runs)
					{
						// object run goes from obj_run. first to +obj_run.second
						// candidate run goes from x to x + len
						if((obj_run.first < x + len) || (obj_run.first + obj_run.second > x))
						{
							// if this is the first run for this line in this object, 
							if(hits == 0)
							{
								// expand the bounding box if needed
								obj.height = y - obj.y + 1;
								// add run
								obj.runs[y].push_back(std::pair<size_t, size_t>(x, len));
	
								// keep a pointer handy for additional hits
								lastObj = &obj;

								printf("Expanded object %li to location %li, %li size %li, %li run count %li\n",
									obj.index, obj.x, obj.y, obj.width, obj.height);
							}
							// see if this is a second or later hit on an object;
							if(++hits > 1)
							{
								// if it's the second hit on the same object, then just add the run
								if(lastObj == &obj)
								{
									obj.runs[y].push_back(std::pair<size_t, size_t>(x, len));
									printf("Added run to object %li,  run count %li\n",
										obj.index, obj.runs[y].size());
								}
								// if it's a hit on a different object, then merge
								else if(lastObj->width > 0)		
								{
									// find new bounding box
									size_t new_left = MIN3(obj.x, lastObj->x, obj_run.first);
									size_t new_top = MIN(obj.y, lastObj->y);
									size_t new_right = MAX3(obj.x + obj.width, lastObj->x + lastObj->width, 
													obj_run.first + obj_run.second);
									size_t new_bottom = MAX(obj.y + obj.height, lastObj->y + lastObj->height);

									obj.x = new_left;
									obj.y = new_top;
									obj.width = new_right - new_left;
									obj.height = new_bottom - new_top;
		
									// add all the runs from lastObj to obj
									for(size_t run_y = lastObj->y; run_y <= lastObj->y + lastObj->height; ++run_y)
										for(std::pair<size_t, size_t> a_run : lastObj->runs[run_y])
												obj.runs[run_y].push_back(a_run);
		
									// now zero out lastObj
									lastObj->x = 0;
									lastObj->y = 0;
									lastObj->width = 0;
									lastObj->height = 0;
									lastObj->runs.clear();

									printf("Zeroed object %li, expanded object %li to location %li, %li size %li, %li\n",
										lastObj->index, obj.index, obj.x, obj.y, obj.width, obj.height);

									// current object now becomes lastObj
									lastObj = &obj;
								}
							}
						}
					}
				}
				// if a run does not intersect any open object, create a new object with that run
				if(hits == 0)
				{
					msaObject newObj;
					newObj.x = x;
					newObj.y = y;
					newObj.width = len;
					newObj.height = 1;
					newObj.runs[y].push_back(std::pair<size_t, size_t>(x, len)); 	// initial run
					newObj.index = newObjects.size();
					newObjects.push_back(newObj);

					printf("Created new object %li, location %li, %li size %li, %li\n",
						newObj.index, newObj.x, newObj.y, newObj.width, newObj.height);
				}
			}

			// swap color, update x
			bg = !bg;
			x += len;
		}
	}

	// copy extent objects from newObjects to objects
	for(msaObject &o : newObjects)
		if(o.width != 0) objects.push_back(o);
}

