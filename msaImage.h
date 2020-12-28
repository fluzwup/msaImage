#include "msaAffine.h"

class msaPixel
{
public:
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

// depths 1 for bitonal, 8 for grayscale, 24 for RGB, 32 for RGBA
class msaImage
{
protected:
	int width;
	int height;
	int bytesPerLine;
	unsigned char *data;
	int depth;
	bool ownsData;

public:
	msaImage();
	~msaImage();

	// accessor functions
	bool OwnsData();
	int Width();
	int Height();
	int Depth();
	int BytesPerLine();
	unsigned char *Data();
	
	// create a blank image
	void CreateImage(int width, int height, int depth);
	// create solid color image
	void CreateImage(int width, int height, int depth, const msaPixel &fill);

	// point to data in an external buffer, don't own the buffer
	void UseExternalData(int width, int height, int bytesPerLine, int depth, unsigned char *data);
	// point to data in an external buffer, do own the buffer (buffer must be allocated with new[]
	void TakeExternalData(int width, int height, int bytesPerLine, int depth, unsigned char *data);
	// copy data in from an external buffer
	void SetCopyData(int width, int height, int bytesPerLine, int depth, unsigned char *data);

	void TransformImage(msaAffineTransform &trans, msaImage &output, int quality);

	// going from 8 bit to 24 or 32 bit, use color as white point, and scale accordingly
	// going from to 32 bit, copy alpha channel from color to whole image
	// going from color to 8 bit, use color as relative brightness of each color component
	void SimpleConvert(int newDepth, msaPixel &color, msaImage &output);

	// gray to 24 bit color conversion with 256 element array of pixels, to do false color mapping
	void ColorMap(msaPixel map[256], msaImage &output);

	// remap brightness of a gray image
	void RemapBrightness(unsigned char map[256], msaImage &output);

	// compositing function to add 8 bit alpha channel to 24 bit to make 32 bit
	void AddAlphaChannel(msaImage &alpha, msaImage &output);

	// compositing functions add 3 or 4 8 bit images to make a 24 or 32 bit image
	void CompositeRGB(msaImage &red, msaImage &green, msaImage &blue);
	void CompositeRGBA(msaImage &red, msaImage &green, msaImage &blue, msaImage &alpha);

	// TODO:  Add splitting function to break 24 and 32 bit images down into RGB/RGBA planes
	void SplitRGB(msaImage &red, msaImage &green, msaImage &blue);
	void SplitRGBA(msaImage &red, msaImage &green, msaImage &blue, msaImage &alpha);

	// TODO:  Add splitting function to break 24 and 32 bit images down into HSV/HSVA planes
	void SplitHSV(msaImage &hue, msaImage &saturation, msaImage &volume);
	void SplitHSVA(msaImage &hue, msaImage &saturation, msaImage &volume, msaImage &alpha);

	// TODO:  Add image combination functions:  min, max, sum, difference, multiply, divide
	void MinImages(msaImage &input, msaImage &output);
	void MaxImages(msaImage &input, msaImage &output);
	void SumImages(msaImage &input, msaImage &output);
	void DiffImages(msaImage &input, msaImage &output);
	void MultiplyImages(msaImage &input, msaImage &output);
	void DivideImages(msaImage &input, msaImage &output);

protected:
	// apply a transform to the given data type
	unsigned char *transformFast32(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input);
	unsigned char *transformBetter32(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input);
	unsigned char *transformBest32(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input);

	unsigned char *transformFast24(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input);
	unsigned char *transformBetter24(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input);
	unsigned char *transformBest24(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input);
	
	unsigned char *transformFast8(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input);
	unsigned char *transformBetter8(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input);
	unsigned char *transformBest8(msaAffineTransform &transform, int &width, int &height, int &bpl, unsigned char *input);
};

