#ifndef _msaFilters_included
#define _msaFilters_included
#include <vector>
#include "msaImage.h"

class msaFilters
{
public:
	msaFilters();

	enum class FilterType
	{
		Undefined = 0,
		UserDefined,
		Dilate,
		Erode,
		Median,
		Gaussian,
		Sharpen
	};

	// read/write access to filter values
	inline int &Val(int x, int y)
	{
		int index = y * m_width + x;
		if(index >= (int)m_values.size())
			throw "Value is out of bounds";

		return m_values[y * m_width + x];
	};

	// read access to filter parameters
	int GetWidth() { return m_width; };
	int GetHeight() { return m_height; };
	int GetDivisor() { return m_divisor; };
	FilterType GetType() { return m_type; };

	// user defined convolution filter
	void SetUserDefined(const int *vals, int w, int h, int cx, int cy, int divisor);
	// predefiend filters
	void SetType(FilterType type, int w, int h);
	// apply filter to the image
	void FilterImage(msaImage &input, msaImage &output);

protected:
	FilterType m_type;
	std::vector<int> m_values;
	int m_count;
	int m_width;
	int m_height;
	int m_divisor;

	int m_cx;
	int m_cy;

	void SetToGaussian(int w, int h);
	void SetToSharpen(int w, int h);
	void SetFilterSize(int w, int h);

	// colorspace specific functions, called by generic functions
	void Filter8(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void Filter24(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void Filter32(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void Dilate8(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void Dilate24(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void Dilate32(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void Erode8(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void Erode24(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void Erode32(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void MedianFilter8(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void MedianFilter24(unsigned char *input, unsigned char *output, int w, int h, int bpl);
	void MedianFilter32(unsigned char *input, unsigned char *output, int w, int h, int bpl);
};
#endif

