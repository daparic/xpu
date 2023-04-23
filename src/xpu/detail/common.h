#ifndef XPU_DETAIL_COMMON_H
#define XPU_DETAIL_COMMON_H

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace xpu::detail {

template<typename I, typename T>
struct action {
    using image = I;
    using tag = T;
};

enum mem_type {
    mem_host,
    mem_device,
    mem_shared,
    mem_unknown,
};

enum driver_t {
    cpu,
    cuda,
    hip,
    sycl,
};
constexpr inline size_t num_drivers = 4;
const char *driver_to_str(driver_t, bool lower = false);

enum direction_t {
    dir_h2d,
    dir_d2h,
};

struct device {
    int id;
    driver_t backend;
    int device_nr;
};

struct device_prop {
    // Filled by driver
    std::string name;
    driver_t driver;
    std::string arch;
    size_t shared_mem_size;
    size_t const_mem_size;

    size_t warp_size;
    size_t max_threads_per_block;
    std::array<size_t, 3> max_grid_size;

    // Filled by runtime
    std::string xpuid;
    int id;
    int device_nr;

    size_t global_mem_total;
    size_t global_mem_available;
};

struct ptr_prop {
    mem_type type;
    device dev;
    void *ptr;
};

struct queue_handle {
    queue_handle();
    queue_handle(device dev);
    ~queue_handle();

    queue_handle(const queue_handle &) = delete;
    queue_handle &operator=(const queue_handle &) = delete;
    queue_handle(queue_handle &&) = delete;
    queue_handle &operator=(queue_handle &&) = delete;

    void *handle;
    device dev;
};

struct kernel_timings {
    std::string_view name; // Fine to make string_view, since kernel names are static
    std::vector<double> times;
};

struct timings {
    double wall = 0;

    bool has_details = false;
    std::vector<kernel_timings> kernels;
    double copy_h2d = 0;
    double copy_d2h = 0;
    double memset = 0;

    std::string name;
    std::vector<timings> children;
};

using error = int;

} // namespace xpu::detail

#endif
