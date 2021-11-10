
void ReadPNG(const char *filename, int &width, int &height, int &bpl, int &dpi, int &depth, 
		unsigned char **data);
void WritePNG(const char *filename, int width, int height, int bpl, int dpi, int depth, 
		unsigned char *data);

