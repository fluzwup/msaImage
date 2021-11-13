#include "msaImage.h"
#include "msaFilters.h"
#include <vector>
#include <utility>

class msaObject
{
public:
	// bounding box
	int x;
	int y;
	int width;
	int height;

	// runs that make up object, starting with a background run
	std::vector< std::list<size_tt> > runs;
};

class msaAnalysis
{
public:
	static void FindEdges(msaImage &input, msaImage &edgemap);
	static void GenerateProjection(msaImage &input, bool vertical, 
		std::vector<long> &projection, long &maximum);
	static void GenerateObjectList(msaImage &input, int threshold,
		bool findLight, std::list<msaObject> &objects);
};

