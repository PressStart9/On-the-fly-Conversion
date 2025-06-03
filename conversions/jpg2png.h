#pragma once

#include "../conversion.h"

struct jpg2png {
  static conversion_mapping get_mapping() {
    return { { "jpg", "png" }, jpg2png::convert };
  }

  static int convert(const char* from_file, const char* to_file);
};
