#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include "png2jpg.h"
#include "jpg2png.h"

int png2jpg::convert(const char* from_file, const char* to_file) {
  int width, height, channels;
  unsigned char* data = stbi_load(from_file, &width, &height, &channels, 0);
  if (!data) {
    return 1;
  }

  stbi_write_jpg(to_file, width, height, channels, data, 90);
  stbi_image_free(data);

  return 0;
}

int jpg2png::convert(const char* from_file, const char* to_file) {
  int width, height, channels;
  unsigned char* data = stbi_load(from_file, &width, &height, &channels, 0);
  if (!data) {
    return 1;
  }

  stbi_write_png(to_file, width, height, channels, data, 90);
  stbi_image_free(data);

  return 0;
}
