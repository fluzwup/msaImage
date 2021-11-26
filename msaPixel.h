#ifndef _msaPixel_included
#define _msaPixel_included
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

#endif

