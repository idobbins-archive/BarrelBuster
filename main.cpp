
#include <png.h>

#include <cmath>
#include <iostream>
#include <tuple>
#include <vector>

struct png_t {
  size_t width, height;
  uint8_t color_type;
  uint8_t bit_depth;
  std::vector<void *> row_pointers;
};

void load_png(const char *path, png_t *image) {

  FILE *f = fopen(path, "rb");

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info = png_create_info_struct(png);

  png_init_io(png, f);
  png_read_info(png, info);

  image->width = png_get_image_width(png, info);
  image->height = png_get_image_height(png, info);
  image->color_type = png_get_color_type(png, info);
  image->bit_depth = png_get_bit_depth(png, info);

  if (image->bit_depth == 16)
	png_set_strip_16(png);

  if (image->color_type == PNG_COLOR_TYPE_PALETTE)
	png_set_palette_to_rgb(png);

  if (image->color_type == PNG_COLOR_TYPE_GRAY && image->bit_depth < 8)
	png_set_expand_gray_1_2_4_to_8(png);

  if (png_get_valid(png, info, PNG_INFO_tRNS))
	png_set_tRNS_to_alpha(png);

  if (image->color_type == PNG_COLOR_TYPE_RGB || image->color_type == PNG_COLOR_TYPE_GRAY || image->color_type == PNG_COLOR_TYPE_PALETTE)
	png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  if (image->color_type == PNG_COLOR_TYPE_GRAY || image->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	png_set_gray_to_rgb(png);

  png_read_update_info(png, info);

  image->row_pointers.reserve(image->height);
  for (int i = 0; i < image->height; ++i) {
	image->row_pointers.push_back(malloc(png_get_rowbytes(png, info)));
  }

  png_read_image(png, (png_bytepp)image->row_pointers.data());

  fclose(f);

  png_destroy_read_struct(&png, &info, nullptr);
}

void free_png(const png_t &image) {
  for (auto i : image.row_pointers) {
	free(i);
  }
}

void write_png(const char *path, png_t *image) {
  FILE *f = fopen(path, "wb");

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

  png_infop info = png_create_info_struct(png);

  png_init_io(png, f);

  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
	  png,
	  info,
	  image->width, image->height,
	  8,
	  PNG_COLOR_TYPE_RGBA,
	  PNG_INTERLACE_NONE,
	  PNG_COMPRESSION_TYPE_DEFAULT,
	  PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png, info);

  png_write_image(png, (png_bytepp)image->row_pointers.data());
  png_write_end(png, nullptr);

  fclose(f);

  png_destroy_write_struct(&png, &info);
}

float diff_pow2(float a, float b) {
  return std::powf(b - a, 2);
}

float euclid_norm(std::tuple<float, float> x, std::tuple<float, float> y) {

  auto [x1, x2] = x;
  auto [y1, y2] = y;

  auto r = diff_pow2(y1, x1) + diff_pow2(y2, x2);

  return std::sqrtf(r);
}

float rdist_div_model(float d, float c, float r, std::vector<float> kterms) {

  float at = 0;
  float result = 1;

  std::for_each(kterms.begin(), kterms.end(), [r, &at, &result](float k) {
	result += k * std::pow(r, 2 * ++at);
  });

  return (d - c) / result;
}



int main(int argc, char **argv) {

  if (argc != 2) {
	return 1;
  }

  png_t original = {};
  load_png(argv[1], &original);

  png_t modified = original;
  modified.row_pointers = std::vector<void *>();

  modified.row_pointers.reserve(modified.height);
  for (size_t i = 0; i < modified.height; ++i) {
	modified.row_pointers.push_back(malloc(modified.width * 4 * sizeof(png_byte)));
  }

  std::vector<float> kterms{-1, .01, .04, .002, .3};

  size_t cx = original.width / 2;
  size_t cy = original.height / 2;

  for (size_t oy = 0; oy < original.height; oy++) {

	auto o_row = static_cast<png_bytep>(original.row_pointers[oy]);
	auto m_row = static_cast<png_bytep>(modified.row_pointers[oy]);

	for (size_t ox = 0; ox < original.width; ox++) {
	  png_bytep o_px = &(o_row[ox * 4]);
	  png_bytep m_px = &(m_row[ox * 4]);

	  float e_norm = euclid_norm({ox, oy}, {cx, cy});

	  size_t mx = (size_t)rdist_div_model(ox, cx, e_norm, kterms);
	  size_t my = (size_t)rdist_div_model(oy, cy, e_norm, kterms);

	  std::cout << std::to_string(ox) << " : " << std::to_string(mx) << std::endl;
	  std::cout << std::to_string(oy) << " : " << std::to_string(my) << std::endl << std::endl;
	}
  }

  write_png("out.png", &modified);

  free_png(original);
  free_png(modified);

  return 0;
}
