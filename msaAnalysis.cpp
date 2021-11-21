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

bool msaObject::AddObjectToImage(msaImage &img, const msaPixel &foreground)
{
	if(img.Width() < x + width) return false;
	if(img.Height() < y + height) return false;

	// for each scanline in map of runs, set all pixels in run to foreground color
	for(size_t line_y = y; line_y < y + height; ++line_y)
	{
		for(std::pair<size_t, size_t> &p : runs[line_y])
		{
			for(size_t run_x = p.first; run_x < p.first + p.second; ++run_x)
			{
				img.SetPixel(run_x, line_y, foreground);
			}
		}
	}
	return true;
}

bool msaObject::DoesRunIntersect(size_t run_y, size_t run_x, size_t run_len, bool addRun)
{
	// bail if the run doesn't touch the bounding box
	if(run_y < y - 1) return false;
	if(run_y > y + height + 1) return false;

	if(run_x > x + width + 1) return false;
	if(run_x + run_len < x - 1) return false;

	// see if there are any runs adjacent to the run_y; if not, exit
	if(runs[run_y - 1].size() == 0 && runs[run_y + 1].size() == 0) 
			return false;

	// The run does touch the bounding box, and there is a run on or adjacent to run_y
	// The nature of runlength encoding means runs will not abut horizontally (because
	// then they wouldn't be separate runs).  Check the line above and the line below
	// for 4-way connections (directly above or below, not diagonal).
	bool overlap = false;
	for(std::pair<size_t, size_t> &p : runs[run_y + 1])
	{
		// if runs overlap, then the start of one run will fall within the range of the other
		// try both ways
		// do a [left, right) comparison
		if((run_x >= p.first && run_x < p.first + p.second) ||
			(p.first >= run_x && p.first < run_x + run_len))
		{
			overlap = true;
			break;
		}
	}
	if(!overlap)
	{
		for(std::pair<size_t, size_t> &p : runs[run_y - 1])
		{
			if((run_x >= p.first && run_x < p.first + p.second) ||
				(p.first >= run_x && p.first < run_x + run_len))
			{
				overlap = true;
				break;
			}
		}
	}
			
	// make sure the run doesn't already exist
	if(overlap && addRun)
	{
		bool exists = false;
		for(std::pair<size_t, size_t> &p : runs[run_y])
		{
			if(p.first == run_x && p.second == run_len)
			{
				exists = true;
				break;
			}
		}
		if(!exists)
		{
			runs[run_y].push_back(std::pair<size_t, size_t>(run_x, run_len));

			// now update bounding box
			if(run_y < y)
			{
				height = (y + height) - run_y;
				y = run_y;
			}
			else if(run_y > y + height)
			{
				height = run_y - y;
			}
		}
	}
	
	return overlap;
}

bool msaObject::MergeObject(msaObject &otherObj)
{
	// TODO:  add validity check:  make sure bounding boxes touch or overlap
	
	// find new bounding box
	size_t new_left = MIN(x, otherObj.x);
	size_t new_top = MIN(y, otherObj.y);
	size_t new_right = MAX(x + width, otherObj.x + otherObj.width); 
	size_t new_bottom = MAX(y + height, otherObj.y + otherObj.height);

	x = new_left;
	y = new_top;
	width = new_right - new_left;
	height = new_bottom - new_top;

	// copy over runs
	for(size_t line = y; line < y + height; ++line)
		for(std::pair<size_t, size_t> &p : otherObj.runs[line])
			runs[line].push_back(p);

	// blank out otherObj
	otherObj.x = 0;
	otherObj.y = 0;
	otherObj.width = 0;
	otherObj.height = 0;
	otherObj.runs.clear();

	return true;
}

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
			unsigned char *byte = input.GetRawLine(y);
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
			unsigned char *byte = input.GetRawLine(y);
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
		unsigned char *scanline = img.GetRawLine(y);
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
			if(bg)
			{
				// swap color, update x
				bg = !bg;
				x += len;
				continue;
			}

			printf("Run %li, %li to %li  ", y, x, x + len);
			// count number of objects a run intersects
			int hits = 0;
			// pointer to the last object hit by a run
			msaObject *lastObj = NULL;
			for(msaObject &obj : newObjects)
			{
				if(hits == 0)
				{
					// do add intersecting runs for first hits
					if(obj.DoesRunIntersect(y, x, len, true))
					{
						++hits;
						printf("First hit, object %li new height %li\n", obj.index, obj.height);
						lastObj = &obj;
					}
				}
				else
				{
					if(obj.DoesRunIntersect(y, x, len, false))
					{
						++hits;
						printf("Hit %i, object %li merging into %li\n", 
										hits, obj.index, lastObj->index);
						lastObj->MergeObject(obj);
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

			// swap color, update x
			bg = !bg;
			x += len;
		}
	}

	// copy extent objects from newObjects to objects
	for(msaObject &o : newObjects)
		if(o.width != 0) objects.push_back(o);
}

