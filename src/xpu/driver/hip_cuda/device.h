#ifndef XPU_DRIVER_CUDA_DEVICE_RUNTIME_H
#define XPU_DRIVER_CUDA_DEVICE_RUNTIME_H

#ifndef XPU_DEVICE_H
#error "This header should not be included directly. Include xpu/device.h instead."
#endif

#include "../../detail/macros.h"

#include <cub/cub.cuh>

#define XPU_CMEM_IDENTIFIER(name) XPU_CONCAT(xpu_cuda_driver_cmem_symbol_, name)

#if XPU_IS_CUDA
#define XPU_CHOOSE(hip, cuda) cuda
#else
#define XPU_CHOOSE(hip, cuda) hip
#endif

#if XPU_IS_CUDA
#define XPU_INTERNAL_LAUNCH_KERNEL(name, nBlocks, nThreads, ...) \
    name<<<(nBlocks), (nThreads)>>>(XPU_PARAM_NAMES(() 0, ##__VA_ARGS__))
#else
#define XPU_INTERNAL_LAUNCH_KERNEL(name, nBlocks, nThreads, ...) \
    hipLaunchKernelGGL(name, dim3(nBlocks), dim3(nThreads), 0, 0, XPU_PARAM_NAMES(() 0, ##__VA_ARGS__))
#endif

#if XPU_IS_CUDA
#define XPU_INTERNAL_SUFFIX _Cuda
#else
#define XPU_INTERNAL_SUFFIX _Hip
#endif

// TODO: don't hardcode block size
#define XPU_DETAIL_KERNEL(deviceLibrary, name, sharedMemoryT, ...) \
    __device__ void name ## _impl(XPU_PARAM_LIST((const xpu::kernel_info &) info, (sharedMemoryT &) smem, ##__VA_ARGS__)); \
    __global__ void name ## _entry(XPU_PARAM_LIST((char) /*dummyArg*/, ##__VA_ARGS__)) { \
        __shared__ sharedMemoryT shm; \
        xpu::kernel_info info{ \
            .i_thread = xpu::dim{xpu::thread_idx::x(), 0, 0}, \
            .n_threads  = xpu::dim{xpu::block_dim::x(), 0, 0}, \
            .i_block  = xpu::dim{xpu::block_idx::x(), 0, 0}, \
            .n_blocks   = xpu::dim{xpu::grid_dim::x(), 0, 0} \
        }; \
        name ## _impl(XPU_PARAM_NAMES(() info, () shm, ##__VA_ARGS__)); \
    } \
    xpu::detail::error XPU_CONCAT(deviceLibrary, XPU_INTERNAL_SUFFIX)::run_ ## name(XPU_PARAM_LIST((xpu::grid) params, ##__VA_ARGS__)) { \
        printf("Running Kernel " #name "\n"); \
        if (params.threads.x > -1) { \
            XPU_INTERNAL_LAUNCH_KERNEL(name ## _entry, (params.threads.x + 63) / 64, 64, ##__VA_ARGS__); \
        } else { \
            XPU_INTERNAL_LAUNCH_KERNEL(name ## _entry, params.blocks.x, 64, ##__VA_ARGS__); \
        } \
        auto err = XPU_CHOOSE(hipDeviceSynchronize(), cudaDeviceSynchronize()); \
        if (err != 0) { \
            printf("Kernel Error: %s\n", XPU_CHOOSE(hipGetErrorString(err), cudaGetErrorString(err))); \
        } \
        return err; \
    } \
    __device__ inline void name ## _impl( XPU_PARAM_LIST((const xpu::kernel_info &) info, (sharedMemoryT &) shm, ##__VA_ARGS__))

#define XPU_DETAIL_ASSERT(x) static_cast<void>(0)


namespace xpu {

XPU_D XPU_FORCE_INLINE int thread_idx::x() {
    return XPU_CHOOSE(hipThreadIdx_x, threadIdx.x);
}

XPU_D XPU_FORCE_INLINE int block_dim::x() {
    return XPU_CHOOSE(hipBlockDim_x, blockDim.x);
}

XPU_D XPU_FORCE_INLINE int block_idx::x() {
    return XPU_CHOOSE(hipBlockIdx_x, blockIdx.x);
}

XPU_D XPU_FORCE_INLINE int grid_dim::x() {
    return XPU_CHOOSE(hipGridDim_x, gridDim.x);
}

// XPU_D XPU_FORCE_INLINE constexpr float pi() { return 3.14159265358979323846f; }

XPU_D XPU_FORCE_INLINE float ceil(float x) { return ::ceilf(x); }
XPU_D XPU_FORCE_INLINE float cos(float x) { return ::cosf(x); }
XPU_D XPU_FORCE_INLINE float abs(float x) { return ::fabsf(x); }
XPU_D XPU_FORCE_INLINE float min(float a, float b) { return ::fminf(a, b); }
XPU_D XPU_FORCE_INLINE float max(float a, float b) { return ::fmaxf(a, b); }
XPU_D XPU_FORCE_INLINE int   abs(int a) { return ::abs(a); }
XPU_D XPU_FORCE_INLINE int   min(int a, int b) { return ::min(a, b); }
XPU_D XPU_FORCE_INLINE unsigned long long int min(unsigned long long int a, unsigned long long int b) { return ::min(a, b); }
XPU_D XPU_FORCE_INLINE long long int min(long long int a, long long int b) { return ::min(a, b); }
XPU_D XPU_FORCE_INLINE int   max(int a, int b) { return ::max(a, b); }
XPU_D XPU_FORCE_INLINE float sqrt(float x) { return ::sqrtf(x); }
XPU_D XPU_FORCE_INLINE float tan(float x) { return ::tanf(x); }

XPU_D XPU_FORCE_INLINE int atomic_add_block(int *addr, int val) { return atomicAdd(addr, val); }

template<typename C>
struct cmem_accessor {
};

namespace detail {

template<typename T>
struct numeric_limits {};

template<>
struct numeric_limits<float> {
    __device__ static constexpr float max_or_inf() { return INFINITY; }
};

template<>
struct numeric_limits<unsigned int> {
    __device__ static constexpr unsigned int max_or_inf() { return UINT_MAX; }
};

} // namespace detail

template<typename C> XPU_D XPU_FORCE_INLINE const C &cmem() { return cmem_accessor<C>::get(); }

template<typename Key, int BlockSize, int ItemsPerThread>
class block_sort<Key, BlockSize, ItemsPerThread, xpu::driver::cuda> {

public:
    using block_radix_sort = cub::BlockRadixSort<Key, BlockSize, ItemsPerThread, short>;

    using storage_t = typename block_radix_sort::TempStorage;

    __device__ block_sort(storage_t &storage_) : storage(storage_) {}

    template<typename T, typename KeyGetter>
    __device__ T *sort(T *data, size_t N, T *buf, KeyGetter &&getKey) {
        return radix_sort(data, N, buf, getKey);
    }

private:
    storage_t &storage;

    template<typename T, typename KeyGetter>
    __device__ T *radix_sort(T *data, size_t N, T *buf, KeyGetter &&getKey) {
        const int ItemsPerBlock = BlockSize * ItemsPerThread;

        size_t nItemBlocks = N / ItemsPerBlock + (N % ItemsPerBlock > 0 ? 1 : 0);

        Key keys_local[ItemsPerThread];
        short index_local[ItemsPerThread];

        for (size_t i = 0; i < nItemBlocks; i++) {
            size_t start = i * ItemsPerBlock;
            for (size_t b = 0; b < ItemsPerThread; b++) {
                short idx = b * BlockSize + xpu::thread_idx::x();
                size_t global_idx = start + idx;
                if (global_idx < N) {
                    keys_local[b] = getKey(data[global_idx]);
                 } else {
                    keys_local[b] = detail::numeric_limits<Key>::max_or_inf();
                }
                index_local[b] = idx;
            }

            block_radix_sort(storage).Sort(keys_local, index_local);

            T tmp[ItemsPerThread];

            for (size_t b = 0; b < ItemsPerThread; b++) {
                size_t from = start + index_local[b];
                if (from < N) {
                    tmp[b] = data[from];
                }
            }
            __syncthreads();

            for (size_t b = 0; b < ItemsPerThread; b++) {
                size_t to = start + thread_idx::x() * ItemsPerThread + b;
                if (to < N) {
                    data[to] = tmp[b];
                }
            }
            __syncthreads();

        }

        __syncthreads();

        T *src = data;
        T *dst = buf;

        for (size_t blockSize = ItemsPerBlock; blockSize < N; blockSize *= 2) {

            size_t carryStart = 0;
            for (size_t st = 0; st + blockSize < N; st += 2 * blockSize) {
                size_t st2 = st + blockSize;
                size_t blockSize2 = min((unsigned long long int)(N - st2), (unsigned long long int)blockSize);
                carryStart = st2 + blockSize2;

                merge(&src[st], &src[st2], blockSize, blockSize2, &dst[st], getKey);
            }

            for (size_t i = carryStart + thread_idx::x(); i < N; i += block_dim::x()) {
                dst[i] = src[i];
            }

            __syncthreads();

            T *tmp = src;
            src = dst;
            dst = tmp;
        }

        return src;
    }

    template<typename T, typename KeyGetter>
    __device__ void merge(const T *block1, const T *block2, size_t block_size1, size_t block_size2, T *out, KeyGetter &&getKey) {
        if (thread_idx::x() > 0) {
            return;
        }

        size_t i1 = 0;
        size_t i2 = 0;
        size_t i_out = 0;

        while (i1 < block_size1 && i2 < block_size2) {
            if (getKey(block1[i1]) < getKey(block2[i2])) {
                out[i_out] = block1[i1];
                i1++;
            } else {
                out[i_out] = block2[i2];
                i2++;
            }
            i_out++;
        }

        size_t r_i = (i1 < block_size1 ? i1 : i2);
        size_t r_size = (i1 < block_size1 ? block_size1 : block_size2);
        const T *r_block = (i1 < block_size1 ? block1 : block2);
        for (; r_i < r_size; r_i++, i_out++) {
            out[i_out] = r_block[r_i];
        }
    }

};

} // namespace xpu

#endif
