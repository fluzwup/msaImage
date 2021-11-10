#include "msaImage.h"
#include "msaFilters.h"

class msaAnalysis
{
public:
	void FindEdges(msaImage &input, msaImage &edgemap);
	void GenerateProjection(msaImage &input, bool vertical, 
		std::vector<long> &projection, long &maximum);
};
