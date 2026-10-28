# Copyright 2026, Beyley Cardellio
# Copyright 2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

add_library(
	st_openvr STATIC
	openvr_devices.cpp
	openvr_devices.hpp
	openvr_events.cpp
	openvr_events.hpp
	openvr_negotiate.cpp
	openvr_input.cpp
	openvr_input.hpp
	common/openvr_error.cpp
	common/openvr_error.hpp
	common/openvr_logger.cpp
	common/openvr_logger.hpp
	common/openvr_math.cpp
	common/openvr_math.hpp
	compositor/openvr_compositor.cpp
	compositor/openvr_compositor.hpp
	interfaces/XRTVRApplications.cpp
	interfaces/XRTVRApplications.hpp
	interfaces/XRTVRChaperone.cpp
	interfaces/XRTVRChaperone.hpp
	interfaces/XRTVRChaperoneSetup.cpp
	interfaces/XRTVRChaperoneSetup.hpp
	interfaces/XRTVRClientCore.cpp
	interfaces/XRTVRClientCore.hpp
	interfaces/XRTVRCompositor.cpp
	interfaces/XRTVRCompositor.hpp
	interfaces/XRTVRControlPanel.cpp
	interfaces/XRTVRControlPanel.hpp
	interfaces/XRTVRDebug.cpp
	interfaces/XRTVRDebug.hpp
	interfaces/XRTVRDriverManager.cpp
	interfaces/XRTVRDriverManager.hpp
	interfaces/XRTVRExtendedDisplay.cpp
	interfaces/XRTVRExtendedDisplay.hpp
	interfaces/XRTVRHeadsetView.cpp
	interfaces/XRTVRHeadsetView.hpp
	interfaces/XRTVRInput.cpp
	interfaces/XRTVRInput.hpp
	interfaces/XRTVRIOBuffer.cpp
	interfaces/XRTVRIOBuffer.hpp
	interfaces/XRTVRIPCResourceManagerClient.cpp
	interfaces/XRTVRIPCResourceManagerClient.hpp
	interfaces/XRTVRNotifications.cpp
	interfaces/XRTVRNotifications.hpp
	interfaces/XRTVROverlay.cpp
	interfaces/XRTVROverlay.hpp
	interfaces/XRTVROverlayView.cpp
	interfaces/XRTVROverlayView.hpp
	interfaces/XRTVRRenderModels.cpp
	interfaces/XRTVRRenderModels.hpp
	interfaces/XRTVRResources.cpp
	interfaces/XRTVRResources.hpp
	interfaces/XRTVRScreenshots.cpp
	interfaces/XRTVRScreenshots.hpp
	interfaces/XRTVRSettings.cpp
	interfaces/XRTVRSettings.hpp
	interfaces/XRTVRSpatialAnchors.cpp
	interfaces/XRTVRSpatialAnchors.hpp
	interfaces/XRTVRSystem.cpp
	interfaces/XRTVRSystem.hpp
	interfaces/XRTVRTrackedCamera.cpp
	interfaces/XRTVRTrackedCamera.hpp
	interfaces/old/IVRApplications.hpp
	interfaces/old/IVRApplications/XRTVRApplications_001.hpp
	interfaces/old/IVRApplications/XRTVRApplications_002.hpp
	interfaces/old/IVRApplications/XRTVRApplications_003.hpp
	interfaces/old/IVRApplications/XRTVRApplications_004.hpp
	interfaces/old/IVRApplications/XRTVRApplications_005.hpp
	interfaces/old/IVRApplications/XRTVRApplications_006.hpp
	interfaces/old/IVRApplications/XRTVRApplications_007.hpp
	interfaces/old/IVRChaperone.hpp
	interfaces/old/IVRChaperone/XRTVRChaperone_002.hpp
	interfaces/old/IVRChaperone/XRTVRChaperone_003.hpp
	interfaces/old/IVRChaperoneSetup.hpp
	interfaces/old/IVRChaperoneSetup/XRTVRChaperoneSetup_004.hpp
	interfaces/old/IVRChaperoneSetup/XRTVRChaperoneSetup_005.hpp
	interfaces/old/IVRCompositor.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_005.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_006.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_007.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_008.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_009.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_010.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_011.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_012.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_013.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_014.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_015.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_016.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_018.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_019.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_020.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_021.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_022.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_024.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_026.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_027.hpp
	interfaces/old/IVRCompositor/XRTVRCompositor_028.hpp
	interfaces/old/IVRInput.hpp
	interfaces/old/IVRInput/XRTVRInput_003.hpp
	interfaces/old/IVRInput/XRTVRInput_004.hpp
	interfaces/old/IVRInput/XRTVRInput_005.hpp
	interfaces/old/IVRInput/XRTVRInput_006.hpp
	interfaces/old/IVRInput/XRTVRInput_007.hpp
	interfaces/old/IVRInput/XRTVRInput_010.hpp
	interfaces/old/IVRIOBuffer.hpp
	interfaces/old/IVRIOBuffer/XRTVRIOBuffer_001.hpp
	interfaces/old/IVRIPCResourceManagerClient.hpp
	interfaces/old/IVRIPCResourceManagerClient/XRTVRIPCResourceManagerClient_001.hpp
	interfaces/old/IVRIPCResourceManagerClient/XRTVRIPCResourceManagerClient_002.hpp
	interfaces/old/IVRNotifications.hpp
	interfaces/old/IVRNotifications/XRTVRNotifications_001.hpp
	interfaces/old/IVROverlay.hpp
	interfaces/old/IVROverlay/XRTVROverlay_001.hpp
	interfaces/old/IVROverlay/XRTVROverlay_002.hpp
	interfaces/old/IVROverlay/XRTVROverlay_003.hpp
	interfaces/old/IVROverlay/XRTVROverlay_004.hpp
	interfaces/old/IVROverlay/XRTVROverlay_005.hpp
	interfaces/old/IVROverlay/XRTVROverlay_006.hpp
	interfaces/old/IVROverlay/XRTVROverlay_007.hpp
	interfaces/old/IVROverlay/XRTVROverlay_008.hpp
	interfaces/old/IVROverlay/XRTVROverlay_010.hpp
	interfaces/old/IVROverlay/XRTVROverlay_011.hpp
	interfaces/old/IVROverlay/XRTVROverlay_012.hpp
	interfaces/old/IVROverlay/XRTVROverlay_013.hpp
	interfaces/old/IVROverlay/XRTVROverlay_014.hpp
	interfaces/old/IVROverlay/XRTVROverlay_016.hpp
	interfaces/old/IVROverlay/XRTVROverlay_017.hpp
	interfaces/old/IVROverlay/XRTVROverlay_018.hpp
	interfaces/old/IVROverlay/XRTVROverlay_019.hpp
	interfaces/old/IVROverlay/XRTVROverlay_020.hpp
	interfaces/old/IVROverlay/XRTVROverlay_021.hpp
	interfaces/old/IVROverlay/XRTVROverlay_022.hpp
	interfaces/old/IVROverlay/XRTVROverlay_024.hpp
	interfaces/old/IVROverlay/XRTVROverlay_025.hpp
	interfaces/old/IVROverlay/XRTVROverlay_026.hpp
	interfaces/old/IVROverlay/XRTVROverlay_027.hpp
	interfaces/old/IVRRenderModels.hpp
	interfaces/old/IVRRenderModels/XRTVRRenderModels_001.hpp
	interfaces/old/IVRRenderModels/XRTVRRenderModels_002.hpp
	interfaces/old/IVRRenderModels/XRTVRRenderModels_004.hpp
	interfaces/old/IVRRenderModels/XRTVRRenderModels_005.hpp
	interfaces/old/IVRSettings.hpp
	interfaces/old/IVRSettings/XRTVRSettings_001.hpp
	interfaces/old/IVRSettings/XRTVRSettings_002.hpp
	interfaces/old/IVRSystem.hpp
	interfaces/old/IVRSystem/XRTVRSystem_003.hpp
	interfaces/old/IVRSystem/XRTVRSystem_004.hpp
	interfaces/old/IVRSystem/XRTVRSystem_005.hpp
	interfaces/old/IVRSystem/XRTVRSystem_006.hpp
	interfaces/old/IVRSystem/XRTVRSystem_007.hpp
	interfaces/old/IVRSystem/XRTVRSystem_009.hpp
	interfaces/old/IVRSystem/XRTVRSystem_010.hpp
	interfaces/old/IVRSystem/XRTVRSystem_011.hpp
	interfaces/old/IVRSystem/XRTVRSystem_012.hpp
	interfaces/old/IVRSystem/XRTVRSystem_014.hpp
	interfaces/old/IVRSystem/XRTVRSystem_015.hpp
	interfaces/old/IVRSystem/XRTVRSystem_016.hpp
	interfaces/old/IVRSystem/XRTVRSystem_017.hpp
	interfaces/old/IVRSystem/XRTVRSystem_019.hpp
	interfaces/old/IVRSystem/XRTVRSystem_020.hpp
	interfaces/old/IVRSystem/XRTVRSystem_021.hpp
	interfaces/old/IVRSystem/XRTVRSystem_022.hpp
	interfaces/old/IVRSystem/XRTVRSystem_023.hpp
	interfaces/old/IVRTrackedCamera.hpp
	interfaces/old/IVRTrackedCamera/XRTVRTrackedCamera_001.hpp
	interfaces/old/IVRTrackedCamera/XRTVRTrackedCamera_002.hpp
	interfaces/old/IVRTrackedCamera/XRTVRTrackedCamera_003.hpp
	interfaces/old/IVRTrackedCamera/XRTVRTrackedCamera_004.hpp
	interfaces/old/IVRTrackedCamera/XRTVRTrackedCamera_005.hpp
	)

if(XRT_HAVE_VULKAN)
	target_sources(st_openvr PRIVATE compositor/openvr_compositor_vulkan.cpp)

	target_link_libraries(st_openvr PRIVATE aux_vk aux_render aux_shaders aux_render_cache)
endif()

target_link_libraries(
	st_openvr
	PRIVATE xrt-interfaces xrt-external-openvr
	PUBLIC aux_os aux_glue comp_client
	)

target_include_directories(st_openvr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/)

target_include_directories(
	st_openvr
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..
	)

# The versioned interface stubs deliberately implement an old interface's signature
# alongside the newer implementation they inherit from, so same-named methods with
# different signatures are intentional, not an accident.
target_compile_options(
	st_openvr PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wno-overloaded-virtual>
	)
