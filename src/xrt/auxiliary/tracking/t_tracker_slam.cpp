// Copyright 2021-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief SLAM tracking code.
 * @author Mateo de Mayo <mateo.demayo@collabora.com>
 * @ingroup aux_tracking
 */

#include "xrt/xrt_config_have.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_tracking.h"
#include "xrt/xrt_frameserver.h"
#include "util/u_debug.h"
#include "util/u_logging.h"
#include "util/u_misc.h"
#include "util/u_sink.h"
#include "util/u_var.h"
#include "util/u_trace_marker.h"
#include "os/os_threading.h"
#include "math/m_api.h"
#include "math/m_filter_fifo.h"
#include "math/m_filter_one_euro.h"
#include "math/m_predict.h"
#include "math/m_relation_history.h"
#include "math/m_space.h"
#include "math/m_vec3.h"
#include "tracking/t_euroc_recorder.h"
#include "tracking/t_openvr_tracker.h"
#include "tracking/t_tracking.h"
#include "tracking/t_vit_loader.h"

#include "vit/vit_interface.h"

#include <opencv2/core/mat.hpp>
#include <opencv2/core/version.hpp>

#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <string>
#include <vector>

//! @todo Get preferred system from systems found at build time
#define PREFERRED_VIT_SYSTEM_LIBRARY "libbasalt.so"

#define SLAM_TRACE(...) U_LOG_IFL_T(t.log_level, __VA_ARGS__)
#define SLAM_DEBUG(...) U_LOG_IFL_D(t.log_level, __VA_ARGS__)
#define SLAM_INFO(...) U_LOG_IFL_I(t.log_level, __VA_ARGS__)
#define SLAM_WARN(...) U_LOG_IFL_W(t.log_level, __VA_ARGS__)
#define SLAM_ERROR(...) U_LOG_IFL_E(t.log_level, __VA_ARGS__)
#define SLAM_ASSERT(predicate, ...)                                                                                    \
	do {                                                                                                           \
		bool p = predicate;                                                                                    \
		if (!p) {                                                                                              \
			U_LOG(U_LOGGING_ERROR, __VA_ARGS__);                                                           \
			assert(false && "SLAM_ASSERT failed: " #predicate);                                            \
			exit(EXIT_FAILURE);                                                                            \
		}                                                                                                      \
	} while (false);
#define SLAM_ASSERT_(predicate) SLAM_ASSERT(predicate, "Assertion failed " #predicate)

// Debug assertions, not vital but useful for finding errors
#ifdef NDEBUG
#define SLAM_DASSERT(predicate, ...) (void)(predicate)
#define SLAM_DASSERT_(predicate) (void)(predicate)
#else
#define SLAM_DASSERT(predicate, ...) SLAM_ASSERT(predicate, __VA_ARGS__)
#define SLAM_DASSERT_(predicate) SLAM_ASSERT_(predicate)
#endif

//! @see t_slam_tracker_config
DEBUG_GET_ONCE_LOG_OPTION(slam_log, "SLAM_LOG", U_LOGGING_INFO)
DEBUG_GET_ONCE_OPTION(vit_system_library_path, "VIT_SYSTEM_LIBRARY_PATH", PREFERRED_VIT_SYSTEM_LIBRARY)
DEBUG_GET_ONCE_OPTION(slam_config, "SLAM_CONFIG", nullptr)
DEBUG_GET_ONCE_BOOL_OPTION(slam_ui, "SLAM_UI", false)
DEBUG_GET_ONCE_BOOL_OPTION(slam_submit_from_start, "SLAM_SUBMIT_FROM_START", false)
DEBUG_GET_ONCE_NUM_OPTION(slam_openvr_groundtruth_device, "SLAM_OPENVR_GROUNDTRUTH_DEVICE", 0)
DEBUG_GET_ONCE_NUM_OPTION(slam_prediction_type, "SLAM_PREDICTION_TYPE", long(SLAM_PRED_IP_IO_IA_IL))
DEBUG_GET_ONCE_BOOL_OPTION(slam_write_csvs, "SLAM_WRITE_CSVS", false)
DEBUG_GET_ONCE_OPTION(slam_csv_path, "SLAM_CSV_PATH", "evaluation/")
DEBUG_GET_ONCE_BOOL_OPTION(slam_timing_stat, "SLAM_TIMING_STAT", true)
DEBUG_GET_ONCE_BOOL_OPTION(slam_features_stat, "SLAM_FEATURES_STAT", true)
DEBUG_GET_ONCE_NUM_OPTION(slam_cam_count, "SLAM_CAM_COUNT", 2)

//! Namespace for the interface to the external SLAM tracking system
namespace xrt::auxiliary::tracking::slam {
constexpr int UI_TIMING_POSE_COUNT = 192;
constexpr int UI_FEATURES_POSE_COUNT = 192;
constexpr int UI_GTDIFF_POSE_COUNT = 192;

using os::Mutex;
using std::deque;
using std::ifstream;
using std::make_shared;
using std::map;
using std::ofstream;
using std::ostream;
using std::pair;
using std::shared_ptr;
using std::string;
using std::to_string;
using std::unique_lock;
using std::vector;
using std::filesystem::create_directories;
using Trajectory = map<timepoint_ns, xrt_pose>;
using timing_sample = vector<timepoint_ns>;

using xrt::auxiliary::math::RelationHistory;


/*
 *
 * CSV Writers
 *
 */

ostream &
operator<<(ostream &os, const xrt_pose_sample &s)
{
	timepoint_ns ts = s.timestamp_ns;
	xrt_vec3 p = s.pose.position;
	xrt_quat r = s.pose.orientation;
	os << ts << ",";
	os << p.x << "," << p.y << "," << p.z << ",";
	os << r.w << "," << r.x << "," << r.y << "," << r.z << CSV_EOL;
	return os;
}

ostream &
operator<<(ostream &os, const timing_sample &timestamps)
{
	for (const timepoint_ns &ts : timestamps) {
		string delimiter = &ts != &timestamps.back() ? "," : CSV_EOL;
		os << ts << delimiter;
	}
	return os;
}

struct feature_count_sample
{
	timepoint_ns ts;
	vector<int> counts;
};

ostream &
operator<<(ostream &os, const feature_count_sample &s)
{
	os << s.ts;
	for (int count : s.counts) {
		os << "," << count;
	}
	os << CSV_EOL;
	return os;
}

//! Writes a CSV file for a particular row type
template <typename RowType> class CSVWriter
{
public:
	bool enabled; // Modified through UI

protected:
	vector<string> column_names;

private:
	string directory;
	string filename;
	ofstream file;
	bool created = false;
	Mutex mutex;

	void
	create()
	{
		create_directories(directory);
		file = ofstream{directory + "/" + filename};
		file << "#";
		for (const string &col : column_names) {
			string delimiter = &col != &column_names.back() ? "," : CSV_EOL;
			file << col << delimiter;
		}
		file << std::fixed << std::setprecision(CSV_PRECISION);
	}

public:
	CSVWriter(const string &dir, const string &fn, bool e, const vector<string> &cn = {})
	    : enabled(e), column_names(cn), directory(dir), filename(fn)
	{}

	virtual ~CSVWriter() {}

	void
	push(RowType row)
	{
		unique_lock lock(mutex);

		if (!enabled) {
			return;
		}

		if (!created) {
			created = true;
			create();
		}

		file << row;
	}
};

//! Writes poses and their timestamps to a CSV file
struct TrajectoryWriter : public CSVWriter<xrt_pose_sample>
{
	TrajectoryWriter(const string &dir, const string &fn, bool e) : CSVWriter<xrt_pose_sample>(dir, fn, e)
	{
		column_names = {"timestamp [ns]", "p_RS_R_x [m]", "p_RS_R_y [m]", "p_RS_R_z [m]",
		                "q_RS_w []",      "q_RS_x []",    "q_RS_y []",    "q_RS_z []"};
	}
};

//! Writes timestamps measured when estimating a new pose by the SLAM system
struct TimingWriter : public CSVWriter<timing_sample>
{
	TimingWriter(const string &dir, const string &fn, bool e, const vector<string> &cn)
	    : CSVWriter<timing_sample>(dir, fn, e, cn)
	{}
};

//! Writes feature information specific to a particular estimated pose
struct FeaturesWriter : public CSVWriter<feature_count_sample>
{
	FeaturesWriter(const string &dir, const string &fn, bool e, size_t cam_count)
	    : CSVWriter<feature_count_sample>(dir, fn, e)
	{
		column_names.push_back("timestamp");
		for (size_t i = 0; i < cam_count; i++) {
			column_names.push_back("cam" + to_string(i) + " feature count");
		}
	}
};

/*!
 * Main implementation of @ref xrt_tracked_slam. This is an adapter class for
 * SLAM tracking that wraps an external SLAM implementation.
 *
 * @implements xrt_tracked_slam
 * @implements xrt_frame_node
 * @implements xrt_frame_sink
 * @implements xrt_imu_sink
 * @implements xrt_pose_sink
 */
struct TrackerSlam
{
	struct xrt_tracked_slam base = {};
	struct xrt_frame_node node = {};       //!< Will be called on destruction
	struct t_vit_bundle vit;               //!< VIT system function pointers
	enum vit_tracker_pose_capability caps; //!< VIT tracker bitfield capabilities
	struct vit_tracker *tracker;           //!< Pointer to the tracker created by the loaded VIT system;

	struct xrt_slam_sinks sinks = {};                            //!< Pointers to the sinks below
	struct xrt_frame_sink cam_sinks[XRT_TRACKING_MAX_SLAM_CAMS]; //!< Sends camera frames to the SLAM system
	struct xrt_imu_sink imu_sink = {};                           //!< Sends imu samples to the SLAM system
	struct xrt_pose_sink gt_sink = {};                           //!< Register groundtruth trajectory for stats
	struct xrt_hand_masks_sink hand_masks_sink = {};             //!< Register latest masks to ignore

	bool submit;        //!< Whether to submit data pushed to sinks to the SLAM tracker
	uint32_t cam_count; //!< Number of cameras used for tracking

	struct u_var_button reset_state_btn; //!< Reset tracker state button

	enum u_logging_level log_level; //!< Logging level for the SLAM tracker, set by SLAM_LOG var

	struct xrt_slam_sinks *euroc_recorder; //!< EuRoC dataset recording sinks
	struct openvr_tracker *ovr_tracker;    //!< OpenVR lighthouse tracker

	// Used mainly for checking that the timestamps come in order
	timepoint_ns last_imu_ts;                     //!< Last received IMU sample timestamp
	vector<timepoint_ns> last_cam_ts;             //!< Last received image timestamp per cam
	struct xrt_hand_masks_sample last_hand_masks; //!< Last received hand masks info
	Mutex last_hand_masks_mutex;                  //!< Mutex for @ref last_hand_masks

	// Prediction

	//! Type of prediction to use
	t_slam_prediction_type pred_type;
	u_var_combo pred_combo;         //!< UI combo box to select @ref pred_type
	RelationHistory slam_rels{};    //!< A history of relations produced purely from external SLAM tracker data
	int dbg_pred_every = 1;         //!< Skip X SLAM poses so that you get tracked mostly by the prediction algo
	int dbg_pred_counter = 0;       //!< SLAM pose counter for prediction debugging
	struct os_mutex lock_ff;        //!< Lock for gyro_ff and accel_ff.
	struct m_ff_vec3_f32 *gyro_ff;  //!< Last gyroscope samples
	struct m_ff_vec3_f32 *accel_ff; //!< Last accelerometer samples
	vector<u_sink_debug> ui_sink;   //!< Sink to display frames in UI of each camera

	//! Used to correct accelerometer measurements when integrating into the prediction.
	//! @todo Should be automatically computed instead of required to be filled manually through the UI.
	xrt_vec3 gravity_correction{0, 0, -MATH_GRAVITY_M_S2};

	struct xrt_space_relation last_rel = XRT_SPACE_RELATION_ZERO; //!< Last reported/tracked pose
	timepoint_ns last_ts;                                         //!< Last reported/tracked pose timestamp

	//! Filters are used to smooth out the resulting trajectory
	struct
	{
		// Moving average filter
		bool use_moving_average_filter = false;
		//! Time window in ms take the average on.
		//! Increasing it smooths out the tracking at the cost of adding delay.
		double window = 66;
		struct m_ff_vec3_f32 *pos_ff; //! Predicted positions fifo
		struct m_ff_vec3_f32 *rot_ff; //! Predicted rotations fifo (only xyz components, w is inferred)

		// Exponential smoothing filter
		bool use_exponential_smoothing_filter = false;
		float alpha = 0.1; //!< How much should we lerp towards the @ref target value on each update
		struct xrt_space_relation last = XRT_SPACE_RELATION_ZERO;   //!< Last filtered relation
		struct xrt_space_relation target = XRT_SPACE_RELATION_ZERO; //!< Target relation

		// One euro filter
		bool use_one_euro_filter = false;
		m_filter_euro_vec3 pos_oe;     //!< One euro position filter
		m_filter_euro_quat rot_oe;     //!< One euro rotation filter
		const float min_cutoff = M_PI; //!< Default minimum cutoff frequency
		const float min_dcutoff = 1;   //!< Default minimum cutoff frequency for the derivative
		const float beta = 0.16;       //!< Default speed coefficient

	} filter;

	// Stats and metrics

	// CSV writers for offline analysis (using pointers because of container_of)
	TimingWriter *slam_times_writer;      //!< Timestamps of the pipeline for performance analysis
	FeaturesWriter *slam_features_writer; //!< Feature tracking information for analysis
	TrajectoryWriter *slam_traj_writer;   //!< Estimated poses from the SLAM system
	TrajectoryWriter *pred_traj_writer;   //!< Predicted poses
	TrajectoryWriter *filt_traj_writer;   //!< Predicted and filtered poses

	//! Tracker timing info for performance evaluation
	struct
	{
		bool enabled = false;               //!< Whether the timing extension is enabled
		float dur_ms[UI_TIMING_POSE_COUNT]; //!< Timing durations in ms
		int idx = 0;                        //!< Index of latest entry in @ref dur_ms
		u_var_combo start_ts;               //!< UI combo box to select initial timing measurement
		u_var_combo end_ts;                 //!< UI combo box to select final timing measurement
		int start_ts_idx;                   //!< Selected initial timing measurement in @ref start_ts
		int end_ts_idx;                     //!< Selected final timing measurement in @ref end_ts
		struct u_var_timing ui;             //!< Realtime UI for tracker durations
		vector<string> columns;             //!< Column names of the measured timestamps
		string joined_columns;              //!< Column names as a null separated string
		struct u_var_button enable_btn;     //!< Toggle tracker timing reports
	} timing;

	//! Tracker feature tracking info
	struct Features
	{
		struct FeatureCounter
		{
			//! Feature count for each frame timestamp for this camera.
			//! @note Harmless race condition over this as the UI might read this while it's being written
			deque<pair<timepoint_ns, int>> entries{};

			//! Persistently stored camera name for display in the UI
			string cam_name;

			void
			addFeatureCount(timepoint_ns ts, int count)
			{
				entries.emplace_back(ts, count);
				if (entries.size() > UI_FEATURES_POSE_COUNT) {
					entries.pop_front();
				}
			}
		};

		vector<FeatureCounter> fcs; //!< Store feature count info for each camera
		u_var_curves fcs_ui;        //!< Display of `fcs` in UI

		bool enabled = false;           //!< Whether the features extension is enabled
		struct u_var_button enable_btn; //!< Toggle extension
	} features;

	//! Ground truth related fields
	struct
	{
		Trajectory *trajectory;               //!< Empty if we've not received groundtruth
		xrt_pose origin;                      //!< First ground truth pose
		float diffs_mm[UI_GTDIFF_POSE_COUNT]; //!< Positional error wrt ground truth
		int diff_idx = 0;                     //!< Index of last error in @ref diffs_mm
		struct u_var_timing diff_ui;          //!< Realtime UI for positional error
		bool override_tracking = false;       //!< Force the tracker to report gt poses instead
	} gt;
};


/*
 *
 * Timing functionality
 *
 */

static void
timing_ui_setup(TrackerSlam &t)
{
	t.timing.enabled = false;

	u_var_add_ro_ftext(&t, "\n%s", "Tracker timing");

	// Setup toggle button
	static const char *msg[2] = {"[OFF] Enable timing", "[ON] Disable timing"};
	u_var_button_cb cb = [](void *t_ptr) {
		TrackerSlam *t = (TrackerSlam *)t_ptr;
		u_var_button &btn = t->timing.enable_btn;
		bool e = !t->timing.enabled;
		snprintf(btn.label, sizeof(btn.label), "%s", msg[e]);
		vit_result_t vres =
		    t->vit.tracker_set_pose_capabilities(t->tracker, VIT_TRACKER_POSE_CAPABILITY_TIMING, e);
		if (vres != VIT_SUCCESS) {
			U_LOG_IFL_E(t->log_level, "Failed to set tracker timing capability");
			return;
		}
		t->timing.enabled = e;
	};
	t.timing.enable_btn.cb = cb;
	t.timing.enable_btn.disabled = (t.caps & VIT_TRACKER_POSE_CAPABILITY_TIMING) == 0;
	t.timing.enable_btn.ptr = &t;
	u_var_add_button(&t, &t.timing.enable_btn, msg[t.timing.enabled]);

	// We provide two timing columns by default, even if there is no extension support
	t.timing.columns = {"sampled", "received_by_monado"};

	// Only fill the timing columns if the tracker supports pose timing
	if ((t.caps & VIT_TRACKER_POSE_CAPABILITY_TIMING) != 0) {
		vit_tracker_timing_titles titles = {};
		vit_result_t vres = t.vit.tracker_get_timing_titles(t.tracker, &titles);
		if (vres != VIT_SUCCESS) {
			SLAM_ERROR("Failed to get timing titles from tracker");
			return;
		}

		// Copies the titles locally.
		std::vector<std::string> cols(titles.titles, titles.titles + titles.count);

		t.timing.columns.insert(t.timing.columns.begin() + 1, cols.begin(), cols.end());
	}

	// Construct null-separated array of options for the combo box
	using namespace std::string_literals;
	t.timing.joined_columns = "";
	for (const string &name : t.timing.columns) {
		t.timing.joined_columns += name + "\0"s;
	}
	t.timing.joined_columns += "\0"s;

	t.timing.start_ts.count = t.timing.columns.size();
	t.timing.start_ts.options = t.timing.joined_columns.c_str();
	t.timing.start_ts.value = &t.timing.start_ts_idx;
	t.timing.start_ts_idx = 0;
	u_var_add_combo(&t, &t.timing.start_ts, "Start timestamp");

	t.timing.end_ts.count = t.timing.columns.size();
	t.timing.end_ts.options = t.timing.joined_columns.c_str();
	t.timing.end_ts.value = &t.timing.end_ts_idx;
	t.timing.end_ts_idx = t.timing.columns.size() - 1;
	u_var_add_combo(&t, &t.timing.end_ts, "End timestamp");

	t.timing.ui.values.data = t.timing.dur_ms;
	t.timing.ui.values.length = UI_TIMING_POSE_COUNT;
	t.timing.ui.values.index_ptr = &t.timing.idx;
	t.timing.ui.reference_timing = 16.6;
	t.timing.ui.center_reference_timing = true;
	t.timing.ui.range = t.timing.ui.reference_timing;
	t.timing.ui.dynamic_rescale = true;
	t.timing.ui.unit = "ms";
	u_var_add_f32_timing(&t, &t.timing.ui, "External tracker times");
}

//! Updates timing UI with info from a computed pose and returns that info
static vector<timepoint_ns>
timing_ui_push(TrackerSlam &t, const vit_pose_t *pose, int64_t ts)
{
	timepoint_ns now = os_monotonic_get_ns();
	vector<timepoint_ns> tss = {ts, now};

	// Add extra timestamps if the SLAM tracker provides them
	if (t.timing.enabled) {
		vit_pose_timing timing;
		vit_result_t vres = t.vit.pose_get_timing(pose, &timing);
		if (vres != VIT_SUCCESS) {
			// Even if the timing is enabled, some of the poses already in the queue won't have it enabled.
			if (vres != VIT_ERROR_NOT_ENABLED) {
				SLAM_ERROR("Failed to get pose timing");
			}

			return {};
		}

		std::vector<int64_t> data(timing.timestamps, timing.timestamps + timing.count);
		tss.insert(tss.begin() + 1, data.begin(), data.end());

		// The two timestamps to compare in the graph
		timepoint_ns start = tss.at(t.timing.start_ts_idx);
		timepoint_ns end = tss.at(t.timing.end_ts_idx);

		// Push to the UI graph
		float tss_ms = (end - start) / U_TIME_1MS_IN_NS;
		t.timing.idx = (t.timing.idx + 1) % UI_TIMING_POSE_COUNT;
		t.timing.dur_ms[t.timing.idx] = tss_ms;
		constexpr float a = 1.0f / UI_TIMING_POSE_COUNT; // Exponential moving average
		t.timing.ui.reference_timing = (1 - a) * t.timing.ui.reference_timing + a * tss_ms;
	}

	return tss;
}


/*
 *
 * Feature information functionality
 *
 */

static void
features_ui_setup(TrackerSlam &t)
{
	t.features.enabled = false;

	u_var_add_ro_ftext(&t, "\n%s", "Tracker features");

	// Setup toggle button
	static const char *msg[2] = {"[OFF] Enable features info", "[ON] Disable features info"};
	u_var_button_cb cb = [](void *t_ptr) {
		TrackerSlam *t = (TrackerSlam *)t_ptr;
		u_var_button &btn = t->features.enable_btn;
		bool e = !t->features.enabled;
		snprintf(btn.label, sizeof(btn.label), "%s", msg[e]);
		vit_result_t vres =
		    t->vit.tracker_set_pose_capabilities(t->tracker, VIT_TRACKER_POSE_CAPABILITY_FEATURES, e);
		if (vres != VIT_SUCCESS) {
			U_LOG_IFL_E(t->log_level, "Failed to set tracker features capability");
			return;
		}
		t->features.enabled = e;
	};
	t.features.enable_btn.cb = cb;
	t.features.enable_btn.disabled = (t.caps & VIT_TRACKER_POSE_CAPABILITY_FEATURES) == 0;
	t.features.enable_btn.ptr = &t;
	u_var_add_button(&t, &t.features.enable_btn, msg[t.features.enabled]);

	// Setup graph
	u_var_curve_getter getter = [](void *fs_ptr, int i) -> u_var_curve_point {
		auto *fs = (TrackerSlam::Features::FeatureCounter *)fs_ptr;
		timepoint_ns now = os_monotonic_get_ns();

		size_t size = fs->entries.size();
		if (size == 0) {
			return {0, 0};
		}

		int last_idx = size - 1;
		if (i > last_idx) {
			i = last_idx;
		}

		auto [ts, count] = fs->entries.at(last_idx - i);
		return {time_ns_to_s(now - ts), double(count)};
	};

	t.features.fcs_ui.curve_count = t.cam_count;
	t.features.fcs_ui.xlabel = "Last seconds";
	t.features.fcs_ui.ylabel = "Number of features";

	t.features.fcs.resize(t.cam_count);
	for (uint32_t i = 0; i < t.cam_count; ++i) {
		auto &fc = t.features.fcs[i];
		fc.cam_name = "Cam" + to_string(i);

		auto &fc_ui = t.features.fcs_ui.curves[i];
		fc_ui.count = UI_FEATURES_POSE_COUNT;
		fc_ui.data = &fc;
		fc_ui.getter = getter;
		fc_ui.label = fc.cam_name.c_str();
	}

	u_var_add_curves(&t, &t.features.fcs_ui, "Feature count");
}

static vector<int>
features_ui_push(TrackerSlam &t, const vit_pose_t *pose, int64_t ts)
{
	if (!t.features.enabled) {
		return {};
	}

	// Push to the UI graph
	vector<int> fcs{};
	for (uint32_t i = 0; i < t.cam_count; ++i) {
		vit_pose_features features = {};
		vit_result_t vres = t.vit.pose_get_features(pose, i, &features);
		if (vres != VIT_SUCCESS) {
			// Even if the features are enabled, some of the poses already in the queue won't have it
			// enabled.
			if (vres != VIT_ERROR_NOT_ENABLED) {
				SLAM_ERROR("Failed to get pose features for camera %u", i);
			}

			return {};
		}

		t.features.fcs.at(i).addFeatureCount(ts, features.count);
		fcs.push_back(features.count);
	}

	return fcs;
}

/*
 *
 * Ground truth functionality
 *
 */

//! Gets an interpolated groundtruth pose (if available) at a specified timestamp
static xrt_pose
get_gt_pose_at(const Trajectory &gt, timepoint_ns ts)
{
	if (gt.empty()) {
		return XRT_POSE_IDENTITY;
	}

	Trajectory::const_iterator rit = gt.upper_bound(ts);

	if (rit == gt.begin()) { // Too far in the past, return first gt pose
		return gt.begin()->second;
	}

	if (rit == gt.end()) { // Too far in the future, return last gt pose
		return std::prev(gt.end())->second;
	}

	Trajectory::const_iterator lit = std::prev(rit);

	const auto &[lts, lpose] = *lit;
	const auto &[rts, rpose] = *rit;

	float t = double(ts - lts) / double(rts - lts);
	SLAM_DASSERT_(0 <= t && t <= 1);

	xrt_pose res{};
	math_quat_slerp(&lpose.orientation, &rpose.orientation, t, &res.orientation);
	res.position = m_vec3_lerp(lpose.position, rpose.position, t);
	return res;
}

//! Converts a pose from the tracker to ground truth
static struct xrt_pose
xr2gt_pose(const xrt_pose &gt_origin, const xrt_pose &xr_pose)
{
	//! @todo Right now this is hardcoded for Basalt and the EuRoC vicon datasets
	//! groundtruth and ignores orientation. Applies a fixed transformation so
	//! that the tracked and groundtruth trajectories origins and general motion
	//! match. The usual way of evaluating trajectory errors in SLAM requires to
	//! first align the trajectories through a non-linear optimization (e.g. gauss
	//! newton) so that they are as similar as possible. For this you need the
	//! entire tracked trajectory to be known beforehand, which makes it not
	//! suitable for reporting an error metric in realtime. See this 2-page paper
	//! for more info on trajectory alignment:
	//! https://ylatif.github.io/movingsensors/cameraReady/paper07.pdf

	xrt_vec3 pos = xr_pose.position;
	xrt_quat z180{0, 0, 1, 0};
	math_quat_rotate_vec3(&z180, &pos, &pos);
	math_quat_rotate_vec3(&gt_origin.orientation, &pos, &pos);
	pos += gt_origin.position;

	return {XRT_QUAT_IDENTITY, pos};
}

//! The inverse of @ref xr2gt_pose.
static struct xrt_pose
gt2xr_pose(const xrt_pose &gt_origin, const xrt_pose &gt_pose)
{
	xrt_vec3 pos = gt_pose.position;
	pos -= gt_origin.position;
	xrt_quat gt_origin_orientation_inv = gt_origin.orientation;
	math_quat_invert(&gt_origin_orientation_inv, &gt_origin_orientation_inv);
	math_quat_rotate_vec3(&gt_origin_orientation_inv, &pos, &pos);
	xrt_quat zn180{0, 0, -1, 0};
	math_quat_rotate_vec3(&zn180, &pos, &pos);

	return {XRT_QUAT_IDENTITY, pos};
}

static void
gt_ui_setup(TrackerSlam &t)
{
	u_var_add_ro_ftext(&t, "\n%s", "Tracker groundtruth");
	t.gt.diff_ui.values.data = t.gt.diffs_mm;
	t.gt.diff_ui.values.length = UI_GTDIFF_POSE_COUNT;
	t.gt.diff_ui.values.index_ptr = &t.gt.diff_idx;
	t.gt.diff_ui.reference_timing = 0;
	t.gt.diff_ui.center_reference_timing = true;
	t.gt.diff_ui.range = 100; // 10cm
	t.gt.diff_ui.dynamic_rescale = true;
	t.gt.diff_ui.unit = "mm";
	u_var_add_f32_timing(&t, &t.gt.diff_ui, "Tracking absolute error");
}

static void
gt_ui_push(TrackerSlam &t, timepoint_ns ts, xrt_pose tracked_pose)
{
	if (t.gt.trajectory->empty()) {
		return;
	}

	xrt_pose gt_pose = get_gt_pose_at(*t.gt.trajectory, ts);
	xrt_pose xr_pose = xr2gt_pose(t.gt.origin, tracked_pose);

	float len_mm = m_vec3_len(xr_pose.position - gt_pose.position) * 1000;
	t.gt.diff_idx = (t.gt.diff_idx + 1) % UI_GTDIFF_POSE_COUNT;
	t.gt.diffs_mm[t.gt.diff_idx] = len_mm;
	constexpr float a = 1.0f / UI_GTDIFF_POSE_COUNT; // Exponential moving average
	t.gt.diff_ui.reference_timing = (1 - a) * t.gt.diff_ui.reference_timing + a * len_mm;
}

/*
 *
 * Tracker functionality
 *
 */

//! Dequeue all tracked poses from the SLAM system and update prediction data with them.
static bool
flush_poses(TrackerSlam &t)
{

	vit_pose_t *pose = NULL;
	vit_result_t vres = t.vit.tracker_pop_pose(t.tracker, &pose);
	if (vres != VIT_SUCCESS) {
		SLAM_ERROR("Failed to get pose from VIT tracker");
	}

	if (pose == NULL) {
		SLAM_TRACE("No poses to flush");
		return false;
	}

	do {
		// New pose
		vit_pose_data_t data;
		vres = t.vit.pose_get_data(pose, &data);
		if (vres != VIT_SUCCESS) {
			SLAM_ERROR("Failed to get pose data from VIT tracker");
			return false;
		}

		int64_t nts = data.timestamp;

		xrt_vec3 npos{data.px, data.py, data.pz};
		xrt_quat nrot{data.ox, data.oy, data.oz, data.ow};

		// Last relation
		xrt_space_relation lr = XRT_SPACE_RELATION_ZERO;
		uint64_t lts;
		t.slam_rels.get_latest(&lts, &lr);
		xrt_vec3 lpos = lr.pose.position;
		xrt_quat lrot = lr.pose.orientation;

		double dt = time_ns_to_s(nts - lts);

		SLAM_TRACE("Dequeued SLAM pose ts=%ld p=[%f,%f,%f] r=[%f,%f,%f,%f]", //
		           nts, data.px, data.py, data.pz, data.ox, data.oy, data.oz, data.ow);

		// TODO linear velocity from the VIT system
		// Compute new relation based on new pose and velocities since last pose
		xrt_space_relation rel{};
		rel.relation_flags = XRT_SPACE_RELATION_BITMASK_ALL;
		rel.pose = {nrot, npos};
		rel.linear_velocity = (npos - lpos) / dt;
		math_quat_finite_difference(&lrot, &nrot, dt, &rel.angular_velocity);

		// Push to relationship history unless we are debugging prediction
		if (t.dbg_pred_counter % t.dbg_pred_every == 0) {
			t.slam_rels.push(rel, nts);
		}
		t.dbg_pred_counter = (t.dbg_pred_counter + 1) % t.dbg_pred_every;

		gt_ui_push(t, nts, rel.pose);
		t.slam_traj_writer->push({nts, rel.pose});
		xrt_pose_sample pose_sample = {nts, rel.pose};
		xrt_sink_push_pose(t.euroc_recorder->gt, &pose_sample);

		auto tss = timing_ui_push(t, pose, nts);
		t.slam_times_writer->push(tss);

		if (t.features.enabled) {
			vector feat_count = features_ui_push(t, pose, nts);
			t.slam_features_writer->push({nts, feat_count});
		}

		t.vit.pose_destroy(pose);
	} while (t.vit.tracker_pop_pose(t.tracker, &pose) == VIT_SUCCESS && pose);

	return true;
}

//! Integrates IMU samples on top of a base pose and predicts from that
static void
predict_pose_from_imu(TrackerSlam &t,
                      timepoint_ns when_ns,
                      xrt_space_relation base_rel, // Pose to integrate IMUs on top of
                      timepoint_ns base_rel_ts,
                      struct xrt_space_relation *out_relation)
{
	os_mutex_lock(&t.lock_ff);

	// Find oldest imu index i that is newer than latest SLAM pose (or -1)
	int i = 0;
	uint64_t imu_ts = UINT64_MAX;
	xrt_vec3 _;
	while (m_ff_vec3_f32_get(t.gyro_ff, i, &_, &imu_ts)) {
		if ((int64_t)imu_ts < base_rel_ts) {
			i--; // Back to the oldest newer-than-SLAM IMU index (or -1)
			break;
		}
		i++;
	}

	if (i == -1) {
		SLAM_WARN("No IMU samples received after latest SLAM pose (and frame)");
	}

	xrt_space_relation integ_rel = base_rel;
	timepoint_ns integ_rel_ts = base_rel_ts;
	xrt_quat &o = integ_rel.pose.orientation;
	xrt_vec3 &p = integ_rel.pose.position;
	xrt_vec3 &w = integ_rel.angular_velocity;
	xrt_vec3 &v = integ_rel.linear_velocity;
	bool clamped = false; // If when_ns is older than the latest IMU ts

	while (i >= 0) { // Decreasing i increases timestamp
		// Get samples
		xrt_vec3 g{};
		xrt_vec3 a{};
		uint64_t g_ts{};
		uint64_t a_ts{};
		bool got = true;
		got &= m_ff_vec3_f32_get(t.gyro_ff, i, &g, &g_ts);
		got &= m_ff_vec3_f32_get(t.accel_ff, i, &a, &a_ts);
		timepoint_ns ts = g_ts;


		// Checks
		if (ts > when_ns) {
			clamped = true;
			//! @todo Instead of using same a and g values, do an interpolated sample like this:
			// a = prev_a + ((when_ns - prev_ts) / (ts - prev_ts)) * (a - prev_a);
			// g = prev_g + ((when_ns - prev_ts) / (ts - prev_ts)) * (g - prev_g);
			ts = when_ns; // clamp ts to when_ns
		}
		SLAM_DASSERT(got && g_ts == a_ts, "Failure getting synced gyro and accel samples");
		SLAM_DASSERT(ts >= base_rel_ts, "Accessing imu sample that is older than latest SLAM pose");

		// Update time
		float dt = (float)time_ns_to_s(ts - integ_rel_ts);
		integ_rel_ts = ts;

		// Integrate gyroscope
		xrt_quat angvel_delta{};
		xrt_vec3 scaled_half_g = g * dt * 0.5f;
		math_quat_exp(&scaled_half_g, &angvel_delta); // Same as using math_quat_from_angle_vector(g/dt)
		math_quat_rotate(&o, &angvel_delta, &o);      // Orientation
		math_quat_rotate_derivative(&o, &g, &w);      // Angular velocity

		// Integrate accelerometer
		xrt_vec3 world_accel{};
		math_quat_rotate_vec3(&o, &a, &world_accel);
		world_accel += t.gravity_correction;
		v += world_accel * dt;                        // Linear velocity
		p += v * dt + world_accel * (dt * dt * 0.5f); // Position

		if (clamped) {
			break;
		}
		i--;
	}

	os_mutex_unlock(&t.lock_ff);

	// Do the prediction based on the updated relation
	double last_imu_to_now_dt = time_ns_to_s(when_ns - integ_rel_ts);
	xrt_space_relation predicted_relation{};
	m_predict_relation(&integ_rel, last_imu_to_now_dt, &predicted_relation);

	*out_relation = predicted_relation;
}

//! Return our best guess of the relation at time @p when_ns using all the data the tracker has.
static void
predict_pose(TrackerSlam &t, timepoint_ns when_ns, struct xrt_space_relation *out_relation)
{
	XRT_TRACE_MARKER();

	bool valid_pred_type = t.pred_type >= SLAM_PRED_NONE && t.pred_type < SLAM_PRED_COUNT;
	SLAM_DASSERT(valid_pred_type, "Invalid prediction type (%d)", t.pred_type);

	// Get last relation computed purely from SLAM data
	xrt_space_relation rel{};
	uint64_t rel_ts;
	bool empty = !t.slam_rels.get_latest(&rel_ts, &rel);

	// Stop if there is no previous relation to use for prediction
	if (empty) {
		out_relation->relation_flags = XRT_SPACE_RELATION_BITMASK_NONE;
		return;
	}

	// Use only last SLAM pose without prediction if PREDICTION_NONE
	if (t.pred_type == SLAM_PRED_NONE) {
		*out_relation = rel;
		return;
	}

	// Use only SLAM data if asking for an old point in time or PREDICTION_SP_SO_SA_SL
	SLAM_DASSERT_(rel_ts < INT64_MAX);
	if (t.pred_type == SLAM_PRED_SP_SO_SA_SL || when_ns <= (int64_t)rel_ts) {
		t.slam_rels.get(when_ns, out_relation);
		return;
	}


	if (t.pred_type == SLAM_PRED_IP_IO_IA_IL) {
		predict_pose_from_imu(t, when_ns, rel, (int64_t)rel_ts, out_relation);
		return;
	}

	os_mutex_lock(&t.lock_ff);

	// Update angular velocity with gyro data
	if (t.pred_type >= SLAM_PRED_SP_SO_IA_SL) {
		xrt_vec3 avg_gyro{};
		m_ff_vec3_f32_filter(t.gyro_ff, rel_ts, when_ns, &avg_gyro);
		math_quat_rotate_derivative(&rel.pose.orientation, &avg_gyro, &rel.angular_velocity);
	}

	// Update linear velocity with accel data
	if (t.pred_type >= SLAM_PRED_SP_SO_IA_IL) {
		xrt_vec3 avg_accel{};
		m_ff_vec3_f32_filter(t.accel_ff, rel_ts, when_ns, &avg_accel);
		xrt_vec3 world_accel{};
		math_quat_rotate_vec3(&rel.pose.orientation, &avg_accel, &world_accel);
		world_accel += t.gravity_correction;
		double slam_to_imu_dt = time_ns_to_s(t.last_imu_ts - rel_ts);
		rel.linear_velocity += world_accel * slam_to_imu_dt;
	}

	os_mutex_unlock(&t.lock_ff);

	// Do the prediction based on the updated relation
	double slam_to_now_dt = time_ns_to_s(when_ns - rel_ts);
	xrt_space_relation predicted_relation{};
	m_predict_relation(&rel, slam_to_now_dt, &predicted_relation);

	*out_relation = predicted_relation;
}

//! Various filters to remove noise from the predicted trajectory.
static void
filter_pose(TrackerSlam &t, timepoint_ns when_ns, struct xrt_space_relation *out_relation)
{
	XRT_TRACE_MARKER();

	if (t.filter.use_moving_average_filter) {
		if (out_relation->relation_flags & XRT_SPACE_RELATION_POSITION_VALID_BIT) {
			xrt_vec3 pos = out_relation->pose.position;
			m_ff_vec3_f32_push(t.filter.pos_ff, &pos, when_ns);
		}

		if (out_relation->relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) {
			// Don't save w component as we can retrieve it knowing these are (almost) unit quaternions
			xrt_vec3 rot = {out_relation->pose.orientation.x, out_relation->pose.orientation.y,
			                out_relation->pose.orientation.z};
			m_ff_vec3_f32_push(t.filter.rot_ff, &rot, when_ns);
		}

		// Get averages in time window
		timepoint_ns window = t.filter.window * U_TIME_1MS_IN_NS;
		xrt_vec3 avg_pos;
		m_ff_vec3_f32_filter(t.filter.pos_ff, when_ns - window, when_ns, &avg_pos);
		xrt_vec3 avg_rot; // Naive but good enough rotation average
		m_ff_vec3_f32_filter(t.filter.rot_ff, when_ns - window, when_ns, &avg_rot);

		// Considering the naive averaging this W is a bit wrong, but it feels reasonably well
		float avg_rot_w = sqrtf(1 - (avg_rot.x * avg_rot.x + avg_rot.y * avg_rot.y + avg_rot.z * avg_rot.z));
		out_relation->pose.orientation = xrt_quat{avg_rot.x, avg_rot.y, avg_rot.z, avg_rot_w};
		out_relation->pose.position = avg_pos;

		//! @todo Implement the quaternion averaging with a m_ff_vec4_f32 and
		//! normalization. Although it would be best to have a way of generalizing
		//! types before so as to not have redundant copies of ff logic.
	}

	if (t.filter.use_exponential_smoothing_filter) {
		xrt_space_relation &last = t.filter.last;
		xrt_space_relation &target = t.filter.target;
		target = *out_relation;
		m_space_relation_interpolate(&last, &target, t.filter.alpha, target.relation_flags, &last);
		*out_relation = last;
	}

	if (t.filter.use_one_euro_filter) {
		xrt_pose &p = out_relation->pose;
		if (out_relation->relation_flags & XRT_SPACE_RELATION_POSITION_VALID_BIT) {
			m_filter_euro_vec3_run(&t.filter.pos_oe, when_ns, &p.position, &p.position);
		}
		if (out_relation->relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) {
			m_filter_euro_quat_run(&t.filter.rot_oe, when_ns, &p.orientation, &p.orientation);
		}
	}
}

static void
setup_ui(TrackerSlam &t)
{
	t.pred_combo.count = SLAM_PRED_COUNT;
	t.pred_combo.options = "None\0Interpolate SLAM poses\0Also gyro\0Also accel\0Latest IMU\0";
	t.pred_combo.value = (int *)&t.pred_type;
	t.ui_sink = vector<u_sink_debug>(t.cam_count);
	for (size_t i = 0; i < t.ui_sink.size(); i++) {
		u_sink_debug_init(&t.ui_sink[i]);
	}
	os_mutex_init(&t.lock_ff);
	m_ff_vec3_f32_alloc(&t.gyro_ff, 1000);
	m_ff_vec3_f32_alloc(&t.accel_ff, 1000);
	m_ff_vec3_f32_alloc(&t.filter.pos_ff, 1000);
	m_ff_vec3_f32_alloc(&t.filter.rot_ff, 1000);

	u_var_add_root(&t, "SLAM Tracker", true);
	u_var_add_log_level(&t, &t.log_level, "Log Level");
	u_var_add_bool(&t, &t.submit, "Submit data to SLAM");

	u_var_button_cb reset_state_cb = [](void *t_ptr) {
		TrackerSlam &t = *(TrackerSlam *)t_ptr;

		vit_result_t vres = t.vit.tracker_reset(t.tracker);
		if (vres != VIT_SUCCESS) {
			SLAM_WARN("Failed to reset VIT tracker");
		}
	};
	t.reset_state_btn.cb = reset_state_cb;
	t.reset_state_btn.ptr = &t;
	u_var_add_button(&t, &t.reset_state_btn, "Reset tracker state");

	u_var_add_bool(&t, &t.gt.override_tracking, "Track with ground truth (if available)");
	euroc_recorder_add_ui(t.euroc_recorder, &t, "");

	u_var_add_gui_header(&t, NULL, "Trajectory Filter");
	u_var_add_bool(&t, &t.filter.use_moving_average_filter, "Enable moving average filter");
	u_var_add_f64(&t, &t.filter.window, "Window size (ms)");
	u_var_add_bool(&t, &t.filter.use_exponential_smoothing_filter, "Enable exponential smoothing filter");
	u_var_add_f32(&t, &t.filter.alpha, "Smoothing factor");
	u_var_add_bool(&t, &t.filter.use_one_euro_filter, "Enable one euro filter");
	u_var_add_f32(&t, &t.filter.pos_oe.base.fc_min, "Position minimum cutoff");
	u_var_add_f32(&t, &t.filter.pos_oe.base.beta, "Position beta speed");
	u_var_add_f32(&t, &t.filter.pos_oe.base.fc_min_d, "Position minimum delta cutoff");
	u_var_add_f32(&t, &t.filter.rot_oe.base.fc_min, "Orientation minimum cutoff");
	u_var_add_f32(&t, &t.filter.rot_oe.base.beta, "Orientation beta speed");
	u_var_add_f32(&t, &t.filter.rot_oe.base.fc_min_d, "Orientation minimum delta cutoff");

	u_var_add_gui_header(&t, NULL, "Prediction");
	u_var_add_combo(&t, &t.pred_combo, "Prediction Type");
	u_var_add_i32(&t, &t.dbg_pred_every, "Debug prediction skips (try 30)");
	u_var_add_ro_ff_vec3_f32(&t, t.gyro_ff, "Gyroscope");
	u_var_add_ro_ff_vec3_f32(&t, t.accel_ff, "Accelerometer");
	u_var_add_f32(&t, &t.gravity_correction.z, "Gravity Correction");
	for (size_t i = 0; i < t.ui_sink.size(); i++) {
		char label[] = "Camera NNNN";
		(void)snprintf(label, sizeof(label), "Camera %zu", i);
		u_var_add_sink_debug(&t, &t.ui_sink[i], label);
	}

	u_var_add_gui_header(&t, NULL, "Stats");
	u_var_add_ro_ftext(&t, "\n%s", "Record to CSV files");
	u_var_add_bool(&t, &t.slam_traj_writer->enabled, "Record tracked trajectory");
	u_var_add_bool(&t, &t.pred_traj_writer->enabled, "Record predicted trajectory");
	u_var_add_bool(&t, &t.filt_traj_writer->enabled, "Record filtered trajectory");
	u_var_add_bool(&t, &t.slam_times_writer->enabled, "Record tracker times");
	u_var_add_bool(&t, &t.slam_features_writer->enabled, "Record feature count");
	timing_ui_setup(t);
	features_ui_setup(t);
	// Later, gt_ui_setup will setup the tracking error UI if ground truth becomes available
}

static void
add_camera_calibration(const TrackerSlam &t, const t_slam_camera_calibration *calib, uint32_t cam_index)
{
	const t_camera_calibration &view = calib->base;

	vit_camera_calibration params = {};
	params.camera_index = cam_index;
	params.width = view.image_size_pixels.w;
	params.height = view.image_size_pixels.h;
	params.frequency = calib->frequency;

	params.fx = view.intrinsics[0][0];
	params.fy = view.intrinsics[1][1];
	params.cx = view.intrinsics[0][2];
	params.cy = view.intrinsics[1][2];

	switch (view.distortion_model) {
	case T_DISTORTION_OPENCV_RADTAN_8: {
		params.model = VIT_CAMERA_DISTORTION_RT8;
		const size_t size = sizeof(struct t_camera_calibration_rt8_params) + sizeof(double);
		params.distortion_count = size / sizeof(double);
		SLAM_ASSERT_(params.distortion_count == 9);

		memcpy(params.distortion, &view.rt8, size);

		// -1 metric radius tells Basalt to estimate the metric radius on its own.
		params.distortion[8] = -1.f;
		break;
	}
	case T_DISTORTION_WMR: {
		params.model = VIT_CAMERA_DISTORTION_RT8;
		const size_t size = sizeof(struct t_camera_calibration_rt8_params) + sizeof(double);
		params.distortion_count = size / sizeof(double);
		SLAM_ASSERT_(params.distortion_count == 9);

		memcpy(params.distortion, &view.wmr, size);

		params.distortion[8] = view.wmr.rpmax;

		break;
	}
	case T_DISTORTION_FISHEYE_KB4: {
		params.model = VIT_CAMERA_DISTORTION_KB4;
		const size_t size = sizeof(struct t_camera_calibration_kb4_params);
		params.distortion_count = size / sizeof(double);
		SLAM_ASSERT_(params.distortion_count == 4);

		memcpy(params.distortion, &view.kb4, size);
		break;
	}
	default:
		SLAM_ASSERT(false, "SLAM doesn't support distortion type %s",
		            t_stringify_camera_distortion_model(view.distortion_model));
		break;
	}

	xrt_matrix_4x4 T; // Row major T_imu_cam
	math_matrix_4x4_transpose(&calib->T_imu_cam, &T);

	// Converts the xrt_matrix_4x4 from float to double
	for (size_t i = 0; i < ARRAY_SIZE(params.transform); ++i)
		params.transform[i] = T.v[i];

	vit_result_t vres = t.vit.tracker_add_camera_calibration(t.tracker, &params);
	if (vres != VIT_SUCCESS) {
		SLAM_ERROR("Failed to add camera calibration for camera %u", cam_index);
	}
}

static void
add_imu_calibration(const TrackerSlam &t, const t_slam_imu_calibration *imu_calib)
{
	vit_imu_calibration_t params = {};
	params.imu_index = 0;
	params.frequency = imu_calib->frequency;

	// TODO improve memcpy size calculation

	const t_inertial_calibration &accel = imu_calib->base.accel;
	memcpy(params.accel.transform, accel.transform, sizeof(double) * 9);
	memcpy(params.accel.offset, accel.offset, sizeof(double) * 3);
	memcpy(params.accel.bias_std, accel.bias_std, sizeof(double) * 3);
	memcpy(params.accel.noise_std, accel.noise_std, sizeof(double) * 3);

	const t_inertial_calibration &gyro = imu_calib->base.gyro;
	memcpy(params.gyro.transform, gyro.transform, sizeof(double) * 9);
	memcpy(params.gyro.offset, gyro.offset, sizeof(double) * 3);
	memcpy(params.gyro.bias_std, gyro.bias_std, sizeof(double) * 3);
	memcpy(params.gyro.noise_std, gyro.noise_std, sizeof(double) * 3);

	vit_result_t vres = t.vit.tracker_add_imu_calibration(t.tracker, &params);
	if (vres != VIT_SUCCESS) {
		SLAM_ERROR("Failed to add imu calibration");
	}
}

static void
send_calibration(const TrackerSlam &t, const t_slam_calibration &c)
{
	vit_tracker_capability_t caps;
	vit_result_t vres = t.vit.tracker_get_capabilities(t.tracker, &caps);
	if (vres != VIT_SUCCESS) {
		SLAM_ERROR("Failed to get VIT tracker capabilities");
		return;
	}

	// Try to send camera calibration data to the SLAM system
	if ((caps & VIT_TRACKER_CAPABILITY_CAMERA_CALIBRATION) != 0) {
		for (int i = 0; i < c.cam_count; i++) {
			SLAM_INFO("Sending Camera %d calibration from Monado", i);
			add_camera_calibration(t, &c.cams[i], i);
		}
	} else {
		SLAM_WARN("Tracker doesn't support camera calibration");
	}

	// Try to send IMU calibration data to the SLAM system
	if ((caps & VIT_TRACKER_CAPABILITY_IMU_CALIBRATION) != 0) {
		SLAM_INFO("Sending IMU calibration from Monado");
		add_imu_calibration(t, &c.imu);
	} else {
		SLAM_WARN("Tracker doesn't support IMU calibration");
	}
}

} // namespace xrt::auxiliary::tracking::slam

using namespace xrt::auxiliary::tracking::slam;

/*
 *
 * External functions
 *
 */

//! Get a filtered prediction from the SLAM tracked poses.
extern "C" void
t_slam_get_tracked_pose(struct xrt_tracked_slam *xts, timepoint_ns when_ns, struct xrt_space_relation *out_relation)
{
	XRT_TRACE_MARKER();

	auto &t = *container_of(xts, TrackerSlam, base);

	//! @todo This should not be cached, the same timestamp can be requested at a
	//! later time on the frame for a better prediction.
	if (when_ns == t.last_ts) {
		*out_relation = t.last_rel;
		return;
	}

	flush_poses(t);

	predict_pose(t, when_ns, out_relation);
	t.pred_traj_writer->push({when_ns, out_relation->pose});

	filter_pose(t, when_ns, out_relation);
	t.filt_traj_writer->push({when_ns, out_relation->pose});

	t.last_rel = *out_relation;
	t.last_ts = when_ns;

	if (t.gt.override_tracking) {
		out_relation->pose = gt2xr_pose(t.gt.origin, get_gt_pose_at(*t.gt.trajectory, when_ns));
	}
}

//! Receive and register ground truth to use for trajectory error metrics.
extern "C" void
t_slam_gt_sink_push(struct xrt_pose_sink *sink, xrt_pose_sample *sample)
{
	XRT_TRACE_MARKER();

	auto &t = *container_of(sink, TrackerSlam, gt_sink);

	if (t.gt.trajectory->empty()) {
		t.gt.origin = sample->pose;
		gt_ui_setup(t);
	}

	t.gt.trajectory->insert_or_assign(sample->timestamp_ns, sample->pose);
	xrt_sink_push_pose(t.euroc_recorder->gt, sample);
}

//! Receive and register masks to use in the next image
extern "C" void
t_slam_hand_mask_sink_push(struct xrt_hand_masks_sink *sink, struct xrt_hand_masks_sample *hand_masks)
{
	XRT_TRACE_MARKER();

	auto &t = *container_of(sink, TrackerSlam, hand_masks_sink);
	unique_lock lock(t.last_hand_masks_mutex);
	t.last_hand_masks = *hand_masks;
}

//! Receive and send IMU samples to the external SLAM system
extern "C" void
t_slam_receive_imu(struct xrt_imu_sink *sink, struct xrt_imu_sample *s)
{
	XRT_TRACE_MARKER();

	auto &t = *container_of(sink, TrackerSlam, imu_sink);

	timepoint_ns ts = s->timestamp_ns;
	xrt_vec3_f64 a = s->accel_m_s2;
	xrt_vec3_f64 w = s->gyro_rad_secs;

	timepoint_ns now = (timepoint_ns)os_monotonic_get_ns();
	SLAM_TRACE("[%ld] imu t=%ld  a=[%f,%f,%f] w=[%f,%f,%f]", now, ts, a.x, a.y, a.z, w.x, w.y, w.z);
	// Check monotonically increasing timestamps
	if (ts <= t.last_imu_ts) {
		SLAM_WARN("Sample (%ld) is older than last (%ld)", ts, t.last_imu_ts);
		return;
	}
	t.last_imu_ts = ts;

	//! @todo There are many conversions like these between xrt and
	//! slam_tracker.hpp types. Implement a casting mechanism to avoid copies.
	vit_imu_sample_t sample = {};
	sample.timestamp = ts;
	sample.ax = a.x;
	sample.ay = a.y;
	sample.az = a.z;
	sample.wx = w.x;
	sample.wy = w.y;
	sample.wz = w.z;

	if (t.submit) {
		t.vit.tracker_push_imu_sample(t.tracker, &sample);
	}

	xrt_sink_push_imu(t.euroc_recorder->imu, s);

	struct xrt_vec3 gyro = {(float)w.x, (float)w.y, (float)w.z};
	struct xrt_vec3 accel = {(float)a.x, (float)a.y, (float)a.z};
	os_mutex_lock(&t.lock_ff);
	m_ff_vec3_f32_push(t.gyro_ff, &gyro, ts);
	m_ff_vec3_f32_push(t.accel_ff, &accel, ts);
	os_mutex_unlock(&t.lock_ff);
}

//! Push the frame to the external SLAM system
static void
receive_frame(TrackerSlam &t, struct xrt_frame *frame, uint32_t cam_index)
{
	XRT_TRACE_MARKER();

	SLAM_DASSERT_(frame->timestamp < INT64_MAX);

	// Return early if we don't submit
	if (!t.submit) {
		return;
	}

	if (cam_index == t.cam_count - 1) {
		flush_poses(t); // Useful to flush SLAM poses when no openxr app is open
	}

	SLAM_DASSERT(t.last_cam_ts[0] != INT64_MIN || cam_index == 0, "First frame was not a cam0 frame");

	// Check monotonically increasing timestamps
	timepoint_ns &last_ts = t.last_cam_ts[cam_index];
	timepoint_ns ts = (int64_t)frame->timestamp;
	SLAM_TRACE("[%ld] cam%d frame t=%ld", os_monotonic_get_ns(), cam_index, ts);
	if (last_ts >= ts) {
		SLAM_WARN("Frame (%ld) is older than last (%ld)", ts, last_ts);
	}
	last_ts = ts;

	// Construct and send the image sample
	vit_img_sample sample = {};
	sample.cam_index = cam_index;
	sample.timestamp = ts;

	sample.data = frame->data;
	sample.width = frame->width;
	sample.height = frame->height;
	sample.stride = frame->stride;
	sample.size = frame->size;

	// TODO check format before
	switch (frame->format) {
	case XRT_FORMAT_L8: sample.format = VIT_IMAGE_FORMAT_L8; break;
	case XRT_FORMAT_R8G8B8: sample.format = VIT_IMAGE_FORMAT_R8G8B8; break;
	default: SLAM_ERROR("Unknown image format"); return;
	}

	xrt_hand_masks_sample hand_masks{};
	{
		unique_lock lock(t.last_hand_masks_mutex);
		hand_masks = t.last_hand_masks;
	}

	auto &view = hand_masks.views[cam_index];
	std::vector<vit_mask_t> masks;
	if (view.enabled) {
		for (auto &hand : view.hands) {
			if (!hand.enabled) {
				continue;
			}
			vit_mask_t mask{};
			mask.x = hand.rect.x;
			mask.y = hand.rect.y;
			mask.w = hand.rect.w;
			mask.h = hand.rect.h;
			masks.push_back(mask);
		}

		sample.mask_count = masks.size();
		sample.masks = masks.empty() ? nullptr : masks.data();
	}

	{
		XRT_TRACE_IDENT(slam_push);
		t.vit.tracker_push_img_sample(t.tracker, &sample);
	}
}

#define DEFINE_RECEIVE_CAM(cam_id)                                                                                     \
	extern "C" void t_slam_receive_cam##cam_id(struct xrt_frame_sink *sink, struct xrt_frame *frame)               \
	{                                                                                                              \
		auto &t = *container_of(sink, TrackerSlam, cam_sinks[cam_id]);                                         \
		receive_frame(t, frame, cam_id);                                                                       \
		u_sink_debug_push_frame(&t.ui_sink[cam_id], frame);                                                    \
		xrt_sink_push_frame(t.euroc_recorder->cams[cam_id], frame);                                            \
	}

DEFINE_RECEIVE_CAM(0)
DEFINE_RECEIVE_CAM(1)
DEFINE_RECEIVE_CAM(2)
DEFINE_RECEIVE_CAM(3)
DEFINE_RECEIVE_CAM(4)

//! Define a function for each XRT_TRACKING_MAX_SLAM_CAMS and reference it in this array
void (*t_slam_receive_cam[XRT_TRACKING_MAX_SLAM_CAMS])(xrt_frame_sink *, xrt_frame *) = {
    t_slam_receive_cam0, //
    t_slam_receive_cam1, //
    t_slam_receive_cam2, //
    t_slam_receive_cam3, //
    t_slam_receive_cam4, //
};


extern "C" void
t_slam_node_break_apart(struct xrt_frame_node *node)
{
	auto &t = *container_of(node, TrackerSlam, node);
	if (t.ovr_tracker != NULL) {
		t_openvr_tracker_stop(t.ovr_tracker);
	}

	vit_result_t vres = t.vit.tracker_stop(t.tracker);
	if (vres != VIT_SUCCESS) {
		SLAM_ERROR("Failed to stop VIT tracker");
		return;
	}

	SLAM_DEBUG("SLAM tracker dismantled");
}

extern "C" void
t_slam_node_destroy(struct xrt_frame_node *node)
{
	auto t_ptr = container_of(node, TrackerSlam, node);
	auto &t = *t_ptr; // Needed by SLAM_DEBUG
	SLAM_DEBUG("Destroying SLAM tracker");
	if (t.ovr_tracker != NULL) {
		t_openvr_tracker_destroy(t.ovr_tracker);
	}
	delete t.gt.trajectory;
	delete t.slam_times_writer;
	delete t.slam_features_writer;
	delete t.slam_traj_writer;
	delete t.pred_traj_writer;
	delete t.filt_traj_writer;
	u_var_remove_root(t_ptr);
	for (size_t i = 0; i < t.ui_sink.size(); i++) {
		u_sink_debug_destroy(&t.ui_sink[i]);
	}
	m_ff_vec3_f32_free(&t.gyro_ff);
	m_ff_vec3_f32_free(&t.accel_ff);
	os_mutex_destroy(&t.lock_ff);
	m_ff_vec3_f32_free(&t.filter.pos_ff);
	m_ff_vec3_f32_free(&t.filter.rot_ff);

	t_ptr->vit.tracker_destroy(t_ptr->tracker);
	t_vit_bundle_unload(&t_ptr->vit);

	delete t_ptr;
}

extern "C" int
t_slam_start(struct xrt_tracked_slam *xts)
{
	auto &t = *container_of(xts, TrackerSlam, base);
	vit_result_t vres = t.vit.tracker_start(t.tracker);
	if (vres != VIT_SUCCESS) {
		SLAM_ERROR("Failed to start VIT tracker");
		return -1;
	}

	SLAM_DEBUG("SLAM tracker started");
	return 0;
}

extern "C" void
t_slam_fill_default_config(struct t_slam_tracker_config *config)
{
	config->log_level = debug_get_log_option_slam_log();
	config->vit_system_library_path = debug_get_option_vit_system_library_path();
	config->slam_config = debug_get_option_slam_config();
	config->slam_ui = debug_get_bool_option_slam_ui();
	config->submit_from_start = debug_get_bool_option_slam_submit_from_start();
	config->openvr_groundtruth_device = int(debug_get_num_option_slam_openvr_groundtruth_device());
	config->prediction = t_slam_prediction_type(debug_get_num_option_slam_prediction_type());
	config->write_csvs = debug_get_bool_option_slam_write_csvs();
	config->csv_path = debug_get_option_slam_csv_path();
	config->timing_stat = debug_get_bool_option_slam_timing_stat();
	config->features_stat = debug_get_bool_option_slam_features_stat();
	config->cam_count = int(debug_get_num_option_slam_cam_count());
	config->slam_calib = NULL;
}

extern "C" int
t_slam_create(struct xrt_frame_context *xfctx,
              struct t_slam_tracker_config *config,
              struct xrt_tracked_slam **out_xts,
              struct xrt_slam_sinks **out_sink)
{
	struct t_slam_tracker_config default_config = {};
	if (config == nullptr) {
		t_slam_fill_default_config(&default_config);
		config = &default_config;
	}

	enum u_logging_level log_level = config->log_level;

	std::unique_ptr<TrackerSlam> t_ptr = std::make_unique<TrackerSlam>();
	TrackerSlam &t = *t_ptr;

	t.log_level = log_level;

	SLAM_INFO("Loading VIT system library from VIT_SYSTEM_LIBRARY_PATH='%s'", config->vit_system_library_path);

	if (!t_vit_bundle_load(&t.vit, config->vit_system_library_path)) {
		SLAM_ERROR("Failed to load VIT system library from '%s'", config->vit_system_library_path);
		return -1;
	}

	// Check the user has provided a SLAM_CONFIG file
	const char *config_file = config->slam_config;
	bool some_calib = config->slam_calib != nullptr;
	if (!config_file && !some_calib) {
		SLAM_WARN("Unable to determine sensor calibration, did you forget to set SLAM_CONFIG?");
		return -1;
	}

	struct vit_config system_config = {};
	system_config.file = config_file;
	system_config.cam_count = config->cam_count;
	system_config.show_ui = config->slam_ui;

	vit_result_t vres = t.vit.tracker_create(&system_config, &t.tracker);
	if (vres != VIT_SUCCESS) {
		SLAM_ERROR("Failed to create VIT tracker (%d)", vres);
		return -1;
	}

	vres = t.vit.tracker_get_pose_capabilities(t.tracker, &t.caps);
	if (vres != VIT_SUCCESS) {
		SLAM_ERROR("Failed to get VIT tracker pose capabilities (%d)", vres);
		return -1;
	}

	t.base.get_tracked_pose = t_slam_get_tracked_pose;

	if (!config_file) {
		SLAM_INFO("Using calibration from driver and default pipeline settings");
		send_calibration(t, *config->slam_calib); // Not null because of `some_calib`
	} else {
		SLAM_INFO("Using sensor calibration provided by the SLAM_CONFIG file");
	}

	SLAM_ASSERT(t_slam_receive_cam[ARRAY_SIZE(t_slam_receive_cam) - 1] != nullptr, "See `cam_sink_push` docs");
	t.sinks.cam_count = config->cam_count;
	for (int i = 0; i < XRT_TRACKING_MAX_SLAM_CAMS; i++) {
		t.cam_sinks[i].push_frame = t_slam_receive_cam[i];
		t.sinks.cams[i] = &t.cam_sinks[i];
	}

	t.imu_sink.push_imu = t_slam_receive_imu;
	t.sinks.imu = &t.imu_sink;

	t.gt_sink.push_pose = t_slam_gt_sink_push;
	t.sinks.gt = &t.gt_sink;

	t.hand_masks_sink.push_hand_masks = t_slam_hand_mask_sink_push;
	t.sinks.hand_masks = &t.hand_masks_sink;

	t.submit = config->submit_from_start;
	t.cam_count = config->cam_count;

	t.node.break_apart = t_slam_node_break_apart;
	t.node.destroy = t_slam_node_destroy;

	xrt_frame_context_add(xfctx, &t.node);

	t.euroc_recorder = euroc_recorder_create(xfctx, NULL, t.cam_count, false);

	t.last_imu_ts = INT64_MIN;
	t.last_cam_ts = vector<timepoint_ns>(t.cam_count, INT64_MIN);
	t.last_hand_masks = xrt_hand_masks_sample{};

	t.pred_type = config->prediction;

	m_filter_euro_vec3_init(&t.filter.pos_oe, t.filter.min_cutoff, t.filter.min_dcutoff, t.filter.beta);
	m_filter_euro_quat_init(&t.filter.rot_oe, t.filter.min_cutoff, t.filter.min_dcutoff, t.filter.beta);

	t.gt.trajectory = new Trajectory{};

	// Setup CSV files
	bool write_csvs = config->write_csvs;
	string dir = config->csv_path;
	t.slam_times_writer = new TimingWriter(dir, "timing.csv", write_csvs, t.timing.columns);
	t.slam_features_writer = new FeaturesWriter(dir, "features.csv", write_csvs, t.cam_count);
	t.slam_traj_writer = new TrajectoryWriter(dir, "tracking.csv", write_csvs);
	t.pred_traj_writer = new TrajectoryWriter(dir, "prediction.csv", write_csvs);
	t.filt_traj_writer = new TrajectoryWriter(dir, "filtering.csv", write_csvs);

	setup_ui(t);

	// Setup OpenVR groundtruth tracker
	if (config->openvr_groundtruth_device > 0) {
		enum openvr_device dev_class = openvr_device(config->openvr_groundtruth_device);
		const double freq = 1000.0f;
		t.ovr_tracker = t_openvr_tracker_create(freq, &dev_class, &t.sinks.gt, 1);
		if (t.ovr_tracker != NULL) {
			t_openvr_tracker_start(t.ovr_tracker);
		}
	}

	// Get ownership
	TrackerSlam *tracker = t_ptr.release();

	*out_xts = &tracker->base;
	*out_sink = &tracker->sinks;

	SLAM_DEBUG("SLAM tracker created");
	return 0;
}
