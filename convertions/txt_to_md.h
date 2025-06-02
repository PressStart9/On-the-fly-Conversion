#pragma once

#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../convertion.h"

struct txt_to_md {
  static convertion_mapping get_mapping() {
    return { { "txt", "md" }, txt_to_md::convert };
  }

  static int convert(const char* from_file, const char* to_file) {
    int source = open(from_file, O_RDONLY, 0);
    int dest = open(to_file, O_WRONLY | O_CREAT, 0644);

    struct stat stat_source;
    fstat(source, &stat_source);

    sendfile(dest, source, 0, stat_source.st_size);

    close(source);
    close(dest);

    return 0;
  }   
};
