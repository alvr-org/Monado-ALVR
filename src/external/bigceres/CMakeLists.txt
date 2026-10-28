# Copyright 2026, Beyley Cardellio
# SPDX-License-Identifier: BSL-1.0

# Set our ceres target to the found system ceres
set(XRT_CERES_TARGET Ceres::ceres)

# Create a wrapper library to carry our ceres target.
add_library(xrt-external-bigceres INTERFACE)
target_link_libraries(xrt-external-bigceres INTERFACE ${XRT_CERES_TARGET})
