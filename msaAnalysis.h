#include "msaImage.h"
#include "msaFilters.h"
#include <vector>
#include <map>
#include <utility>

class msaObject
{
public:
	size_t index;

	// bounding box
	size_t x;
	size_t y;
	size_t width;
	size_t height;

	// runs that make up object, starting with a background run
	// the initial size_t is the y coordinate
	std::map<size_t, std::vector<std::pair<size_t, size_t> > > runs;

	// if addRun is true, run is added if it intersects
	bool DoesRunIntersect(size_t y, size_t x, size_t len, bool addRun = false);
	// returns false if objects cannot be merged
	bool MergeObject(msaObject &otherObj);

	// assume img is already extant, and runs will be overlayed in foreground color
	bool AddObjectToImage(msaImage &img, const msaPixel &foreground);
};

class msaAnalysis
{
public:
	static void FindEdges(msaImage &input, msaImage &edgemap);
	static void GenerateProjection(msaImage &input, bool vertical, std::vector<long> &projection, long &maximum);
	static void GenerateObjectList(msaImage &input, int threshold, bool findLight, std::list<msaObject> &objects);
	static void RunlengthEncodeImage(msaImage &img, std::vector< std::list<size_t> > &runs, int threshold, bool startBlack);
};

