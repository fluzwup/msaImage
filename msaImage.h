#ifndef _msaImage_included
#define _msaImage_included
#include <vector>
#include <list>
#include "msaAffine.h"

// alpha 0 is transparent, 255 is opaque
// if pixel is interpreted as gray, r will be used for intensity
class msaPixel
{
public:
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
	inline bool operator==(const msaPixel &rhs)
	{
		if(r != rhs.r) return false;
		if(g != rhs.g) return false;
		if(b != rhs.b) return false;
		if(a != rhs.a) return false;
		return true;
	};
};

// depths 8 for grayscale, 24 for RGB, 32 for RGBA
class msaImage
{
protected:
	size_t width;
	size_t height;
	size_t bytesPerLine;
	unsigned char *data;
	size_t depth;
	bool ownsData;

	void MoveData(msaImage &other);

public:
	msaImage();
	~msaImage();

	// accessor functions
	bool OwnsData();
	size_t Width();
	size_t Height();
	size_t Depth();
	size_t BytesPerLine();
	unsigned char *Data();

	// access to raw data
	inline unsigned char *GetRawLine(size_t y) { return &data[bytesPerLine * y]; };
	inline unsigned char *GetRawPixel(size_t x, size_t y) 
	{ 
			return &data[bytesPerLine * y + x * depth / 8]; 
	};

	// access msaPixel access
	inline msaPixel GetPixel(size_t x, size_t y)
	{
		msaPixel p;
		unsigned char *raw = GetRawPixel(x, y);
		switch(depth)
		{
			case 8:
				p.r = *raw;
				p.g = p.r;
				p.b = p.r;
				p.a = 255;
				break;
			case 24:
				p.r = *raw++;
				p.g = *raw++;
				p.b = *raw;
				p.a = 255;
				break;
			case 32:
				p.r = *raw++;
				p.g = *raw++;
				p.b = *raw++;
				p.a = *raw;
				break;
			default:
				throw "Invalid depth";
		}
		return p;
	};

	inline void SetPixel(size_t x, size_t y, msaPixel p)
	{
		unsigned char *raw = GetRawPixel(x, y);
		switch(depth)
		{
			case 8:
				*raw = p.r;
				break;
			case 24:
				*raw++ = p.r;
				*raw++ = p.g;
				*raw = p.b;
				break;
			case 32:
				*raw++ = p.r;
				*raw++ = p.g;
				*raw++ = p.b;
				*raw = p.a;
				break;
			default:
				throw "Invalid depth";
		}
	};
	
	// create a blank image
	void CreateImage(size_t width, size_t height, size_t depth);
	// create solid color image
	void CreateImage(size_t width, size_t height, size_t depth, const msaPixel &fill);

	// point to data in an external buffer, don't own the buffer
	void UseExternalData(size_t width, size_t height, size_t bytesPerLine, size_t depth, unsigned char *data);
	// point to data in an external buffer, do own the buffer (buffer must be allocated with new[]
	void TakeExternalData(size_t width, size_t height, size_t bytesPerLine, size_t depth, unsigned char *data);
	// copy data in from an external buffer
	void SetCopyData(size_t width, size_t height, size_t bytesPerLine, size_t depth, unsigned char *data);

	void TransformImage(msaAffineTransform &trans, msaImage &output, size_t quality);

	// going from 8 bit to 24 or 32 bit, use color as white point, and scale accordingly
	// going from to 32 bit, copy alpha channel from color to whole image
	// going from color to 8 bit, use color as relative brightness of each color component
	void SimpleConvert(size_t newDepth, msaPixel &color, msaImage &output);

	// gray to 24 bit color conversion with 256 element array of pixels, to do false color mapping
	void ColorMap(msaPixel map[256], msaImage &output);

	// remap brightness of a gray image
	void RemapBrightness(unsigned char map[256]) { RemapBrightness(map, *this); };
	void RemapBrightness(unsigned char map[256], msaImage &output);

	// compositing function to add 8 bit alpha channel to 24 bit to make 32 bit
	void AddAlphaChannel(msaImage &alpha);
	void AddAlphaChannel(msaImage &alpha, msaImage &output);

	// compositing functions add 3 or 4 8 bit images to make a 24 or 32 bit image
	void ComposeRGB(msaImage &red, msaImage &green, msaImage &blue);
	void ComposeRGBA(msaImage &red, msaImage &green, msaImage &blue, msaImage &alpha);

	// splitting functions to break 24 and 32 bit images down into RGB/RGBA planes
	void SplitRGB(msaImage &red, msaImage &green, msaImage &blue);
	void SplitRGBA(msaImage &red, msaImage &green, msaImage &blue, msaImage &alpha);

	// compositing functions add 3 or 4 8 bit images to make a 24 or 32 bit image
	void ComposeHSV(msaImage &hue, msaImage &sat, msaImage &vol);
	void ComposeHSVA(msaImage &hue, msaImage &sat, msaImage &vol, msaImage &alpha);

	// splitting functions to break 24 and 32 bit images down into HSV/HSVA planes
	void SplitHSV(msaImage &hue, msaImage &saturation, msaImage &volume);
	void SplitHSVA(msaImage &hue, msaImage &saturation, msaImage &volume, msaImage &alpha);

	// image combination functions
	void MinImages(msaImage &input) { MinImages(input, *this); };
	void MinImages(msaImage &input, msaImage &output);
	void MaxImages(msaImage &input) { MaxImages(input, *this); };
	void MaxImages(msaImage &input, msaImage &output);
	void SumImages(msaImage &input) { SumImages(input, *this); };
	void SumImages(msaImage &input, msaImage &output);
	void DiffImages(msaImage &input) { DiffImages(input, *this); };
	void DiffImages(msaImage &input, msaImage &output);
	void MultiplyImages(msaImage &input) { MultiplyImages(input, *this); };
	void MultiplyImages(msaImage &input, msaImage &output);
	void DivideImages(msaImage &input) { DivideImages(input, *this); };
	void DivideImages(msaImage &input, msaImage &output);

	// simple overlay function; if images are 32 bit, then alpha channel will be used
	void OverlayImage(msaImage &overlay, size_t x, size_t y, size_t w, size_t h);

	// mask may be gray for alpha channel, or RGB for stained glass transparency
	// mask and overlay must be same size
	void OverlayImage(msaImage &overlay, msaImage &mask, size_t x, size_t y, size_t w, size_t h);

	// create a gray image from a set of runs, using given colors for background and foreground
	// runs start with background color
	void CreateImageFromRuns(std::vector< std::list <size_t> > &runs, size_t depth, msaPixel bg, msaPixel fg);
protected:
	// apply a transform to the given data type
	unsigned char *transformFast32(msaAffineTransform &transform, size_t &width, size_t &height, size_t &bpl);
	unsigned char *transformBetter32(msaAffineTransform &transform, size_t &width, size_t &height, size_t &bpl);
	unsigned char *transformBest32(msaAffineTransform &transform, size_t &width, size_t &height, size_t &bpl);

	unsigned char *transformFast24(msaAffineTransform &transform, size_t &width, size_t &height, size_t &bpl);
	unsigned char *transformBetter24(msaAffineTransform &transform, size_t &width, size_t &height, size_t &bpl);
	unsigned char *transformBest24(msaAffineTransform &transform, size_t &width, size_t &height, size_t &bpl);
	
	unsigned char *transformFast8(msaAffineTransform &transform, size_t &width, size_t &height, size_t &bpl);
	unsigned char *transformBetter8(msaAffineTransform &transform, size_t &width, size_t &height, size_t &bpl);
	unsigned char *transformBest8(msaAffineTransform &transform, size_t &width, size_t &height, size_t &bpl);
};
#endif

