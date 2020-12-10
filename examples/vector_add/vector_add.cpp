#include "VectorOps.h"
#include <xpu/host.h>

#include <cassert>
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
    constexpr int NElems = 100;

    xpu::initialize(xpu::driver::cuda); // or xpu::driver::cuda
    // xpu::select_device(xpu::any_gpu | xpu::cpu_fallback);

    std::vector<float> hx(NElems, 8);
    std::vector<float> hy(NElems, 8);

    float *dx = xpu::device_malloc<float>(NElems);
    float *dy = xpu::device_malloc<float>(NElems);
    float *dz = xpu::device_malloc<float>(NElems);

    xpu::copy(dx, hx.data(), NElems);
    xpu::copy(dy, hy.data(), NElems);

    xpu::run_kernel<VectorOps::add>(xpu::grid::n_threads(NElems), dx, dy, dz, NElems);

    std::vector<float> hz(NElems);
    xpu::copy(hz.data(), dz, NElems);

    for (auto &x: hz) {
        if (x != 16) {
            std::cout << "ERROR" << std::endl;
            abort();
        }
    }

    xpu::free(dx);
    xpu::free(dy);
    xpu::free(dz);

    std::cout << "Looking good!" << std::endl;
    return 0;
}