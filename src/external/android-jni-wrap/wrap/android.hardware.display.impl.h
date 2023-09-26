// Copyright 2022-2023, Qualcomm Innovation Center, Inc.
// SPDX-License-Identifier: BSL-1.0
// Author: Jarvis Huang
// Inline implementations: do not include on its own!

#pragma once

#include "android.view.h"
#include <string>

namespace wrap {
namespace android::hardware::display {
inline std::string DisplayManager::DISPLAY_CATEGORY_PRESENTATION() {
    auto &data = Meta::data(true);
    auto ret = get(data.DISPLAY_CATEGORY_PRESENTATION, data.clazz());
    data.dropClassRef();
    return ret;
}

inline view::Display DisplayManager::getDisplay(int32_t displayId) const {
    assert(!isNull());
    return view::Display(
        object().call<jni::Object>(Meta::data().getDisplay, displayId));
}

inline jni::Array<jni::Object> DisplayManager::getDisplays() const {
    assert(!isNull());
    return jni::Array<jni::Object>(
        (jobjectArray)object()
            .call<jni::Object>(Meta::data().getDisplays)
            .getHandle(),
        0);
}

inline jni::Array<jni::Object>
DisplayManager::getDisplays(std::string const &category) {
    assert(!isNull());
    return jni::Array<jni::Object>(
        (jobjectArray)object()
            .call<jni::Object>(Meta::data().getDisplays1, category)
            .getHandle(),
        0);
}

inline std::string DeviceProductInfo::getName() const {
    assert(!isNull());
    return object().call<std::string>(Meta::data().getName);
}

} // namespace android::hardware::display
} // namespace wrap
