// Copyright 2021-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Simple @ref xrt_frame wrapper around a cv::Mat.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_tracking
 */

#include "xrt/xrt_frame.h"

#include <opencv2/opencv.hpp>


namespace xrt::auxiliary::tracking {


/*!
 * This class implements the @ref xrt_frame interface, allowing interfacing to
 * @p cv::Mat from C code. Keeps a reference to the cv::Mat and so the data
 * alive, this class is freed when the reference count reaches zero.
 */
class FrameMat
{
public:
	//! Additional optional parameters for frame creation.
	class Params
	{
	public:
		enum xrt_stereo_format stereo_format;
		uint64_t timestamp_ns;
	};

	//! Exposed @ref xrt_frame API.
	struct xrt_frame frame = {};

	// The cv::Mat that holds the data.
	cv::Mat matrix = cv::Mat();


public: // Methods
	/*!
	 * Wraps the given cv::Mat assuming it's a 24bit RGB format matrix.
	 * In all but the most strange cases you probably want the pointer
	 * pointed to by @p xf_ptr to be `nullptr`, if not `nullptr` it will have
	 * its reference count decremented so make sure it's a valid pointer.
	 */
	static void
	wrapR8G8B8(const cv::Mat &mat, xrt_frame **fm_out, Params params = {});

	/*!
	 * Wraps the given cv::Mat assuming it's a 8bit format matrix.
	 * In all but the most strange cases you probably want the pointer
	 * pointed to by @p xf_ptr to be `nullptr`, if not `nullptr` it will have
	 * its reference count decremented so make sure it's a valid pointer.
	 */
	static void
	wrapL8(const cv::Mat &mat, xrt_frame **fm_out, Params params = {});


private:
	~FrameMat();
	FrameMat();

	void
	fillInFields(cv::Mat mat, xrt_format format, const Params &params);

	//! C callback used when the reference of the frames reaches zero.
	static void
	destroyFrame(xrt_frame *frame);
};



} // namespace xrt::auxiliary::tracking
