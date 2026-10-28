# Copyright 2019-2025, Collabora, Ltd.
# Copyright 2026, NVIDIA CORPORATION.
#
# SPDX-License-Identifier: BSL-1.0

set(ENABLED_HEADSET_DRIVERS)
set(ENABLED_DRIVERS)

add_library(drv_includes INTERFACE)
target_include_directories(drv_includes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if(XRT_BUILD_DRIVER_ALVR)
	# TODO: Only requires eventmanager header actually
	add_subdirectory("${CMAKE_SOURCE_DIR}/../alvr_render" "alvr_render2")
	get_property(alvr_inc_dirs TARGET AlvrRender PROPERTY INCLUDE_DIRECTORIES)

	add_library(
		drv_alvr STATIC alvr/alvr.cpp alvr/alvr_interface.h
				   alvr/alvr_prober.c
		)
	target_link_libraries(drv_alvr PRIVATE xrt-interfaces aux_util aux_os AlvrRender)
	target_include_directories(drv_alvr PRIVATE ${alvr_inc_dirs})
	list(APPEND ENABLED_DRIVERS alvr)
endif()


if(XRT_BUILD_DRIVER_ARDUINO)
	add_library(
		drv_arduino STATIC arduino/arduino_device.c arduino/arduino_interface.h
				   arduino/arduino_prober.c
		)
	target_link_libraries(drv_arduino PRIVATE xrt-interfaces aux_util aux_os aux_os_ble)
	list(APPEND ENABLED_DRIVERS arduino)
endif()

add_library(drv_cemu STATIC ht_ctrl_emu/ht_ctrl_emu.cpp ht_ctrl_emu/ht_ctrl_emu_interface.h)
target_link_libraries(drv_cemu PRIVATE xrt-interfaces aux_util)
list(APPEND ENABLED_HEADSET_DRIVERS drv_cemu)

if(XRT_BUILD_DRIVER_BLUBUR_S1)
	add_library(
		drv_blubur_s1 STATIC blubur_s1/blubur_s1_hmd.c blubur_s1/blubur_s1_interface.h
				     blubur_s1/blubur_s1_prober.c
		)
	target_link_libraries(drv_blubur_s1 PRIVATE xrt-interfaces aux_util aux_os)
	list(APPEND ENABLED_DRIVERS blubur_s1)
endif()

if(XRT_BUILD_DRIVER_CONTACTGLOVE)
	add_library(
		drv_contactglove STATIC
		contactglove/contactglove.c
		contactglove/contactglove_calibration.c
		contactglove/contactglove_calibration.h
		contactglove/contactglove_interface.h
		contactglove/contactglove_internal.h
		contactglove/contactglove_prober.c
		contactglove/contactglove_protocol.h
		)
	target_link_libraries(drv_contactglove PRIVATE xrt-interfaces aux_util aux_os)
	list(APPEND ENABLED_DRIVERS contactglove)
endif()

if(XRT_BUILD_DRIVER_DAYDREAM)
	add_library(
		drv_daydream STATIC daydream/daydream_device.c daydream/daydream_device.h
				    daydream/daydream_interface.h daydream/daydream_prober.c
		)
	target_link_libraries(drv_daydream PRIVATE xrt-interfaces aux_util aux_os aux_os_ble)
	list(APPEND ENABLED_DRIVERS daydream)
endif()

if(XRT_BUILD_DRIVER_DEPTHAI)
	add_library(drv_depthai STATIC depthai/depthai_driver.cpp depthai/depthai_interface.h)
	target_link_libraries(
		drv_depthai
		PRIVATE
			xrt-interfaces
			aux_os
			aux_util
			aux_tracking
			${OpenCV_LIBRARIES}
			depthai::core
			XLink
		)
	target_include_directories(drv_depthai PRIVATE ${OpenCV_INCLUDE_DIRS})
	list(APPEND ENABLED_DRIVERS depthai)
endif()

if(XRT_BUILD_DRIVER_SIMULATED)
	add_library(
		drv_simulated STATIC simulated/simulated_controller.c simulated/simulated_hmd.c
				     simulated/simulated_interface.h simulated/simulated_prober.c
		)
	target_link_libraries(drv_simulated PRIVATE xrt-interfaces aux_util)
	list(APPEND ENABLED_HEADSET_DRIVERS simulated)
endif()

if(XRT_BUILD_DRIVER_TWRAP)
	add_library(drv_twrap STATIC twrap/twrap_slam.c twrap/twrap_interface.h)
	target_link_libraries(drv_twrap PRIVATE xrt-interfaces aux_util)
endif()

if(XRT_BUILD_DRIVER_QWERTY)
	add_library(
		drv_qwerty STATIC
		qwerty/qwerty_device.c
		qwerty/qwerty_device.h
		qwerty/qwerty_interface.h
		qwerty/qwerty_prober.c
		qwerty/qwerty_sdl.c
		)
	target_link_libraries(drv_qwerty PRIVATE xrt-interfaces aux_util SDL2::SDL2)
	list(APPEND ENABLED_DRIVERS qwerty)

	add_library(drv_qwerty_includes INTERFACE)
	target_include_directories(drv_qwerty_includes INTERFACE qwerty)
endif()

if(XRT_BUILD_DRIVER_HDK)
	set(HDK_SOURCE_FILES)

	add_library(
		drv_hdk STATIC hdk/hdk_device.cpp hdk/hdk_device.h hdk/hdk_interface.h
			       hdk/hdk_prober.c
		)
	target_link_libraries(drv_hdk PRIVATE xrt-interfaces aux_math)
	list(APPEND ENABLED_HEADSET_DRIVERS hdk)
endif()

if(XRT_BUILD_DRIVER_HYDRA)
	set(HYDRA_SOURCE_FILES)

	add_library(drv_hydra STATIC hydra/hydra_driver.c hydra/hydra_interface.h)
	target_link_libraries(drv_hydra PRIVATE xrt-interfaces aux_os aux_util)
	list(APPEND ENABLED_DRIVERS hydra)
endif()

if(XRT_BUILD_DRIVER_NS)
	add_library(
		drv_ns STATIC
		north_star/distortion_3d/utility_northstar.h
		north_star/distortion_3d/deformation_northstar.h
		north_star/distortion_3d/deformation_northstar.cpp
		north_star/ns_hmd.h
		north_star/ns_hmd.c
		north_star/ns_interface.h
		)
	target_link_libraries(drv_ns PRIVATE xrt-interfaces aux_math xrt-external-cjson)
	list(APPEND ENABLED_HEADSET_DRIVERS ns)
endif()

if(XRT_BUILD_DRIVER_XREAL_AIR)
	add_library(
		drv_xreal_air STATIC xreal_air/xreal_air_hmd.h xreal_air/xreal_air_hmd.c
				     xreal_air/xreal_air_interface.h xreal_air/xreal_air_packet.c
		)
	target_link_libraries(
		drv_xreal_air PRIVATE xrt-interfaces HIDAPI::hidapi aux_util xrt-external-cjson
		)
	target_include_directories(drv_xreal_air PRIVATE ${HIDAPI_INCLUDE_DIRS})
	list(APPEND ENABLED_HEADSET_DRIVERS xreal_air)
endif()

if(XRT_BUILD_DRIVER_UVC)
	add_library(drv_uvc STATIC uvc/uvc_interface.h uvc/uvc_driver.c uvc/uvc_internal.h)
	target_link_libraries(drv_uvc PRIVATE xrt-interfaces aux_os aux_util ${LIBUSB1_LIBRARIES})
	target_include_directories(drv_uvc PUBLIC ${LIBUSB1_INCLUDE_DIRS})
	list(APPEND ENABLED_DRIVERS uvc)
endif()

if(XRT_BUILD_DRIVER_ULV2)
	add_library(drv_ulv2 STATIC ultraleap_v2/ulv2_driver.cpp ultraleap_v2/ulv2_interface.h)
	target_link_libraries(drv_ulv2 PRIVATE xrt-interfaces aux_util aux_math LeapV2::LeapV2)
endif()

if(XRT_BUILD_DRIVER_ULV5)
	add_library(drv_ulv5 STATIC ultraleap_v5/ulv5_driver.cpp ultraleap_v5/ulv5_interface.h)
	target_link_libraries(drv_ulv5 PRIVATE xrt-interfaces aux_util aux_math LeapSDK::LeapC)
endif()

if(XRT_BUILD_DRIVER_OHMD)
	add_library(
		drv_ohmd STATIC ohmd/oh_device.c ohmd/oh_device.h ohmd/oh_interface.h
				ohmd/oh_prober.c
		)
	target_link_libraries(drv_ohmd PRIVATE xrt-interfaces OpenHMD::OpenHMD aux_util aux_math)
	list(APPEND ENABLED_HEADSET_DRIVERS openhmd)
endif()

if(XRT_BUILD_DRIVER_OPENGLOVES)
	add_library(
		drv_opengloves STATIC
		opengloves/opengloves_interface.h
		opengloves/opengloves_device.c
		opengloves/opengloves_prober.c
		opengloves/opengloves_device.h
		opengloves/communication/serial/opengloves_serial.h
		opengloves/communication/serial/opengloves_serial.c
		opengloves/encoding/alpha_encoding.h
		opengloves/encoding/alpha_encoding.cpp
		opengloves/encoding/encoding.h
		opengloves/communication/bluetooth/opengloves_bt_serial.h
		opengloves/communication/bluetooth/opengloves_bt_serial.c
		opengloves/communication/opengloves_communication.h
		opengloves/communication/serial/opengloves_prober_serial.h
		opengloves/communication/serial/opengloves_prober_serial.c
		opengloves/communication/bluetooth/opengloves_prober_bt.h
		opengloves/communication/bluetooth/opengloves_prober_bt.c
		)
	target_link_libraries(drv_opengloves PRIVATE xrt-interfaces aux_util aux_os bluetooth)
	list(APPEND ENABLED_DRIVERS opengloves)

endif()

if(XRT_BUILD_DRIVER_PSMV)
	add_library(drv_psmv STATIC psmv/psmv_driver.c psmv/psmv_interface.h)
	target_link_libraries(
		drv_psmv
		PRIVATE xrt-interfaces aux_util
		PUBLIC aux_os aux_tracking
		)
	list(APPEND ENABLED_DRIVERS psmv)
endif()

if(XRT_BUILD_DRIVER_PSSENSE)
	add_library(
		drv_pssense STATIC pssense/pssense_driver.c pssense/pssense_interface.h
				   pssense/pssense_protocol.h pssense/pssense_led_model.h
		)
	target_link_libraries(
		drv_pssense
		PRIVATE
			xrt-interfaces
			tracking-includes
			aux_os
			aux_util
			constellation
		)
	list(APPEND ENABLED_DRIVERS pssense)
endif()

if(XRT_BUILD_DRIVER_PSVR)
	add_library(
		drv_psvr STATIC
		psvr/psvr_device.c
		psvr/psvr_device.h
		psvr/psvr_interface.h
		psvr/psvr_packet.c
		psvr/psvr_prober.c
		)
	target_link_libraries(drv_psvr PRIVATE xrt-interfaces HIDAPI::hidapi aux_util)
	target_include_directories(drv_psvr PRIVATE ${HIDAPI_INCLUDE_DIRS})
	list(APPEND ENABLED_HEADSET_DRIVERS psvr)
endif()

if(XRT_BUILD_DRIVER_PSVR2)
	add_library(
		drv_psvr2 STATIC
		psvr2/psvr2.c
		psvr2/psvr2.h
		psvr2/psvr2_interface.h
		psvr2/psvr2_prober.c
		psvr2/psvr2_protocol.h
		psvr2/psvr2_distortion.c
		psvr2/psvr2_eye.c
		)
	target_link_libraries(
		drv_psvr2
		PRIVATE
			xrt-interfaces
			aux_os
			aux_util
			aux_tracking
			${LIBUSB1_LIBRARIES}
		)
	target_include_directories(drv_psvr2 PRIVATE ${LIBUSB1_INCLUDE_DIRS})
	list(APPEND ENABLED_HEADSET_DRIVERS psvr2)
endif()

if(XRT_BUILD_DRIVER_REALSENSE)
	add_library(
		drv_realsense STATIC
		realsense/rs_ddev.c
		realsense/rs_hdev.c
		realsense/rs_prober.c
		realsense/rs_driver.h
		realsense/rs_interface.h
		)
	target_link_libraries(drv_realsense PRIVATE xrt-interfaces realsense2::realsense2 aux_util)
	list(APPEND ENABLED_HEADSET_DRIVERS rs)
endif()

if(XRT_BUILD_DRIVER_REMOTE)
	add_library(
		drv_remote STATIC
		remote/r_device.c
		remote/r_hmd.c
		remote/r_hub.c
		remote/r_interface.h
		remote/r_internal.h
		)
	target_link_libraries(drv_remote PRIVATE xrt-interfaces aux_util aux_vive base)
	if(WIN32)
		target_link_libraries(drv_remote PRIVATE ws2_32)
	endif()
	list(APPEND ENABLED_HEADSET_DRIVERS remote)
endif()

if(XRT_BUILD_DRIVER_ROKID)
	add_library(drv_rokid STATIC rokid/rokid_hmd.c rokid/rokid_interface.h)
	target_link_libraries(
		drv_rokid
		PRIVATE
			xrt-interfaces
			aux_util
			aux_os
			aux_math
			${LIBUSB1_LIBRARIES}
		)
	target_include_directories(drv_rokid PUBLIC ${LIBUSB1_INCLUDE_DIRS})
	list(APPEND ENABLED_HEADSET_DRIVERS rokid)
endif()

if(XRT_BUILD_DRIVER_RIFT)
	add_library(
		drv_rift STATIC
		rift/rift_interface.h
		rift/rift_internal.h
		rift/rift_driver.c
		rift/rift_usb.c
		rift/rift_usb.h
		rift/rift_radio.c
		rift/rift_radio.h
		rift/rift_touch_calibration.cpp
		)
	target_include_directories(drv_rift SYSTEM PRIVATE)
	target_link_libraries(
		drv_rift
		PRIVATE
			xrt-interfaces
			tracking-includes
			aux_util
			aux_util_sink
			aux_math
			base
			constellation
			xrt-external-cjson
			xrt-external-tinyceres
		)
	list(APPEND ENABLED_HEADSET_DRIVERS rift)
endif()

if(XRT_BUILD_DRIVER_RIFT_SENSOR)
	add_library(
		drv_rift_sensor STATIC
		rift_sensor/ar0134.c
		rift_sensor/ar0134.h
		rift_sensor/esp570.c
		rift_sensor/esp570.h
		rift_sensor/esp770u.c
		rift_sensor/esp770u.h
		rift_sensor/mt9v034.c
		rift_sensor/mt9v034.h
		rift_sensor/rift_sensor_interface.h
		rift_sensor/rift_sensor_internal.h
		rift_sensor/rift_sensor.c
		)
	target_include_directories(drv_rift_sensor SYSTEM PRIVATE)
	target_link_libraries(
		drv_rift_sensor
		PRIVATE
			xrt-interfaces
			tracking-includes
			aux_util
			aux_util_sink
			aux_math
			drv_includes
			drv_uvc
			${LIBUSB1_LIBRARIES}
		)
	target_include_directories(drv_rift_sensor PUBLIC ${LIBUSB1_INCLUDE_DIRS})
endif()

if(XRT_BUILD_DRIVER_RIFT_S)
	add_library(
		drv_rift_s STATIC
		rift_s/rift_s_interface.h
		rift_s/rift_s_camera.c
		rift_s/rift_s_camera.h
		rift_s/rift_s_controller.c
		rift_s/rift_s_controller.h
		rift_s/rift_s_firmware.c
		rift_s/rift_s_firmware.h
		rift_s/rift_s_hmd.c
		rift_s/rift_s_hmd.h
		rift_s/rift_s_protocol.c
		rift_s/rift_s_protocol.h
		rift_s/rift_s_radio.c
		rift_s/rift_s_radio.h
		rift_s/rift_s_tracker.c
		rift_s/rift_s_tracker.h
		rift_s/rift_s_util.cpp
		rift_s/rift_s_util.h
		rift_s/rift_s.c
		rift_s/rift_s.h
		)
	target_link_libraries(
		drv_rift_s
		PRIVATE
			xrt-interfaces
			aux_util
			aux_util_sink
			aux_math
			xrt-external-cjson
			xrt-external-tinyceres
		)
	if(XRT_BUILD_DRIVER_HANDTRACKING)
		target_link_libraries(drv_rift_s PRIVATE drv_ht)
	endif()
	list(APPEND ENABLED_HEADSET_DRIVERS rift-s)
endif()

set(VIVE_CONFIG_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/vive")
if(XRT_BUILD_DRIVER_VIVE)
	add_library(
		drv_vive STATIC
		vive/vive_device.h
		vive/vive_device.c
		vive/vive_prober.h
		vive/vive_prober.c
		vive/vive_protocol.c
		vive/vive_protocol.h
		vive/vive_controller.h
		vive/vive_controller.c
		vive/vive_lighthouse.h
		vive/vive_lighthouse.c
		vive/vive_source.h
		vive/vive_source.c
		)
	target_link_libraries(
		drv_vive
		PRIVATE
			xrt-interfaces
			aux_os
			aux_util
			aux_math
			xrt-external-cjson
			aux_vive
		)
	target_link_libraries(drv_vive PRIVATE ${ZLIB_LIBRARIES})
	target_include_directories(drv_vive PRIVATE ${ZLIB_INCLUDE_DIRS})
	list(APPEND ENABLED_HEADSET_DRIVERS vive)
endif()

if(XRT_HAVE_V4L2)
	add_library(drv_v4l2 STATIC v4l2/v4l2_driver.h v4l2/v4l2_driver.c)
	target_link_libraries(drv_v4l2 PRIVATE xrt-interfaces aux_os aux_util)
	list(APPEND ENABLED_DRIVERS v4l2)
endif()

if(XRT_BUILD_DRIVER_VF)
	add_library(drv_vf STATIC vf/vf_driver.c)
	target_link_libraries(drv_vf PRIVATE xrt-interfaces aux_os aux_util ${GST_LIBRARIES})
	target_include_directories(drv_vf PRIVATE ${GST_INCLUDE_DIRS})
	list(APPEND ENABLED_DRIVERS vf)
endif()

if(XRT_BUILD_DRIVER_HANDTRACKING)
	add_library(drv_ht STATIC ht/ht_driver.c ht/ht_interface.h)
	target_link_libraries(
		drv_ht
		PRIVATE
			xrt-interfaces
			tracking-includes
			aux_os
			aux_util
			aux_math
			t_ht_mercury
			hand_async
		)

	target_include_directories(drv_ht PRIVATE ${OpenCV_INCLUDE_DIRS})
	list(APPEND ENABLED_DRIVERS ht)
endif()

if(XRT_BUILD_DRIVER_SURVIVE)
	include(CMakePushCheckState)
	include(CheckSymbolExists)
	add_library(
		drv_survive STATIC
		survive/survive_driver.c
		survive/survive_interface.h
		survive/survive_internal.h
		survive/survive_vp2.c
		survive/survive_vp2.h
		)
	target_link_libraries(
		drv_survive
		PRIVATE
			xrt-interfaces
			aux_os
			aux_util
			aux_math
			aux_vive
			drv_includes
			drv_vp2
			PkgConfig::SURVIVE
		)

	cmake_push_check_state(RESET)
	set(CMAKE_REQUIRED_LIBRARIES PkgConfig::SURVIVE)
	check_symbol_exists(
		"survive_simple_object_charge_percet" "survive_api.h" HAVE_MISSPELLED_CHARGE_FN
		)
	cmake_pop_check_state()
	if(HAVE_MISSPELLED_CHARGE_FN)
		target_compile_definitions(drv_survive PUBLIC XR_SURVIVE_MISSPELLED_CHARGE_FN)
	endif()

	list(APPEND ENABLED_HEADSET_DRIVERS survive)
endif()

if(XRT_BUILD_DRIVER_ANDROID)
	add_library(
		drv_android STATIC android/android_prober.c android/android_prober.h
				   android/android_sensors.c android/android_sensors.h
		)
	target_link_libraries(
		drv_android
		PRIVATE
			xrt-interfaces
			aux_util
			aux_os
			aux_android
			${ANDROID_LIBRARY}
		)
	target_link_libraries(drv_android PRIVATE xrt-external-cardboard)
	list(APPEND ENABLED_DRIVERS android)
endif()

if(XRT_BUILD_DRIVER_ILLIXR)
	add_library(
		drv_illixr STATIC
		illixr/illixr_device.cpp
		illixr/illixr_interface.h
		illixr/illixr_prober.c
		illixr/illixr_component.cpp
		illixr/illixr_component.h
		)
	target_link_libraries(drv_illixr PUBLIC ${CMAKE_DL_LIBS} xrt-interfaces aux_util aux_os)
	target_include_directories(drv_illixr PUBLIC ${ILLIXR_PATH})
	list(APPEND ENABLED_HEADSET_DRIVERS illixr)
endif()

add_library(drv_multi STATIC multi_wrapper/multi.c multi_wrapper/multi.h)
target_link_libraries(drv_multi PUBLIC xrt-interfaces aux_util)
list(APPEND ENABLED_HEADSET_DRIVERS drv_multi)

if(XRT_BUILD_DRIVER_WMR)
	add_library(
		drv_wmr STATIC
		wmr/wmr_camera.h
		wmr/wmr_common.h
		wmr/wmr_config.c
		wmr/wmr_config.h
		wmr/wmr_controller_base.c
		wmr/wmr_controller_base.h
		wmr/wmr_controller_og.c
		wmr/wmr_controller_hp.c
		wmr/wmr_controller.c
		wmr/wmr_controller.h
		wmr/wmr_bt_controller.c
		wmr/wmr_bt_controller.h
		wmr/wmr_hmd.c
		wmr/wmr_hmd.h
		wmr/wmr_hmd_controller.c
		wmr/wmr_hmd_controller.h
		wmr/wmr_interface.h
		wmr/wmr_prober.c
		wmr/wmr_protocol.c
		wmr/wmr_protocol.h
		wmr/wmr_controller_protocol.c
		wmr/wmr_controller_protocol.h
		wmr/wmr_source.c
		wmr/wmr_source.h
		)
	target_link_libraries(
		drv_wmr
		PRIVATE
			xrt-interfaces
			aux_util
			aux_util_sink # TODO remove this when converted to builders.
			aux_math
			aux_tracking
			xrt-external-cjson
		)
	list(APPEND ENABLED_HEADSET_DRIVERS wmr)

	# Can only build camera support with libusb
	if(XRT_HAVE_LIBUSB)
		target_sources(drv_wmr PRIVATE wmr/wmr_camera.c)
		target_include_directories(drv_wmr PUBLIC ${LIBUSB1_INCLUDE_DIRS})
		target_link_libraries(drv_wmr PRIVATE ${LIBUSB1_LIBRARIES})
	endif()

	if(XRT_BUILD_DRIVER_HANDTRACKING)
		target_link_libraries(drv_wmr PRIVATE drv_ht)
		target_link_libraries(drv_wmr PRIVATE drv_cemu)
	endif()
endif()

if(XRT_BUILD_DRIVER_EUROC)
	add_library(
		drv_euroc STATIC
		euroc/euroc_player.cpp
		euroc/euroc_driver.h
		euroc/euroc_device.c
		euroc/euroc_interface.h
		euroc/euroc_runner.c
		)
	target_link_libraries(
		drv_euroc PRIVATE xrt-interfaces aux_util aux_tracking ${OpenCV_LIBRARIES}
		)
	target_include_directories(drv_euroc PRIVATE ${OpenCV_INCLUDE_DIRS})
	list(APPEND ENABLED_DRIVERS euroc)
endif()

if(XRT_BUILD_DRIVER_SIMULAVR)
	add_library(drv_svr STATIC simula/svr_hmd.c simula/svr_interface.h)
	target_link_libraries(drv_svr PRIVATE xrt-interfaces aux_math xrt-external-cjson)
	list(APPEND ENABLED_HEADSET_DRIVERS svr)
endif()

if(XRT_BUILD_DRIVER_STEAMVR_LIGHTHOUSE OR XRT_BUILD_DRIVER_SURVIVE)
	add_library(
		drv_vp2 STATIC
		vp2/vp2_hid.c
		vp2/vp2_hid.h
		vp2/vp2_config.cpp
		vp2/vp2_config.h
		vp2/vp2_protocol.h
		)
	target_link_libraries(
		drv_vp2
		PRIVATE
			aux_os
			aux_util
			aux_math
			aux_vive
			aux_htc
			xrt-external-cjson
		)
endif()

if(XRT_BUILD_DRIVER_STEAMVR_LIGHTHOUSE)
	add_library(
		drv_steamvr_lh STATIC
		steamvr_lh/steamvr_lh.cpp
		steamvr_lh/device.cpp
		steamvr_lh/interfaces/driver_manager.cpp
		steamvr_lh/interfaces/iobuffer.cpp
		steamvr_lh/interfaces/resources.cpp
		steamvr_lh/interfaces/settings.cpp
		steamvr_lh/interfaces/blockqueue.cpp
		steamvr_lh/interfaces/paths.cpp
		)
	target_link_libraries(
		drv_steamvr_lh
		PRIVATE
			aux_util
			aux_math
			aux_vive
			drv_vp2
			drv_includes
			base
			xrt-interfaces
			xrt-external-openvr
			xrt-external-cjson
			xrt-external-vdf
			${CMAKE_DL_LIBS}
		)
	list(APPEND ENABLED_HEADSET_DRIVERS steamvr_lh)
endif()

if(XRT_BUILD_DRIVER_SOLARXR)
	add_library(
		drv_solarxr STATIC
		solarxr/feeder.c
		solarxr/feeder.h
		solarxr/protocol.c
		solarxr/protocol.h
		solarxr/solarxr_device.c
		solarxr/solarxr_interface.h
		solarxr/solarxr_ipc_message.h
		solarxr/solarxr_ipc_socket.c
		solarxr/solarxr_ipc_socket.h
		)
	target_link_libraries(drv_solarxr PRIVATE xrt-interfaces aux_util ipc_shared)
	list(APPEND ENABLED_DRIVERS solarxr)
endif()

if(XRT_BUILD_SAMPLES)
	# We build the sample driver to make sure it stays valid,
	# but it never gets linked into a final target.
	add_library(
		drv_sample STATIC sample/sample_hmd.c sample/sample_interface.h
				  sample/sample_prober.c
		)
	target_link_libraries(drv_sample PRIVATE xrt-interfaces aux_util)
endif()

if(ENABLED_HEADSET_DRIVERS)
	set(ENABLED_DRIVERS ${ENABLED_HEADSET_DRIVERS} ${ENABLED_DRIVERS})
	list(SORT ENABLED_DRIVERS)
	string(REPLACE ";" " " ENABLED_DRIVERS "${ENABLED_DRIVERS}")
	message(STATUS "Enabled drivers: ${ENABLED_DRIVERS}")
else()
	message(FATAL_ERROR "You must enable at least one headset driver to build Monado.")
endif()
