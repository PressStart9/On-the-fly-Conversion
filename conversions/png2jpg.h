#pragma once

#include "../conversion.h"

struct png2jpg {
  static conversion_mapping get_mapping() {
    return { { "png", "jpg" }, png2jpg::convert };
  }

  static int convert(const char* from_file, const char* to_file);
};
