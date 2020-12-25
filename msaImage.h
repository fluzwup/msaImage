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

