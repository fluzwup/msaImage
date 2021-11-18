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
};

class msaAnalysis
{
public:
	static void FindEdges(msaImage &input, msaImage &edgemap);
	static void GenerateProjection(msaImage &input, bool vertical, std::vector<long> &projection, long &maximum);
	static void GenerateObjectList(msaImage &input, int threshold, bool findLight, std::list<msaObject> &objects);
	static void RunlengthEncodeImage(msaImage &img, std::vector< std::list<size_t> > &runs, int threshold, bool startBlack);
};

