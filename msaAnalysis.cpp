#include "msaAnalysis.h"

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


