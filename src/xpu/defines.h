#ifndef XPU_DEFINES_H
#define XPU_DEFINES_H

#define XPU_IS_CPU XPU_DETAIL_IS_CPU
#define XPU_IS_CUDA XPU_DETAIL_IS_CUDA
#define XPU_IS_HIP XPU_DETAIL_IS_HIP

#define XPU_D XPU_DETAIL_DEVICE_SPEC

#define XPU_FORCE_INLINE XPU_DETAIL_FORCE_INLINE

#include "detail/defines.h"

#endif
