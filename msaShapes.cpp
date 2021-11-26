
void msaShapes::FillCircle(msaImage &img, float centerX, float centerY, float radius, msaPixel color)
{
	int top = round(centerY - radius);
	int bottom = round(centerY + radius);

	if(top < 0) top = 0;
	if(bottom >= h) bottom = h - 1;

	for(int y = top; y <= bottom; ++y)
	{
		float span = safeSqrt(radius * radius - (y  - centerY) * (y - centerY));
		int left = round(centerX - span);
		int count = round(span * 2.0);
		if(left < 0) left = 0;
		if(left + span >= w) span = w - 1 - left;
		if(span <= 0) continue;
		memset(pixels + y * bpl + left, color, count);
	}
}

