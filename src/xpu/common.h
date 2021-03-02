#ifndef XPU_COMMON_H
#define XPU_COMMON_H

#include "defines.h"

namespace xpu {

struct dim {
    int x = 0;
    int y = 0;
    int z = 0;

    XPU_D dim(int _x) : x(_x) {}
    XPU_D dim(int _x, int _y) : x(_x), y(_y) {}
    XPU_D dim(int _x, int _y, int _z) : x(_x), y(_y), z(_z) {}
};

struct grid {

    static inline grid n_blocks(dim blocks);

    static inline grid n_threads(dim threads);

    dim blocks;
    dim threads;

private:
    grid(dim b, dim t);

};

struct kernel_info {
    dim i_thread;
    dim n_threads;
    dim i_block;
    dim n_blocks;
};

enum class side {
    host,
    device,
};

template<typename T, side S>
struct cmem_io {};

template<typename T, side S>
using cmem_io_t = typename cmem_io<T, S>::type;

template<typename T, side S>
struct cmem_device {};

template<typename T, side S>
using cmem_device_t = typename cmem_device<T, S>::type;

} // namespace xpu

#include "common_impl.h"

#endif
