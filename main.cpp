#include <dirent.h>
#include <fuse/fuse.h>
#include <string.h>
#include <sys/xattr.h>

#include <functional>
#include <iostream>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "convertion.h"
#include "convertions/txt_to_md.h"

#pragma region Config

struct fs_config {
  char* original_dir;
} config;

static struct fuse_opt fs_opts[] = {
    {"-d %s", offsetof(struct fs_config, original_dir), 0},
    {"--original-dir=%s", offsetof(struct fs_config, original_dir), 0},
    FUSE_OPT_END};

#define FULL_PATHM(path, full_path)                          \
  char full_path[PATH_MAX];                                  \
  snprintf(full_path, PATH_MAX, "%s%s", config.original_dir, \
           path + 1);  // +1 to remove leading '/'

#define FULL_PATH(path) FULL_PATHM(path, full_path)

#define FIND_ORIGINAL_PATH(path)                                          \
  FULL_PATH(path);                                                        \
  char orig_path[PATH_MAX];                                               \
  bool is_full_original = true;                                           \
  strncpy(orig_path, full_path, PATH_MAX);                                \
  char* from_ext = nullptr;                                               \
  const char* to_ext = nullptr;                                           \
  if (access(orig_path, F_OK) != F_OK) {                                  \
    is_full_original = false;                                             \
    from_ext = strrchr(orig_path, '.');                                   \
    if (!from_ext) { return -ENOENT; }                                    \
    to_ext = full_path + (from_ext - orig_path);                          \
    auto rev_conv = possible_reverse_convertions.find(from_ext);          \
    if (rev_conv == possible_reverse_convertions.end()) {                 \
      return -ENOENT;                                                     \
    }                                                                     \
                                                                          \
    bool is_found = false;                                                \
    for (const char* from_extention : rev_conv->second) {                 \
      strncpy(from_ext, from_extention, PATH_MAX + orig_path - from_ext); \
      if (access(orig_path, F_OK) == F_OK) {                              \
        is_found = true;                                                  \
        break;                                                            \
      }                                                                   \
    }                                                                     \
    if (!is_found) {                                                      \
      return -ENOENT;                                                     \
    }                                                                     \
  }

#pragma endregion  // Config

#pragma region Convertions

std::unordered_map<const char*, std::vector<const char*>, CharPointerComp,
                   CharPointerComp>
    possible_reverse_convertions;

std::unordered_map<const char*, std::vector<const char*>, CharPointerComp,
                   CharPointerComp>
    possible_convertions;

std::unordered_map<Convertation, std::function<void(const char* from_file,
                                                    const char* to_file)>>
    convertion_functions;

#pragma endregion  // Convertions

#pragma region FuseOperations

static int fs_getattr(const char* path, struct stat* stbuf) {
  std::cout << "--getattr   " << std::endl;
  FIND_ORIGINAL_PATH(path);
  std::cout << '\t' << orig_path << std::endl;

  if (is_full_original) {
    return lstat(full_path, stbuf) == -1 ? -errno : 0;
  }

  struct timespec now = { .tv_sec = time(NULL), .tv_nsec = 0 };
  *stbuf = (struct stat){
      // view is the only one link
      .st_nlink = 1,
      // read for all, write for owner
      .st_mode = S_IFREG | 0644,
      // this process owns this view
      .st_uid = getuid(),
      .st_gid = getgid(),
      // real file is not created
      .st_size = 0,
      .st_blocks = 0,
      // file have never been accesssed
      .st_atim = now,
      .st_mtim = now,
      .st_ctim = now,
  };

  return 0;
}

static int fs_open(const char* path, struct fuse_file_info* fi) {
  std::cout << "--open   " << path << std::endl;
  FIND_ORIGINAL_PATH(path);
  if (!is_full_original) {
    auto func = convertion_functions.find({from_ext, to_ext});
    if (func == convertion_functions.end()) {
      return -ENOENT;
    }

    func->second(orig_path, full_path);
  }

  int fd = open(full_path, fi->flags);
  if (fd == -1) {
    return -errno;
  }
  fi->fh = fd;
  fi->direct_io = 1;

  return 0;
}

static int fs_read(const char* path, char* buf, size_t size, off_t offset,
                   struct fuse_file_info* fi) {
  std::cout << "--read   " << path << std::endl;
  int ret = lseek(fi->fh, offset, SEEK_SET);
  if (ret == -1) {
    return -errno;
  }

  ret = pread(fi->fh, buf, size, offset);

  return ret == -1 ? -errno : ret;
}

static int fs_release(const char* path, struct fuse_file_info* fi) {
  std::cout << "--release" << std::endl;
  int ret = close(fi->fh);
  return ret == -1 ? -errno : ret;
}

static int fs_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                      off_t offset, fuse_file_info* fi) {
  std::cout << "--readdir   " << path << std::endl;
  FULL_PATH(path);
  DIR* dr = opendir(full_path);
  if (!dr) {
    return -ENOENT;
  }

  char item_path[PATH_MAX];
  strncpy(item_path, full_path, PATH_MAX);
  char* item_diff_start = strrchr(item_path, '\0');
  struct dirent* de;
  while ((de = readdir(dr)) != NULL) {
    if (filler(buf, de->d_name, nullptr, 0) == -1) {
      break;
    }

    strncpy(item_diff_start, de->d_name,
            PATH_MAX + item_path - item_diff_start);
    char* item_ext_diff_start = strrchr(item_path, '.');
    if (!item_ext_diff_start) { continue; }
    auto poss_conv = possible_convertions.find(item_ext_diff_start);
    if (poss_conv == possible_convertions.end()) {
      continue;
    }
    for (auto ext : poss_conv->second) {
      strncpy(item_ext_diff_start, ext, PATH_MAX + item_path - item_diff_start);
      if (access(item_path, F_OK) == F_OK) {
        continue;
      }
      if (filler(buf, item_diff_start, nullptr, 0) == -1) {
        break;
      }
    }
  }

  return 0;
}

#pragma region RedirectOps

static int fs_readlink(const char* path, char* buf, size_t size) {
  int res = readlink(path, buf, size - 1);
  if (res == -1) {
    return -errno;
  }
  buf[res] = '\0';
  return 0;
}

static int fs_mknod(const char* path, mode_t mode, dev_t rdev) {
  FULL_PATH(path);
  int res;

  if (S_ISREG(mode)) {
    res = open(full_path, O_CREAT | O_EXCL | O_WRONLY, mode);
    if (res >= 0) res = close(res);
  } else if (S_ISFIFO(mode)) {
    res = mkfifo(full_path, mode);
  } else {
    res = mknod(full_path, mode, rdev);
  }

  return res == -1 ? -errno : 0;
}

static int fs_mkdir(const char* path, mode_t mode) {
  FULL_PATH(path);
  int res = mkdir(full_path, mode);
  return res == -1 ? -errno : 0;
}

static int fs_unlink(const char* path) {
  FULL_PATH(path);
  int res = unlink(full_path);
  return res == -1 ? -errno : 0;
}

static int fs_rmdir(const char* path) {
  FULL_PATH(path);
  int res = rmdir(full_path);
  return res == -1 ? -errno : 0;
}

static int fs_symlink(const char* target, const char* linkpath) {
  FULL_PATH(linkpath);
  int res = symlink(target, full_path);
  return res == -1 ? -errno : 0;
}

static int fs_rename(const char* from, const char* to) {
  FULL_PATH(from);
  FULL_PATHM(to, to_full_path);
  int res = rename(full_path, to_full_path);
  return res == -1 ? -errno : 0;
}

static int fs_link(const char* from, const char* to) {
  FULL_PATH(from);
  FULL_PATHM(to, to_full_path);
  int res = link(full_path, to_full_path);
  return res == -1 ? -errno : 0;
}

static int fs_chmod(const char* path, mode_t mode) {
  FULL_PATH(path);
  int res = chmod(full_path, mode);
  return res == -1 ? -errno : 0;
}

static int fs_chown(const char* path, uid_t uid, gid_t gid) {
  FULL_PATH(path);
  int res = lchown(full_path, uid, gid);
  return res == -1 ? -errno : 0;
}

static int fs_truncate(const char* path, off_t size) {
  FULL_PATH(path);
  int res = truncate(full_path, size);
  return res == -1 ? -errno : 0;
}

static int fs_utime(const char* path, struct utimbuf* ubuf) {
  FULL_PATH(path);
  int res = utime(full_path, ubuf);
  return res == -1 ? -errno : 0;
}

static int fs_write(const char* path, const char* buf, size_t size,
                    off_t offset, struct fuse_file_info* fi) {
  int fd = fi->fh;
  int res = pwrite(fd, buf, size, offset);
  return res == -1 ? -errno : res;
}

static int fs_statfs(const char* path, struct statvfs* stbuf) {
  FULL_PATH(path);
  int res = statvfs(full_path, stbuf);
  return res == -1 ? -errno : 0;
}

static int fs_fsync(const char* path, int isdatasync,
                    struct fuse_file_info* fi) {
  int res;
  if (isdatasync)
    res = fdatasync(fi->fh);
  else
    res = fsync(fi->fh);
  return res == -1 ? -errno : 0;
}

static int fs_setxattr(const char* path, const char* name, const char* value,
                       size_t size, int flags) {
  FULL_PATH(path);
  int res = lsetxattr(full_path, name, value, size, flags);
  return res == -1 ? -errno : 0;
}

static int fs_getxattr(const char* path, const char* name, char* value,
                       size_t size) {
  FULL_PATH(path);
  int res = lgetxattr(full_path, name, value, size);
  return res == -1 ? -errno : res;
}

static int fs_listxattr(const char* path, char* list, size_t size) {
  FULL_PATH(path);
  int res = llistxattr(full_path, list, size);
  return res == -1 ? -errno : res;
}

static int fs_removexattr(const char* path, const char* name) {
  FULL_PATH(path);
  int res = lremovexattr(full_path, name);
  return res == -1 ? -errno : 0;
}

static int fs_opendir(const char* path, struct fuse_file_info* fi) {
  FULL_PATH(path);
  DIR* dp = opendir(full_path);
  if (dp == NULL) return -errno;
  fi->fh = (intptr_t)dp;
  return 0;
}

static int fs_releasedir(const char* path, struct fuse_file_info* fi) {
  DIR* dp = (DIR*)(uintptr_t)fi->fh;
  closedir(dp);
  return 0;
}

static int fs_access(const char* path, int mask) {
  FULL_PATH(path);
  int res = access(full_path, mask);
  return res == -1 ? -errno : 0;
}

static int fs_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
  FULL_PATH(path);
  int fd = open(full_path, fi->flags, mode);
  if (fd == -1) {
    return -errno;
  }
  fi->fh = fd;
  return 0;
}

static int fs_ftruncate(const char* path, off_t size,
                        struct fuse_file_info* fi) {
  int res = ftruncate(fi->fh, size);
  return res == -1 ? -errno : 0;
}

static int fs_fgetattr(const char* path, struct stat* stbuf,
                       struct fuse_file_info* fi) {
  int res = fstat(fi->fh, stbuf);
  return res == -1 ? -errno : 0;
}

static int fs_lock(const char* path, struct fuse_file_info* fi, int cmd,
                   struct flock* lock) {
  return fcntl(fi->fh, cmd, lock) == -1 ? -errno : 0;
}

static int fs_utimens(const char* path, const struct timespec ts[2]) {
  FULL_PATH(path);
  int res = utimensat(AT_FDCWD, full_path, ts, AT_SYMLINK_NOFOLLOW);
  return res == -1 ? -errno : 0;
}

#pragma endregion

static fuse_operations convertation_fuse_operations = {
    .getattr = fs_getattr,
    .readlink = fs_readlink,
    .mknod = fs_mknod,
    .mkdir = fs_mkdir,
    .unlink = fs_unlink,
    .rmdir = fs_rmdir,
    .symlink = fs_symlink,
    .rename = fs_rename,
    .link = fs_link,
    .chmod = fs_chmod,
    .chown = fs_chown,
    .truncate = fs_truncate,
    .utime = fs_utime,
    .open = fs_open,
    .read = fs_read,
    .write = fs_write,
    .statfs = fs_statfs,
    .release = fs_release,
    .fsync = fs_fsync,
    .setxattr = fs_setxattr,
    .getxattr = fs_getxattr,
    .listxattr = fs_listxattr,
    .removexattr = fs_removexattr,
    .opendir = fs_opendir,
    .readdir = fs_readdir,
    .releasedir = fs_releasedir,
    .fsyncdir = nullptr,
    .access = fs_access,
    .create = fs_create,
    .ftruncate = fs_ftruncate,
    .fgetattr = fs_fgetattr,
    .lock = fs_lock,
    .utimens = fs_utimens,
};

#pragma endregion FuseOperations

int main(int argc, char** argv) {
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  if (fuse_opt_parse(&args, &config, fs_opts, nullptr) == -1) {
    throw std::runtime_error("Can't parse arguments.");
  }
  if (config.original_dir == nullptr) {
    throw std::runtime_error("Original directory is not specified.");
  }

  std::cout << "Original directory: " << config.original_dir << std::endl;

  std::array mappings{
      txt_to_md::get_mapping(),
  };

  for (convertion_mapping& mapping : mappings) {
    possible_convertions[mapping.first.from].push_back(mapping.first.to);
    possible_reverse_convertions[mapping.first.to].push_back(
        mapping.first.from);
    convertion_functions[mapping.first] = mapping.second;
  }

  int ret =
      fuse_main(args.argc, args.argv, &convertation_fuse_operations, nullptr);

  fuse_opt_free_args(&args);

  std::cout << "Finish working." << std::endl;

  return ret;
}