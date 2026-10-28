// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Tests for glue/g_frameserver.hpp and glue/g_frame_node.hpp
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 */

#include "glue/g_frameserver.hpp"
#include "glue/g_frame_node.hpp"

#include "catch_amalgamated.hpp"

#include <stdexcept>


//! Test frameserver with minimal functions
constexpr xrt::util::FrameServerFunctions kMinimalFunctions = {};

//! Test frameserver with SLAM enabled
constexpr xrt::util::FrameServerFunctions kSlamFunctions = {
    .slam = true,
};


/*!
 * Minimal test frameserver, also a frame node.
 */
class MinimalTestFrameServer : public xrt::util::FrameServerBase<MinimalTestFrameServer, kMinimalFunctions>,
                               public xrt::util::FrameNodeBase<MinimalTestFrameServer>
{
public:
	bool running = false;
	bool broken_apart = false;
	bool destroyed = false;
	int enumerate_modes_count = 0;
	int configure_capture_count = 0;
	xrt_frame_sink *sink = nullptr;
	xrt_fs_capture_type capture_type = XRT_FS_CAPTURE_TYPE_TRACKING;
	uint32_t descriptor_index = 0;

	xrt_result_t
	enumerateModes(struct xrt_fs_mode **out_modes, uint32_t *out_count)
	{
		enumerate_modes_count++;
		*out_modes = nullptr;
		*out_count = 0;
		return XRT_SUCCESS;
	}

	xrt_result_t
	configureCapture(struct xrt_fs_capture_parameters *cp)
	{
		configure_capture_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	streamStart(struct xrt_frame_sink *xs, enum xrt_fs_capture_type type, uint32_t index)
	{
		sink = xs;
		capture_type = type;
		descriptor_index = index;
		running = true;
		return XRT_SUCCESS;
	}

	xrt_result_t
	streamStop()
	{
		running = false;
		return XRT_SUCCESS;
	}

	xrt_result_t
	isRunning(bool &is_running)
	{
		is_running = this->running;
		return XRT_SUCCESS;
	}

	void
	breakApart()
	{
		broken_apart = true;
	}

	static void
	destroyFrameNode(struct xrt_frame_node *node)
	{
		auto *fs = MinimalTestFrameServer::fromFrameNode(node);
		fs->destroyed = true;
	}
};

/*!
 * SLAM test frameserver.
 */
class SlamTestFrameServer : public xrt::util::FrameServerBase<SlamTestFrameServer, kSlamFunctions>
{
public:
	int slam_stream_start_count = 0;

	xrt_result_t
	enumerateModes(struct xrt_fs_mode **out_modes, uint32_t *out_count)
	{
		return XRT_ERROR_NOT_IMPLEMENTED;
	}

	xrt_result_t
	configureCapture(struct xrt_fs_capture_parameters *cp)
	{
		return XRT_ERROR_NOT_IMPLEMENTED;
	}

	xrt_result_t
	streamStart(struct xrt_frame_sink *xs, enum xrt_fs_capture_type type, uint32_t index)
	{
		return XRT_ERROR_NOT_IMPLEMENTED;
	}

	xrt_result_t
	slamStreamStart(struct xrt_slam_sinks *sinks)
	{
		slam_stream_start_count++;
		return XRT_SUCCESS;
	}

	xrt_result_t
	streamStop()
	{
		return XRT_ERROR_NOT_IMPLEMENTED;
	}

	xrt_result_t
	isRunning(bool &is_running)
	{
		return XRT_ERROR_NOT_IMPLEMENTED;
	}
};

/*!
 * Frameserver that throws from every callback.
 */
class ThrowingTestFrameServer : public xrt::util::FrameServerBase<ThrowingTestFrameServer, kMinimalFunctions>
{
public:
	xrt_result_t
	enumerateModes(struct xrt_fs_mode **out_modes, uint32_t *out_count)
	{
		throw std::runtime_error("enumerateModes");
	}

	xrt_result_t
	configureCapture(struct xrt_fs_capture_parameters *cp)
	{
		throw std::runtime_error("configureCapture");
	}

	xrt_result_t
	streamStart(struct xrt_frame_sink *xs, enum xrt_fs_capture_type type, uint32_t index)
	{
		throw std::runtime_error("streamStart");
	}

	xrt_result_t
	streamStop()
	{
		throw std::runtime_error("streamStop");
	}

	xrt_result_t
	isRunning(bool &is_running)
	{
		throw std::runtime_error("isRunning");
	}
};


TEST_CASE("FrameServerBase_Minimal")
{
	MinimalTestFrameServer fs;
	auto *xfs = fs.getXFS();

	SECTION("Function pointers are set")
	{
		REQUIRE(xfs->enumerate_modes != nullptr);
		REQUIRE(xfs->configure_capture != nullptr);
		REQUIRE(xfs->stream_start != nullptr);
		REQUIRE(xfs->slam_stream_start != nullptr);
		REQUIRE(xfs->stream_stop != nullptr);
		REQUIRE(xfs->is_running != nullptr);
	}

	SECTION("Disabled optional functions fail cleanly")
	{
		REQUIRE(xfs->slam_stream_start(xfs, nullptr) == XRT_ERROR_NOT_IMPLEMENTED);
	}

	SECTION("Identifying fields are zeroed")
	{
		REQUIRE(xfs->name[0] == '\0');
		REQUIRE(xfs->product[0] == '\0');
		REQUIRE(xfs->manufacturer[0] == '\0');
		REQUIRE(xfs->serial[0] == '\0');
		REQUIRE(xfs->source_id == 0);
	}

	SECTION("fromXFS conversion works")
	{
		REQUIRE(MinimalTestFrameServer::fromXFS(xfs) == &fs);
	}

	SECTION("Functions can be called")
	{
		xrt_fs_mode *modes = nullptr;
		uint32_t count = 1;
		REQUIRE(xrt_fs_enumerate_modes(xfs, &modes, &count) == XRT_SUCCESS);
		REQUIRE(fs.enumerate_modes_count == 1);
		REQUIRE(count == 0);

		xrt_fs_capture_parameters cp = {};
		REQUIRE(xrt_fs_configure_capture(xfs, &cp) == XRT_SUCCESS);
		REQUIRE(fs.configure_capture_count == 1);

		xrt_frame_sink sink = {};
		bool running;
		REQUIRE(xrt_fs_is_running(xfs, &running) == XRT_SUCCESS);
		REQUIRE_FALSE(running);
		REQUIRE(xrt_fs_stream_start(xfs, &sink, XRT_FS_CAPTURE_TYPE_CALIBRATION, 3) == XRT_SUCCESS);
		REQUIRE(xrt_fs_is_running(xfs, &running) == XRT_SUCCESS);
		REQUIRE(running);
		REQUIRE(fs.sink == &sink);
		REQUIRE(fs.capture_type == XRT_FS_CAPTURE_TYPE_CALIBRATION);
		REQUIRE(fs.descriptor_index == 3);

		REQUIRE(xrt_fs_stream_stop(xfs) == XRT_SUCCESS);
		REQUIRE(xrt_fs_is_running(xfs, &running) == XRT_SUCCESS);
		REQUIRE_FALSE(running);
	}
}

TEST_CASE("FrameServerBase_Slam")
{
	SlamTestFrameServer fs;
	auto *xfs = fs.getXFS();

	REQUIRE(xfs->slam_stream_start != nullptr);
	REQUIRE(xrt_fs_slam_stream_start(xfs, nullptr) == XRT_SUCCESS);
	REQUIRE(fs.slam_stream_start_count == 1);
}

TEST_CASE("FrameServerBase_Exceptions")
{
	ThrowingTestFrameServer fs;
	auto *xfs = fs.getXFS();

	xrt_fs_mode *modes = nullptr;
	uint32_t count = 0;
	xrt_fs_capture_parameters cp = {};

	bool running;
	REQUIRE(xrt_fs_enumerate_modes(xfs, &modes, &count) == XRT_ERROR_UNCAUGHT_EXCEPTION);
	REQUIRE(xrt_fs_configure_capture(xfs, &cp) == XRT_ERROR_UNCAUGHT_EXCEPTION);
	REQUIRE(xrt_fs_stream_start(xfs, nullptr, XRT_FS_CAPTURE_TYPE_TRACKING, 0) == XRT_ERROR_UNCAUGHT_EXCEPTION);
	REQUIRE(xrt_fs_stream_stop(xfs) == XRT_ERROR_UNCAUGHT_EXCEPTION);
	REQUIRE(xrt_fs_is_running(xfs, &running) == XRT_ERROR_UNCAUGHT_EXCEPTION);
}

TEST_CASE("FrameNodeBase_WithFrameServer")
{
	MinimalTestFrameServer fs;
	auto *node = fs.getFrameNode();

	SECTION("Function pointers are set")
	{
		REQUIRE(node->break_apart != nullptr);
		REQUIRE(node->destroy != nullptr);
	}

	SECTION("Both bases recover the same object")
	{
		REQUIRE(MinimalTestFrameServer::fromFrameNode(node) == &fs);
		REQUIRE(MinimalTestFrameServer::fromXFS(fs.getXFS()) == &fs);
	}

	SECTION("Frame context calls break apart then destroy")
	{
		xrt_frame_context xfctx = {};
		xrt_frame_context_add(&xfctx, node);

		REQUIRE_FALSE(fs.broken_apart);
		REQUIRE_FALSE(fs.destroyed);

		xrt_frame_context_destroy_nodes(&xfctx);

		REQUIRE(fs.broken_apart);
		REQUIRE(fs.destroyed);
		REQUIRE(xfctx.nodes == nullptr);
	}
}
