
void ReadPNG(const char *filename, size_t &width, size_t &height, size_t &bpl, 
				size_t &dpi, size_t &depth, unsigned char **data);
void WritePNG(const char *filename, size_t width, size_t height, size_t bpl, 
				size_t dpi, size_t depth, unsigned char *data);

