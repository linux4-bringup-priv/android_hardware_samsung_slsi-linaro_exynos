/*
 * Copyright 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GRALLOC_MAPPER_CAMERA_H
#define GRALLOC_MAPPER_CAMERA_H

#include <string>

#include <android/hardware/graphics/mapper/2.0/IMapper.h>
//#include <utils/StrongPointer.h>

namespace android {

namespace GrallocMapperCamera {

using hardware::graphics::common::V1_0::BufferUsage;
using hardware::graphics::common::V1_0::PixelFormat;
using hardware::graphics::mapper::V2_0::BufferDescriptor;
using hardware::graphics::mapper::V2_0::Error;
using hardware::graphics::mapper::V2_0::IMapper;
using hardware::graphics::mapper::V2_0::YCbCrLayout;

// A wrapper to IMapper
class Mapper {
public:
    Mapper() {};

    // The ownership of acquireFence is always transferred to the callee, even
    // on errors.
    Error lock(buffer_handle_t bufferHandle, uint64_t usage,
            const IMapper::Rect& accessRegion,
            int acquireFence, void** outData) const;

    // The ownership of acquireFence is always transferred to the callee, even
    // on errors.
    Error lock(buffer_handle_t bufferHandle, uint64_t usage,
            const IMapper::Rect& accessRegion,
            int acquireFence, YCbCrLayout* outLayout) const;

    // unlock returns a fence sync object (or -1) and the fence sync object is
    // owned by the caller
    int unlock(buffer_handle_t bufferHandle) const;
};

} // namespace GrallocWrapper

} // namespace android

#endif /*  GRALLOC_MAPPER_CAMERA_H */
