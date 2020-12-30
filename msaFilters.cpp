#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include <sys/time.h>
#include <memory.h>
#include "msaFilters.h"
#include "ColorspaceConversion.h"

using namespace std;

msaFilters::msaFilters()
{
	m_type = FilterType::Undefined;
	m_width = 0;
	m_height = 0;
	m_divisor = 1;
	m_count = 0;
	m_values.resize(m_count);
}

void msaFilters::FilterImage(msaImage &input, msaImage &output)
{
	// grab input image parameters
	int w = input.Width();
	int h = input.Height();
	int bpl = input.BytesPerLine();
	int depth = input.Depth();
	unsigned char *indata = input.Data();

	// we're going to allocate our own space, so we can be sure input BPL and output BPL match,
	//  and we'll give the data to the output image so it can delete it
	unsigned char *outdata = new unsigned char[h * bpl];

	switch(m_type)
	{
	case FilterType::UserDefined:
	case FilterType::Gaussian:
	case FilterType::Sharpen:
		switch(depth)
		{
		case 8:
			Filter8(indata, outdata, w, h, bpl);
			break;
		case 24:
			Filter24(indata, outdata, w, h, bpl);
			break;
		case 32:
			Filter32(indata, outdata, w, h, bpl);
			break;
		default:
			throw "Invalid image depth";
			break;
		}
		break;
	case FilterType::Dilate:
		switch(depth)
		{
		case 8:
			Dilate8(indata, outdata, w, h, bpl);
			break;
		case 24:
			Dilate24(indata, outdata, w, h, bpl);
			break;
		case 32:
			Dilate32(indata, outdata, w, h, bpl);
			break;
		default:
			throw "Invalid image depth";
			break;
		}
		break;
	case FilterType::Erode:
		switch(depth)
		{
		case 8:
			Erode8(indata, outdata, w, h, bpl);
			break;
		case 24:
			Erode24(indata, outdata, w, h, bpl);
			break;
		case 32:
			Erode32(indata, outdata, w, h, bpl);
			break;
		default:
			throw "Invalid image depth";
			break;
		}
		break;
	case FilterType::Median:
		switch(depth)
		{
		case 8:
			MedianFilter8(indata, outdata, w, h, bpl);
			break;
		case 24:
			MedianFilter24(indata, outdata, w, h, bpl);
			break;
		case 32:
			MedianFilter32(indata, outdata, w, h, bpl);
			break;
		default:
			throw "Invalid image depth";
			break;
		}
		break;
	default:
		throw "Invalid filter type";
	}

	// create the output image and give it the filtered data
	output.TakeExternalData(input.Width(), input.Height(), input.BytesPerLine(), input.Depth(), outdata);
}

void msaFilters::SetType(FilterType type, int w, int h)
{
	switch(type)
	{
	case FilterType::Dilate:
		SetFilterSize(w, h);
		m_type = type;
		break;
	case FilterType::Erode:
		SetFilterSize(w, h);
		m_type = type;
		break;
	case FilterType::Median:
		SetFilterSize(w, h);
		m_type = type;
		break;
	case FilterType::Gaussian:
		SetToGaussian(w, h);
		m_type = type;
		break;
	case FilterType::Sharpen:
		SetToSharpen(w, h);
		m_type = type;
		break;
	case FilterType::UserDefined:
		throw "User defined filters must be set with SetUserDefined";
		break;
	default:
		throw "SetType must be called with a defined filter type";
	}
}

void msaFilters::SetFilterSize(int w, int h)
{
	// set the thing special filters need
	m_width = w;
	m_height = h;

	// clear the rest
	m_divisor = 1;
	m_count = 0;
	m_values.resize(m_count);
}

void msaFilters::SetUserDefined(const int *vals, int w, int h, int cx, int cy, int divisor)
{
	m_type = FilterType::UserDefined;
	m_width = w;
	m_height = h;
	m_divisor = divisor;
	m_count = w * h;
	m_values.resize(m_count);

	m_cx = cx;
	m_cy = cy;

	int i;
	for(i = 0; i < m_count; ++i)
	{
		m_values[i] = vals[i];
	}

	// an invalid divisor, such as zero, we'll take to mean normalize
	if(divisor == 0)
	{
		int sum = 0;
		for(i = 0; i < m_count; ++i)
		{
			sum += m_values[i];
		}
		m_divisor = sum;
	}
	else
		m_divisor = divisor;

	if(m_divisor == 0) m_divisor = 1;
}

void msaFilters::SetToGaussian(int w, int h)
{
	// make sure it's big enough
	if (w < 3 || h < 3)
		throw "Width or height of Gaussian filter must be at least 3 pixels";

	// make sure both dimensions are odd
	if(!(w & h & 1))	// if w or h is odd, this will be false
		throw "Width and height of Gaussian filter must both be odd";

	m_cx = w / 2;
	m_cy = h / 2;

	// set up with Gaussian curve values
	double normalized;
	int vals[w * h];
	for (double x = 0; x < w; x += 1.0)
	{
		// find the central peak
		normalized = (m_cx - x) * (m_cx - x) / (m_cx * m_cx);
		double peak = exp(-3.14 * normalized) * 255;

		for (double y = 0; y < h; y += 1.0)
		{
			normalized = (m_cy - y) * (m_cy - y) / (m_cy * m_cy);
			vals[(int)y * w + (int)x] = peak * exp(-3.14 * normalized);
		}
	}

	// set up values, divisor of 0 will normalize
	SetUserDefined(vals, w, h, m_cx, m_cy, 0);
}

void msaFilters::SetToSharpen(int w, int h)
{
	// make sure it's big enough
	if (w < 5 || h < 5)
		throw "Width or height of sharpening filter must be at least 5 pixels";

	// make sure both dimensions are odd
	if(!(w & h & 1))	// if w or h is odd, this will be false
		throw "Width and height of sharpening filter must both be odd";

	// set up array to store filter values
	int vals[w * h];

	// set up a Gaussian filter
	msaFilters f1;
	f1.SetToGaussian(w, h);

	// set up a smaller Gaussian filter
	// use half the size, rounded up and odd
	int w2 = ((w + 1) / 2) | 1;
	int h2 = ((h + 1) / 2) | 1;
	msaFilters f2;
	f2.SetToGaussian(w2, h2);

	// invert larger Gaussian into the array
	for(int y = 0; y < h; ++y)
		for(int x = 0; x < w; ++x)
			vals[y * w + x] = -1 * f1.Val(x, y);

	// determine magnitude difference; we want the positive filter to have about 25% more weight than the negative
	double magnitude = 1.25 * f1.GetDivisor() / f2.GetDivisor();

	// now add in the smaller, increasing the magnitude
	int xoffset = (w - w2) / 2;
	int yoffset = (h - h2) / 2;
	for(int y = 0; y < h2; ++y)
		for(int x = 0; x < w2; ++x)
			vals[(y + yoffset) * w + (x + xoffset)] += (int)(magnitude * f2.Val(x, y));

	// set up values, divisor of 0 will normalize
	SetUserDefined(vals, w, h, w / 2, h / 2, 0);
}

inline unsigned char *GetClippedValue24(int x, int y, unsigned char *data, int w, int h, int bpl)
{
	if(x >= w) x = w - 1;
	if(y >= h) y = h - 1;
	if(x < 0) x = 0;
	if(y < 0) y = 0;

	return &data[y * bpl + x * 3];
}

inline unsigned char *GetClippedValue32(int x, int y, unsigned char *data, int w, int h, int bpl)
{
	if(x >= w) x = w - 1;
	if(y >= h) y = h - 1;
	if(x < 0) x = 0;
	if(y < 0) y = 0;

	return &data[y * bpl + x * 4];
}

inline unsigned char *GetClippedValue8(int x, int y, unsigned char *data, int w, int h, int bpl)
{
	if(x >= w) x = w - 1;
	if(y >= h) y = h - 1;
	if(x < 0) x = 0;
	if(y < 0) y = 0;

	return &data[y * bpl + x];
}

/*
	For color values; since we're going to sort by gray values (the only way to really do a median filter) then
	we need to have some way to get from the median value back to the source RGB value.
*/
void msaFilters::MedianFilter24(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_width / 2;
	int endx = w - startx - 1;
	int starty = m_height / 2;
	int endy = h - starty - 1;	

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)		
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long colors[256] = {0};

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 3];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;

					int gray = RGBtoGray(r, g, b);
					++count[gray];

					// create an RGB int to store so we can obtain the median color later
					int rgb = (((int)r) << 16) + (((int)g) << 8) + (int)b;
					if(colors[gray] != 0)
					{
						if(rgb > colors[gray]) colors[gray] = rgb;
					}
					else
						colors[gray] = rgb;
					
				}
			}

			// find median value
			int total = m_count / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = colors[i] >> 16;
			*pout++ = (colors[i] >> 8) & 0xff;
			*pout++ = colors[i] & 0xff;
		}
	}

	// now deal with the edges
	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long colors[256] = {0};
			long totalVals = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if(imgY - starty + filtY < 0) continue;

				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 3];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					++totalVals;
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;

					int gray = RGBtoGray(r, g, b);
					++count[gray];

					// create an RGB int to store so we can obtain the median color later
					int rgb = (((int)r) << 16) + (((int)g) << 8) + (int)b;
					if(colors[gray] != 0)
					{
						if(rgb > colors[gray]) colors[gray] = rgb;
					}
					else
						colors[gray] = rgb;
					
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = colors[i] >> 16;
			*pout++ = (colors[i] >> 8) & 0xff;
			*pout++ = colors[i] & 0xff;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long colors[256] = {0};
			long totalVals = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if(imgY - starty + filtY >= h) continue;

				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 3];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					++totalVals;
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;

					int gray = RGBtoGray(r, g, b);
					++count[gray];

					// create an RGB int to store so we can obtain the median color later
					int rgb = (((int)r) << 16) + (((int)g) << 8) + (int)b;
					if(colors[gray] != 0)
					{
						if(rgb > colors[gray]) colors[gray] = rgb;
					}
					else
						colors[gray] = rgb;
					
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = colors[i] >> 16;
			*pout++ = (colors[i] >> 8) & 0xff;
			*pout++ = colors[i] & 0xff;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long colors[256] = {0};
			long totalVals = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if((imgY - starty + filtY >= h) || (imgY - starty + filtY < 0)) continue;

				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// skip the column if it takes us out of bounds
					if(imgX - startx + filtX < 0) continue;

					unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx + filtX) * 3];

					++totalVals;
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;

					int gray = RGBtoGray(r, g, b);
					++count[gray];

					// create an RGB int to store so we can obtain the median color later
					int rgb = (((int)r) << 16) + (((int)g) << 8) + (int)b;
					if(colors[gray] != 0)
					{
						if(rgb > colors[gray]) colors[gray] = rgb;
					}
					else
						colors[gray] = rgb;
					
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = colors[i] >> 16;
			*pout++ = (colors[i] >> 8) & 0xff;
			*pout++ = colors[i] & 0xff;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx * 3];

		for(imgX = endx; imgX < w; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long colors[256] = {0};
			long totalVals = 0;
			
			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if((imgY - starty + filtY >= h) || (imgY - starty + filtY < 0)) continue;

				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// skip the column if it takes us out of bounds
					if(imgX - startx + filtX >= w) continue;

					unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx + filtX) * 3];

					++totalVals;
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;

					int gray = RGBtoGray(r, g, b);
					++count[gray];

					// create an RGB int to store so we can obtain the median color later
					int rgb = (((int)r) << 16) + (((int)g) << 8) + (int)b;
					if(colors[gray] != 0)
					{
						if(rgb > colors[gray]) colors[gray] = rgb;
					}
					else
						colors[gray] = rgb;
					
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = colors[i] >> 16;
			*pout++ = (colors[i] >> 8) & 0xff;
			*pout++ = colors[i] & 0xff;
		}
	}
}

void msaFilters::Dilate24(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_width / 2;
	int endx = w - startx - 1;
	int starty = m_height / 2;
	int endy = h - starty - 1;

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmax = 0;
			long gmax = 0;
			long bmax = 0;
			long max = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 3];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					if(r + b + g > max)
					{
						rmax = r;
						gmax = g;
						bmax = b;
						max = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmax;
			*pout++ = gmax;
			*pout++ = bmax;
		}
	}

	// now deal with the edges

	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmax = 0;
			long gmax = 0;
			long bmax = 0;
			long max = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue24(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					if(r + b + g > max)
					{
						rmax = r;
						gmax = g;
						bmax = b;
						max = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmax;
			*pout++ = gmax;
			*pout++ = bmax;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmax = 0;
			long gmax = 0;
			long bmax = 0;
			long max = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue24(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					if(r + b + g > max)
					{
						rmax = r;
						gmax = g;
						bmax = b;
						max = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmax;
			*pout++ = gmax;
			*pout++ = bmax;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			long rmax = 0;
			long gmax = 0;
			long bmax = 0;
			long max = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue24(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					if(r + b + g > max)
					{
						rmax = r;
						gmax = g;
						bmax = b;
						max = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmax;
			*pout++ = gmax;
			*pout++ = bmax;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx * 3];

		for(imgX = endx; imgX < w; ++imgX)
		{
			long rmax = 0;
			long gmax = 0;
			long bmax = 0;
			long max = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue24(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					if(r + b + g > max)
					{
						rmax = r;
						gmax = g;
						bmax = b;
						max = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmax;
			*pout++ = gmax;
			*pout++ = bmax;
		}
	}
}

void msaFilters::Erode24(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_width / 2;
	int endx = w - startx - 1;
	int starty = m_height / 2;
	int endy = h - starty - 1;

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmin = 255;
			long gmin = 255;
			long bmin = 255;
			long min = 255 * 3;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 3];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					if(r + b + g < min)
					{
						rmin = r;
						gmin = g;
						bmin = b;
						min = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmin;
			*pout++ = gmin;
			*pout++ = bmin;
		}
	}

	// now deal with the edges
	// now deal with the edges

	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmin = 255;
			long gmin = 255;
			long bmin = 255;
			long min = 255 * 3;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue24(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					if(r + b + g < min)
					{
						rmin = r;
						gmin = g;
						bmin = b;
						min = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmin;
			*pout++ = gmin;
			*pout++ = bmin;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmin = 255;
			long gmin = 255;
			long bmin = 255;
			long min = 255 * 3;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue24(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					if(r + b + g < min)
					{
						rmin = r;
						gmin = g;
						bmin = b;
						min = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmin;
			*pout++ = gmin;
			*pout++ = bmin;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			long rmin = 255;
			long gmin = 255;
			long bmin = 255;
			long min = 255 * 3;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue24(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					if(r + b + g < min)
					{
						rmin = r;
						gmin = g;
						bmin = b;
						min = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmin;
			*pout++ = gmin;
			*pout++ = bmin;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx * 3];

		for(imgX = endx; imgX < w; ++imgX)
		{
			long rmin = 255;
			long gmin = 255;
			long bmin = 255;
			long min = 255 * 3;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue24(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					if(r + b + g < min)
					{
						rmin = r;
						gmin = g;
						bmin = b;
						min = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmin;
			*pout++ = gmin;
			*pout++ = bmin;
		}
	}
}


void msaFilters::Filter24(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_cx;
	int endx = w - startx - 1;
	int starty = m_cy;
	int endy = h - starty - 1;

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rsum = m_divisor / 2;	// for rounding purposes
			long gsum = m_divisor / 2;
			long bsum = m_divisor / 2;

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 3];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					rsum += *pin++ * m_values[filtVal];
					gsum += *pin++ * m_values[filtVal];
					bsum += *pin++ * m_values[filtVal++];
				}
			}

			rsum /= m_divisor;
			gsum /= m_divisor;
			bsum /= m_divisor;

			if(rsum > 255) rsum = 255;
			if(gsum > 255) gsum = 255;
			if(bsum > 255) bsum = 255;
			if(rsum < 0) rsum = 0;
			if(gsum < 0) gsum = 0;
			if(bsum < 0) bsum = 0;

			// set value and increment output pointer
			*pout++ = rsum;
			*pout++ = gsum;
			*pout++ = bsum;
		}
	}

	// now deal with the edges

	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rsum = m_divisor / 2;	// for rounding purposes
			long gsum = m_divisor / 2;
			long bsum = m_divisor / 2;

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// get a pointer to the input line; this will be clipped verticall, but not horizontally
				unsigned char *pin = GetClippedValue24(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					rsum += *pin++ * m_values[filtVal];
					gsum += *pin++ * m_values[filtVal];
					bsum += *pin++ * m_values[filtVal++];
				}
			}

			rsum /= m_divisor;
			gsum /= m_divisor;
			bsum /= m_divisor;

			if(rsum > 255) rsum = 255;
			if(gsum > 255) gsum = 255;
			if(bsum > 255) bsum = 255;
			if(rsum < 0) rsum = 0;
			if(gsum < 0) gsum = 0;
			if(bsum < 0) bsum = 0;

			// set value and increment output pointer
			*pout++ = rsum;
			*pout++ = gsum;
			*pout++ = bsum;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 3];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rsum = m_divisor / 2;	// for rounding purposes
			long gsum = m_divisor / 2;
			long bsum = m_divisor / 2;

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// get a pointer to the input line; this will be clipped verticall, but not horizontally
				unsigned char *pin = GetClippedValue24(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					rsum += *pin++ * m_values[filtVal];
					gsum += *pin++ * m_values[filtVal];
					bsum += *pin++ * m_values[filtVal++];
				}
			}

			rsum /= m_divisor;
			gsum /= m_divisor;
			bsum /= m_divisor;

			if(rsum > 255) rsum = 255;
			if(gsum > 255) gsum = 255;
			if(bsum > 255) bsum = 255;
			if(rsum < 0) rsum = 0;
			if(gsum < 0) gsum = 0;
			if(bsum < 0) bsum = 0;

			// set value and increment output pointer
			*pout++ = rsum;
			*pout++ = gsum;
			*pout++ = bsum;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			long rsum = m_divisor / 2;	// for rounding purposes
			long gsum = m_divisor / 2;
			long bsum = m_divisor / 2;

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue24(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);

					rsum += *pin++ * m_values[filtVal];
					gsum += *pin++ * m_values[filtVal];
					bsum += *pin++ * m_values[filtVal++];
				}
			}

			rsum /= m_divisor;
			gsum /= m_divisor;
			bsum /= m_divisor;

			if(rsum > 255) rsum = 255;
			if(gsum > 255) gsum = 255;
			if(bsum > 255) bsum = 255;
			if(rsum < 0) rsum = 0;
			if(gsum < 0) gsum = 0;
			if(bsum < 0) bsum = 0;

			// set value and increment output pointer
			*pout++ = rsum;
			*pout++ = gsum;
			*pout++ = bsum;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx * 3];

		for(imgX = endx; imgX < w; ++imgX)
		{
			long rsum = m_divisor / 2;	// for rounding purposes
			long gsum = m_divisor / 2;
			long bsum = m_divisor / 2;

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue24(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);

					rsum += *pin++ * m_values[filtVal];
					gsum += *pin++ * m_values[filtVal];
					bsum += *pin++ * m_values[filtVal++];
				}
			}

			rsum /= m_divisor;
			gsum /= m_divisor;
			bsum /= m_divisor;

			if(rsum > 255) rsum = 255;
			if(gsum > 255) gsum = 255;
			if(bsum > 255) bsum = 255;
			if(rsum < 0) rsum = 0;
			if(gsum < 0) gsum = 0;
			if(bsum < 0) bsum = 0;

			// set value and increment output pointer
			*pout++ = rsum;
			*pout++ = gsum;
			*pout++ = bsum;
		}
	}
}

/*
	For color values; since we're going to sort by gray values (the only way to really do a median filter) then
	we need to have some way to get from the median value back to the source RGB value.
*/
void msaFilters::MedianFilter32(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_width / 2;
	int endx = w - startx - 1;
	int starty = m_height / 2;
	int endy = h - starty - 1;	

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)		
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long colors[256] = {0};
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 4];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;

					int gray = RGBtoGray(r, g, b);
					++count[gray];

					// create an RGB int to store so we can obtain the median color later
					int rgb = (((int)r) << 16) + (((int)g) << 8) + (int)b;
					if(colors[gray] != 0)
					{
						if(rgb > colors[gray]) colors[gray] = rgb;
					}
					else
						colors[gray] = rgb;
					
				}
			}

			// find median value
			int total = m_count / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = colors[i] >> 16;
			*pout++ = (colors[i] >> 8) & 0xff;
			*pout++ = colors[i] & 0xff;
			*pout++ = alpha;
		}
	}

	// now deal with the edges
	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long colors[256] = {0};
			long totalVals = 0;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if(imgY - starty + filtY < 0) continue;

				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 4];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					++totalVals;
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;

					int gray = RGBtoGray(r, g, b);
					++count[gray];

					// create an RGB int to store so we can obtain the median color later
					int rgb = (((int)r) << 16) + (((int)g) << 8) + (int)b;
					if(colors[gray] != 0)
					{
						if(rgb > colors[gray]) colors[gray] = rgb;
					}
					else
						colors[gray] = rgb;
					
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = colors[i] >> 16;
			*pout++ = (colors[i] >> 8) & 0xff;
			*pout++ = colors[i] & 0xff;
			*pout++ = alpha;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long colors[256] = {0};
			long totalVals = 0;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if(imgY - starty + filtY >= h) continue;

				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 4];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					++totalVals;
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;

					int gray = RGBtoGray(r, g, b);
					++count[gray];

					// create an RGB int to store so we can obtain the median color later
					int rgb = (((int)r) << 16) + (((int)g) << 8) + (int)b;
					if(colors[gray] != 0)
					{
						if(rgb > colors[gray]) colors[gray] = rgb;
					}
					else
						colors[gray] = rgb;
					
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = colors[i] >> 16;
			*pout++ = (colors[i] >> 8) & 0xff;
			*pout++ = colors[i] & 0xff;
			*pout++ = alpha;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long colors[256] = {0};
			long totalVals = 0;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if((imgY - starty + filtY >= h) || (imgY - starty + filtY < 0)) continue;

				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// skip the column if it takes us out of bounds
					if(imgX - startx + filtX < 0) continue;

					unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx + filtX) * 4];

					++totalVals;
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;

					int gray = RGBtoGray(r, g, b);
					++count[gray];

					// create an RGB int to store so we can obtain the median color later
					int rgb = (((int)r) << 16) + (((int)g) << 8) + (int)b;
					if(colors[gray] != 0)
					{
						if(rgb > colors[gray]) colors[gray] = rgb;
					}
					else
						colors[gray] = rgb;
					
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = colors[i] >> 16;
			*pout++ = (colors[i] >> 8) & 0xff;
			*pout++ = colors[i] & 0xff;
			*pout++ = alpha;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx * 4];

		for(imgX = endx; imgX < w; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long colors[256] = {0};
			long totalVals = 0;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];
			
			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if((imgY - starty + filtY >= h) || (imgY - starty + filtY < 0)) continue;

				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// skip the column if it takes us out of bounds
					if(imgX - startx + filtX >= w) continue;

					unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx + filtX) * 4];

					++totalVals;
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;

					int gray = RGBtoGray(r, g, b);
					++count[gray];

					// create an RGB int to store so we can obtain the median color later
					int rgb = (((int)r) << 16) + (((int)g) << 8) + (int)b;
					if(colors[gray] != 0)
					{
						if(rgb > colors[gray]) colors[gray] = rgb;
					}
					else
						colors[gray] = rgb;
					
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = colors[i] >> 16;
			*pout++ = (colors[i] >> 8) & 0xff;
			*pout++ = colors[i] & 0xff;
			*pout++ = alpha;
		}
	}
}

void msaFilters::Dilate32(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_width / 2;
	int endx = w - startx - 1;
	int starty = m_height / 2;
	int endy = h - starty - 1;

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmax = 0;
			long gmax = 0;
			long bmax = 0;
			long max = 0;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 4];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;
					if(r + b + g > max)
					{
						rmax = r;
						gmax = g;
						bmax = b;
						max = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmax;
			*pout++ = gmax;
			*pout++ = bmax;
			*pout++ = alpha;
		}
	}

	// now deal with the edges

	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmax = 0;
			long gmax = 0;
			long bmax = 0;
			long max = 0;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue32(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;
					if(r + b + g > max)
					{
						rmax = r;
						gmax = g;
						bmax = b;
						max = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmax;
			*pout++ = gmax;
			*pout++ = bmax;
			*pout++ = alpha;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmax = 0;
			long gmax = 0;
			long bmax = 0;
			long max = 0;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue32(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;
					if(r + b + g > max)
					{
						rmax = r;
						gmax = g;
						bmax = b;
						max = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmax;
			*pout++ = gmax;
			*pout++ = bmax;
			*pout++ = alpha;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			long rmax = 0;
			long gmax = 0;
			long bmax = 0;
			long max = 0;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue32(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					pin++;
					if(r + b + g > max)
					{
						rmax = r;
						gmax = g;
						bmax = b;
						max = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmax;
			*pout++ = gmax;
			*pout++ = bmax;
			*pout++ = alpha;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx * 4];

		for(imgX = endx; imgX < w; ++imgX)
		{
			long rmax = 0;
			long gmax = 0;
			long bmax = 0;
			long max = 0;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue32(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;
					if(r + b + g > max)
					{
						rmax = r;
						gmax = g;
						bmax = b;
						max = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmax;
			*pout++ = gmax;
			*pout++ = bmax;
			*pout++ = alpha;
		}
	}
}

void msaFilters::Erode32(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_width / 2;
	int endx = w - startx - 1;
	int starty = m_height / 2;
	int endy = h - starty - 1;

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmin = 255;
			long gmin = 255;
			long bmin = 255;
			long min = 255 * 3;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 4];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;
					if(r + b + g < min)
					{
						rmin = r;
						gmin = g;
						bmin = b;
						min = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmin;
			*pout++ = gmin;
			*pout++ = bmin;
			*pout++ = alpha;
		}
	}

	// now deal with the edges

	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmin = 255;
			long gmin = 255;
			long bmin = 255;
			long min = 255 * 3;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue32(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;
					if(r + b + g < min)
					{
						rmin = r;
						gmin = g;
						bmin = b;
						min = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmin;
			*pout++ = gmin;
			*pout++ = bmin;
			*pout++ = alpha;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rmin = 255;
			long gmin = 255;
			long bmin = 255;
			long min = 255 * 3;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue32(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;
					if(r + b + g < min)
					{
						rmin = r;
						gmin = g;
						bmin = b;
						min = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmin;
			*pout++ = gmin;
			*pout++ = bmin;
			*pout++ = alpha;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			long rmin = 255;
			long gmin = 255;
			long bmin = 255;
			long min = 255 * 3;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue32(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;
					if(r + b + g < min)
					{
						rmin = r;
						gmin = g;
						bmin = b;
						min = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmin;
			*pout++ = gmin;
			*pout++ = bmin;
			*pout++ = alpha;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx * 4];

		for(imgX = endx; imgX < w; ++imgX)
		{
			long rmin = 255;
			long gmin = 255;
			long bmin = 255;
			long min = 255 * 3;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue32(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char r = *pin++;
					unsigned char g = *pin++;
					unsigned char b = *pin++;
					++pin;
					if(r + b + g < min)
					{
						rmin = r;
						gmin = g;
						bmin = b;
						min = r + g + b;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = rmin;
			*pout++ = gmin;
			*pout++ = bmin;
			*pout++ = alpha;
		}
	}
}


void msaFilters::Filter32(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_cx;
	int endx = w - startx - 1;
	int starty = m_cy;
	int endy = h - starty - 1;

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rsum = m_divisor / 2;	// for rounding purposes
			long gsum = m_divisor / 2;
			long bsum = m_divisor / 2;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx) * 4];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					rsum += *pin++ * m_values[filtVal];
					gsum += *pin++ * m_values[filtVal];
					bsum += *pin++ * m_values[filtVal++];
					++pin;
				}
			}

			rsum /= m_divisor;
			gsum /= m_divisor;
			bsum /= m_divisor;

			if(rsum > 255) rsum = 255;
			if(gsum > 255) gsum = 255;
			if(bsum > 255) bsum = 255;
			if(rsum < 0) rsum = 0;
			if(gsum < 0) gsum = 0;
			if(bsum < 0) bsum = 0;

			// set value and increment output pointer
			*pout++ = rsum;
			*pout++ = gsum;
			*pout++ = bsum;
			*pout++ = alpha;
		}
	}

	// now deal with the edges

	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rsum = m_divisor / 2;	// for rounding purposes
			long gsum = m_divisor / 2;
			long bsum = m_divisor / 2;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// get a pointer to the input line; this will be clipped verticall, but not horizontally
				unsigned char *pin = GetClippedValue32(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					rsum += *pin++ * m_values[filtVal];
					gsum += *pin++ * m_values[filtVal];
					bsum += *pin++ * m_values[filtVal++];
					++pin;
				}
			}

			rsum /= m_divisor;
			gsum /= m_divisor;
			bsum /= m_divisor;

			if(rsum > 255) rsum = 255;
			if(gsum > 255) gsum = 255;
			if(bsum > 255) bsum = 255;
			if(rsum < 0) rsum = 0;
			if(gsum < 0) gsum = 0;
			if(bsum < 0) bsum = 0;

			// set value and increment output pointer
			*pout++ = rsum;
			*pout++ = gsum;
			*pout++ = bsum;
			*pout++ = alpha;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx * 4];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long rsum = m_divisor / 2;	// for rounding purposes
			long gsum = m_divisor / 2;
			long bsum = m_divisor / 2;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// get a pointer to the input line; this will be clipped verticall, but not horizontally
				unsigned char *pin = GetClippedValue32(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					rsum += *pin++ * m_values[filtVal];
					gsum += *pin++ * m_values[filtVal];
					bsum += *pin++ * m_values[filtVal++];
					++pin;
				}
			}

			rsum /= m_divisor;
			gsum /= m_divisor;
			bsum /= m_divisor;

			if(rsum > 255) rsum = 255;
			if(gsum > 255) gsum = 255;
			if(bsum > 255) bsum = 255;
			if(rsum < 0) rsum = 0;
			if(gsum < 0) gsum = 0;
			if(bsum < 0) bsum = 0;

			// set value and increment output pointer
			*pout++ = rsum;
			*pout++ = gsum;
			*pout++ = bsum;
			*pout++ = alpha;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			long rsum = m_divisor / 2;	// for rounding purposes
			long gsum = m_divisor / 2;
			long bsum = m_divisor / 2;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue32(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);

					rsum += *pin++ * m_values[filtVal];
					gsum += *pin++ * m_values[filtVal];
					bsum += *pin++ * m_values[filtVal++];
					++pin;
				}
			}

			rsum /= m_divisor;
			gsum /= m_divisor;
			bsum /= m_divisor;

			if(rsum > 255) rsum = 255;
			if(gsum > 255) gsum = 255;
			if(bsum > 255) bsum = 255;
			if(rsum < 0) rsum = 0;
			if(gsum < 0) gsum = 0;
			if(bsum < 0) bsum = 0;

			// set value and increment output pointer
			*pout++ = rsum;
			*pout++ = gsum;
			*pout++ = bsum;
			*pout++ = alpha;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx * 4];

		for(imgX = endx; imgX < w; ++imgX)
		{
			long rsum = m_divisor / 2;	// for rounding purposes
			long gsum = m_divisor / 2;
			long bsum = m_divisor / 2;
			unsigned char alpha = input[imgY * bpl + imgX * 4 + 3];

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue32(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);

					rsum += *pin++ * m_values[filtVal];
					gsum += *pin++ * m_values[filtVal];
					bsum += *pin++ * m_values[filtVal++];
					++pin;
				}
			}

			rsum /= m_divisor;
			gsum /= m_divisor;
			bsum /= m_divisor;

			if(rsum > 255) rsum = 255;
			if(gsum > 255) gsum = 255;
			if(bsum > 255) bsum = 255;
			if(rsum < 0) rsum = 0;
			if(gsum < 0) gsum = 0;
			if(bsum < 0) bsum = 0;

			// set value and increment output pointer
			*pout++ = rsum;
			*pout++ = gsum;
			*pout++ = bsum;
			*pout++ = alpha;
		}
	}
}

void msaFilters::MedianFilter8(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_width / 2;
	int endx = w - startx - 1;
	int starty = m_height / 2;
	int endy = h - starty - 1;	

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)		
		{
			// set and zero out array containing the count of values
			long count[256] = {0};

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx)];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					++count[*pin++];
				}
			}

			// find median value
			int total = m_count / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = i;
		}
	}

	// now deal with the edges
	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long totalVals = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if(imgY - starty + filtY < 0) continue;

				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx)];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					++totalVals;
					++count[*pin++];
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = i;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long totalVals = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if(imgY - starty + filtY >= h) continue;

				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx)];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					++totalVals;
					++count[*pin++];
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = i;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long totalVals = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if((imgY - starty + filtY >= h) || (imgY - starty + filtY < 0)) continue;

				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// skip the column if it takes us out of bounds
					if(imgX - startx + filtX < 0) continue;

					unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx + filtX)];

					++totalVals;
					++count[*pin];
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = i;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx];

		for(imgX = endx; imgX < w; ++imgX)
		{
			// set and zero out array containing the count of values
			long count[256] = {0};
			long totalVals = 0;
			
			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// skip the row if it takes us out of bounds
				if((imgY - starty + filtY >= h) || (imgY - starty + filtY < 0)) continue;

				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// skip the column if it takes us out of bounds
					if(imgX - startx + filtX >= w) continue;

					unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx + filtX)];

					++totalVals;
					++count[*pin];
				}
			}

			// find median value
			int total = totalVals / 2;
			int i;
			for(i = 255; i >= 0; --i)
			{
				total -= count[i];
				if(total < 0)
					break;
			}			

			// set value and increment output pointer
			*pout++ = i;
		}
	}
}

void msaFilters::Dilate8(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_width / 2;
	int endx = w - startx - 1;
	int starty = m_height / 2;
	int endy = h - starty - 1;

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long max = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx)];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char g = *pin++;
					if(g > max)
						max = g;
				}
			}

			// set value and increment output pointer
			*pout++ = max;
		}
	}

	// now deal with the edges

	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long max = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue8(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char g = *pin++;
					if( g > max)
						max = g;
				}
			}

			// set value and increment output pointer
			*pout++ = max;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long max = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue8(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char g = *pin++;
					if(g > max)
						max = g;
				}
			}

			// set value and increment output pointer
			*pout++ = max;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			long max = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue8(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char g = *pin;
					if(g > max)
						max = g;
				}
			}

			// set value and increment output pointer
			*pout++ = max;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx];

		for(imgX = endx; imgX < w; ++imgX)
		{
			long max = 0;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue8(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char g = *pin;
					if(g > max)
						max = g;
				}
			}

			// set value and increment output pointer
			*pout++ = max;
		}
	}
}

void msaFilters::Erode8(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_width / 2;
	int endx = w - startx - 1;
	int starty = m_height / 2;
	int endy = h - starty - 1;

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long min = 255;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx)];
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char g = *pin++;
					if(g < min)
					{
						min = g;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = min;
		}
	}

	// now deal with the edges
	// now deal with the edges

	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long min = 255;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue8(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char g = *pin;
					if(g < min)
						min = g;
				}
			}

			// set value and increment output pointer
			*pout++ = min;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long min = 255;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = GetClippedValue8(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					unsigned char g = *pin++;
					if(g < min)
					{
						min = g;
					}
				}
			}

			// set value and increment output pointer
			*pout++ = min;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			long min = 255;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue8(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char g = *pin;
					if(g < min)
						min = g;
				}
			}

			// set value and increment output pointer
			*pout++ = min;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx];

		for(imgX = endx; imgX < w; ++imgX)
		{
			long min = 255;

			int filtX, filtY;
			for(filtY = 0; filtY < m_height; ++filtY)
			{				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue8(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);
					
					unsigned char g = *pin++;
					if(g < min)
						min = g;
				}
			}

			// set value and increment output pointer
			*pout++ = min;
		}
	}
}


void msaFilters::Filter8(unsigned char *input, unsigned char *output, int w, int h, int bpl)
{
	int imgX, imgY;

	// do the easy cases first, where the filter doesn't overlap the edge of the data
	int startx = m_cx;
	int endx = w - startx - 1;
	int starty = m_cy;
	int endy = h - starty - 1;

	for(imgY = starty; imgY < endy; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long sum = m_divisor / 2;	// for rounding purposes

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// start input pointer at upper left of filter window and work down
				// want to start at upper left of window
				unsigned char *pin = &input[(imgY - starty + filtY) * bpl + (imgX - startx)];
				
				for(filtX = 0; filtX < m_width; ++filtX)
					sum += *pin++ * m_values[filtVal++];
			}

			sum /= m_divisor;

			if(sum > 255) sum = 255;
			if(sum < 0) sum = 0;

			// set value and increment output pointer
			*pout++ = sum;
		}
	}

	// now deal with the edges

	// top edge
	for(imgY = 0; imgY < starty; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long sum = m_divisor / 2;	// for rounding purposes

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// get a pointer to the input line; this will be clipped verticall, but not horizontally
				unsigned char *pin = GetClippedValue8(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					sum += *pin++ * m_values[filtVal++];
				}
			}

			sum /= m_divisor;

			if(sum > 255) sum = 255;
			if(sum < 0) sum = 0;

			// set value and increment output pointer
			*pout++ = sum;
		}
	}

	// bottom edge
	for(imgY = endy; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + startx];

		for(imgX = startx; imgX < endx; ++imgX)
		{
			long sum = m_divisor / 2;	// for rounding purposes

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				// get a pointer to the input line; this will be clipped verticall, but not horizontally
				unsigned char *pin = GetClippedValue8(imgX - startx, imgY - starty + filtY, input, w, h, bpl);
				
				for(filtX = 0; filtX < m_width; ++filtX)
					sum += *pin++ * m_values[filtVal++];
			}

			sum /= m_divisor;

			if(sum > 255) sum = 255;
			if(sum < 0) sum = 0;

			// set value and increment output pointer
			*pout++ = sum;
		}
	}

	// left edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl];

		for(imgX = 0; imgX < startx; ++imgX)
		{
			long sum = m_divisor / 2;	// for rounding purposes

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue8(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);

					sum += *pin * m_values[filtVal++];
				}
			}

			sum /= m_divisor;

			if(sum > 255) sum = 255;
			if(sum < 0) sum = 0;

			// set value and increment output pointer
			*pout++ = sum;
		}
	}

	// right edge
	for(imgY = 0; imgY < h; ++imgY)
	{		
		unsigned char *pout = &output[imgY * bpl + endx];

		for(imgX = endx; imgX < w; ++imgX)
		{
			long sum = m_divisor / 2;	// for rounding purposes

			int filtX, filtY;
			int filtVal = 0;	// use index so we don't need to calculate
			for(filtY = 0; filtY < m_height; ++filtY)
			{
				
				for(filtX = 0; filtX < m_width; ++filtX)
				{
					// get a pointer for each pixel; since it'll clip vertically AND horizontally, can't use lines
					unsigned char *pin = GetClippedValue8(imgX - startx + filtX, imgY - starty + filtY, input, w, h, bpl);

					sum += *pin * m_values[filtVal++];
				}
			}

			sum /= m_divisor;

			if(sum > 255) sum = 255;
			if(sum < 0) sum = 0;

			// set value and increment output pointer
			*pout++ = sum;
		}
	}
}

