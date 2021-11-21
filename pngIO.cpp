#include <cstdlib>
#include <cstdio>
#include <png.h>

void WritePNG(const char *filename, size_t width, size_t height, size_t bpl, 
	size_t dpi, size_t depth, unsigned char *data) 
{
	int png_type;

	switch(depth)
	{
		case 8:
			png_type = PNG_COLOR_TYPE_GRAY;
			break;
		case 24:
			png_type = PNG_COLOR_TYPE_RGB;
			break;
		case 32:
			png_type = PNG_COLOR_TYPE_RGBA;
			break;
		default:
			throw "Unsupported bit depth";
	}

	FILE *fp = fopen(filename, "wb");
	if(!fp) throw "Could not open output file for writing";

	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 
					NULL, NULL, NULL);
	if(!png) throw "Could not create PNG write structure";

	png_infop info = png_create_info_struct(png);
	if(!info) throw "Could not create PNG info structure";

	png_set_pHYs(png, info, dpi * 39.37, dpi * 39.37, PNG_RESOLUTION_METER);
	
	if(setjmp(png_jmpbuf(png))) throw "Could not setjmp";

	png_init_io(png, fp);

	// Output is 8bit depth, RGBA format.
	png_set_IHDR(png, info, width, height, 8,
		png_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	png_write_info(png, info);

	// set up array of png_bytep pointing to the first byte of each scanline
	png_bytep *row_pointers = new png_bytep[height];
	for(size_t y = 0; y < height; ++y)
		row_pointers[y] = (png_bytep)&data[y * bpl];

	png_write_image(png, row_pointers);

	png_write_end(png, NULL);

	delete[] row_pointers;

	fclose(fp);

	png_destroy_write_struct(&png, &info);
}

void ReadPNG(const char *filename, size_t &width, size_t &height, size_t &bpl, 
		size_t &dpi, size_t &depth, unsigned char **data) 
{
  FILE *fp = fopen(filename, "rb");

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png) throw "Cannot create read struct";

  png_infop info = png_create_info_struct(png);
  if(!info) throw "Cannot create info struct";

  if(setjmp(png_jmpbuf(png))) throw "Cannot setjmp";

  png_init_io(png, fp);

  png_read_info(png, info);

  width = png_get_image_width(png, info);
  height = png_get_image_height(png, info);
  png_byte color_type = png_get_color_type(png, info);
  depth = png_get_bit_depth(png, info);
  dpi = png_get_pixels_per_inch(png, info);

  // convert input into RGBA, see http://www.libpng.org/pub/png/libpng-manual.txt
  if(depth == 16)
    png_set_strip_16(png);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

  if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

  // fill the alpha channel with 0xff for full transparency
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

  png_read_update_info(png, info);

  // at this point, the image will have been converted to 32 bit RGBA
  depth = 32;

  // round to 4 bytes
  bpl = (depth * width / 8 + 3) / 4 * 4;
  *data = new unsigned char[bpl * height];

  png_bytep *row_pointers = new png_bytep[sizeof(png_bytep) * height];
  for(size_t y = 0; y < height; y++) 
    row_pointers[y] = &(*data)[y * bpl];

  png_read_image(png, row_pointers);

  fclose(fp);

  png_destroy_read_struct(&png, &info, NULL);

  delete[] row_pointers;
}
