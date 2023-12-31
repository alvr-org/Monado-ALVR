// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  A debugging scene.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup gui
 */

#include "xrt/xrt_config_have.h"

#include "os/os_time.h"

#include "util/u_var.h"
#include "util/u_misc.h"
#include "util/u_sink.h"
#include "util/u_debug.h"
#include "util/u_native_images_debug.h"

#ifdef XRT_HAVE_OPENCV
#include "tracking/t_tracking.h"
#endif

#include "xrt/xrt_frame.h"
#include "xrt/xrt_prober.h"
#include "xrt/xrt_tracking.h"
#include "xrt/xrt_settings.h"
#include "xrt/xrt_frameserver.h"

#include "math/m_api.h"
#include "math/m_filter_fifo.h"

#include "gui_common.h"
#include "gui_imgui.h"
#include "gui_window_record.h"
#include "gui_widget_native_images.h"

#include "imgui_monado/cimgui_monado.h"

#include <float.h>

/*
 *
 * Structs and defines.
 *
 */

/*!
 * @defgroup gui_debug Debug GUI
 * @ingroup gui
 *
 * @brief GUI for live inspecting Monado.
 */

/*!
 * A single record window, here only used to draw a single element in a object
 * window, holds all the needed state.
 *
 * @ingroup gui_debug
 */
struct debug_record
{
	void *ptr;

	struct gui_record_window rw;
};

/*!
 * A GUI scene for debugging Monado while it is running, it uses the variable
 * tracking code in the @ref util/u_var.h file to provide live updates state.
 *
 * @implements gui_scene
 * @ingroup gui_debug
 */
struct debug_scene
{
	struct gui_scene base;
	struct xrt_frame_context *xfctx;

	struct gui_widget_native_images_storage gwnis;

	struct debug_record recs[32];
	uint32_t num_recrs;
};

//! How many nested gui headers can we show, overly large.
#define MAX_HEADER_NESTING 256

//! Shared flags for color gui elements.
#define COLOR_FLAGS (ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_PickerHueWheel)

/*!
 * One "frame" of draw state, what is passed to the variable tracking visitor
 * functions, holds pointers to the program and live state such as visibility
 * stack of gui headers.
 *
 * @ingroup gui_debug
 */
struct draw_state
{
	struct gui_program *p;
	struct debug_scene *ds;

	//! Visibility stack for nested headers.
	bool vis_stack[MAX_HEADER_NESTING];
	int vis_i;

	//! Should we show the GUI headers for record sinks.
	bool inhibit_sink_headers;
};

/*!
 * State for plotting @ref m_ff_vec3_f32, assumes it's relative to now.
 *
 * @ingroup gui_debug
 */
struct plot_state
{
	//! The filter fifo we are plotting.
	struct m_ff_vec3_f32 *ff;

	//! When now is, all entries are made relative to this.
	uint64_t now;
};

DEBUG_GET_ONCE_BOOL_OPTION(curated_gui, "XRT_CURATED_GUI", false)


/*
 *
 * Helper functions.
 *
 */

static void
conv_rgb_f32_to_u8(struct xrt_colour_rgb_f32 *from, struct xrt_colour_rgb_u8 *to)
{
	to->r = (uint8_t)(from->r * 255.0f);
	to->g = (uint8_t)(from->g * 255.0f);
	to->b = (uint8_t)(from->b * 255.0f);
}

static void
conv_rgb_u8_to_f32(struct xrt_colour_rgb_u8 *from, struct xrt_colour_rgb_f32 *to)
{
	to->r = from->r / 255.0f;
	to->g = from->g / 255.0f;
	to->b = from->b / 255.0f;
}

static void
handle_draggable_vec3_f32(const char *name, struct xrt_vec3 *v)
{
	float min = -256.0f;
	float max = 256.0f;

	igDragFloat3(name, (float *)v, 0.005f, min, max, "%+f", 1.0f);
}

static void
handle_draggable_quat(const char *name, struct xrt_quat *q)
{
	float min = -1.0f;
	float max = 1.0f;

	igDragFloat4(name, (float *)q, 0.005f, min, max, "%+f", 1.0f);

	// Avoid invalid
	if (q->x == 0.0f && q->y == 0.0f && q->z == 0.0f && q->w == 0.0f) {
		q->w = 1.0f;
	}

	// And make sure it's a unit rotation.
	math_quat_normalize(q);
}

static struct debug_record *
ensure_debug_record_created(void *ptr, struct draw_state *state)
{
	struct debug_scene *ds = state->ds;
	struct u_sink_debug *usd = (struct u_sink_debug *)ptr;

	if (usd->sink == NULL) {
		struct debug_record *dr = &ds->recs[ds->num_recrs++];

		dr->ptr = ptr;

		gui_window_record_init(&dr->rw);
		u_sink_debug_set_sink(usd, &dr->rw.sink);

		return dr;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ds->recs); i++) {
		struct debug_record *dr = &ds->recs[i];

		if ((ptrdiff_t)dr->ptr == (ptrdiff_t)ptr) {
			return dr;
		}
	}

	return NULL;
}

// Currently unused.
XRT_MAYBE_UNUSED static void
draw_sink_to_background(struct u_var_info *var, struct draw_state *state)
{
	struct debug_record *dr = ensure_debug_record_created(var->ptr, state);
	if (dr == NULL) {
		return;
	}

	gui_window_record_to_background(&dr->rw, state->p);
}

XRT_MAYBE_UNUSED static void
draw_native_images_to_background(struct u_var_info *var, struct draw_state *state)
{
	struct debug_scene *ds = state->ds;
	struct u_native_images_debug *unid = (struct u_native_images_debug *)var->ptr;

	struct gui_widget_native_images *gwni = gui_widget_native_images_storage_ensure(&ds->gwnis, unid);
	if (gwni == NULL) {
		return;
	}

	gui_widget_native_images_update(gwni, unid);
	gui_widget_native_images_to_background(gwni, state->p);
}


/*
 *
 * Plot helpers.
 *
 */

#define PLOT_HELPER(elm)                                                                                               \
	static ImPlotPoint plot_vec3_f32_##elm(void *ptr, int index)                                                   \
	{                                                                                                              \
		struct plot_state *state = (struct plot_state *)ptr;                                                   \
		struct xrt_vec3 value;                                                                                 \
		uint64_t timestamp;                                                                                    \
		m_ff_vec3_f32_get(state->ff, index, &value, &timestamp);                                               \
		ImPlotPoint point = {time_ns_to_s(state->now - timestamp), value.elm};                                 \
		return point;                                                                                          \
	}

PLOT_HELPER(x)
PLOT_HELPER(y)
PLOT_HELPER(z)

static ImPlotPoint
plot_curve_point(void *ptr, int i)
{
	struct u_var_curve *c = (struct u_var_curve *)ptr;
	struct u_var_curve_point point = c->getter(c->data, i);
	ImPlotPoint implot_point = {point.x, point.y};
	return implot_point;
}

static float
plot_f32_array_value(void *ptr, int i)
{
	float *arr = ptr;
	return arr[i];
}


/*
 *
 * Main debug gui visitor functions.
 *
 */


static void
on_color_rgb_f32(const char *name, void *ptr)
{
	igColorEdit3(name, (float *)ptr, COLOR_FLAGS);
	igSameLine(0.0f, 4.0f);
	igText("%s", name);
}

static void
on_color_rgb_u8(const char *name, void *ptr)
{
	struct xrt_colour_rgb_f32 tmp;
	conv_rgb_u8_to_f32((struct xrt_colour_rgb_u8 *)ptr, &tmp);
	igColorEdit3(name, (float *)&tmp, COLOR_FLAGS);
	igSameLine(0.0f, 4.0f);
	igText("%s", name);
	conv_rgb_f32_to_u8(&tmp, (struct xrt_colour_rgb_u8 *)ptr);
}

static void
on_f32_arr(const char *name, void *ptr)
{
	struct u_var_f32_arr *f32_arr = ptr;
	int index = *f32_arr->index_ptr;
	int length = f32_arr->length;
	float *arr = (float *)f32_arr->data;

	float w = igGetWindowContentRegionWidth();
	ImVec2 graph_size = {w, 200};

	float stats_min = FLT_MAX;
	float stats_max = FLT_MAX;

	igPlotLinesFnFloatPtr(    //
	    name,                 //
	    plot_f32_array_value, //
	    arr,                  //
	    length,               //
	    index,                //
	    NULL,                 //
	    stats_min,            //
	    stats_max,            //
	    graph_size);          //
}

static void
on_timing(const char *name, void *ptr)
{
	struct u_var_timing *frametime_arr = ptr;
	struct u_var_f32_arr *f32_arr = &frametime_arr->values;
	int index = *f32_arr->index_ptr;
	int length = f32_arr->length;
	float *arr = (float *)f32_arr->data;

	float w = igGetWindowContentRegionWidth();
	ImVec2 graph_size = {w, 200};


	float stats_min = FLT_MAX;
	float stats_max = 0;

	for (int f = 0; f < length; f++) {
		if (arr[f] < stats_min)
			stats_min = arr[f];
		if (arr[f] > stats_max)
			stats_max = arr[f];
	}

	igPlotTimings(                              //
	    name,                                   //
	    plot_f32_array_value,                   //
	    arr,                                    //
	    length,                                 //
	    index,                                  //
	    NULL,                                   //
	    0,                                      //
	    stats_max,                              //
	    graph_size,                             //
	    frametime_arr->reference_timing,        //
	    frametime_arr->center_reference_timing, //
	    frametime_arr->range,                   //
	    frametime_arr->unit,                    //
	    frametime_arr->dynamic_rescale);        //
}

static void
on_pose(const char *name, void *ptr)
{
	struct xrt_pose *pose = (struct xrt_pose *)ptr;
	char text[512];
	snprintf(text, 512, "%s.position", name);
	handle_draggable_vec3_f32(text, &pose->position);
	snprintf(text, 512, "%s.orientation", name);
	handle_draggable_quat(text, &pose->orientation);
}

static void
on_ff_vec3_var(struct u_var_info *info, struct gui_program *p)
{
	char tmp[512];
	const char *name = info->name;
	struct m_ff_vec3_f32 *ff = (struct m_ff_vec3_f32 *)info->ptr;


	struct xrt_vec3 value = {0};

	uint64_t timestamp;

	m_ff_vec3_f32_get(ff, 0, &value, &timestamp);
	float value_arr[3] = {value.x, value.y, value.z};

	snprintf(tmp, sizeof(tmp), "%s.toggle", name);
	igToggleButton(tmp, &info->gui.graphed);
	igSameLine(0, 0);
	igInputFloat3(name, value_arr, "%+f", ImGuiInputTextFlags_ReadOnly);

	if (!info->gui.graphed) {
		return;
	}


	/*
	 * Showing the plot
	 */

	struct plot_state state = {ff, os_monotonic_get_ns()};
	ImPlotFlags flags = 0;
	ImPlotAxisFlags x_flags = 0;
	ImPlotAxisFlags y_flags = 0;
	ImPlotAxisFlags y2_flags = 0;
	ImPlotAxisFlags y3_flags = 0;

	ImVec2 size = {igGetWindowContentRegionWidth(), 256};
	bool shown = ImPlot_BeginPlot(name, "time", "value", size, flags, x_flags, y_flags, y2_flags, y3_flags);
	if (!shown) {
		return;
	}

	size_t num = m_ff_vec3_f32_get_num(ff);
	ImPlot_PlotLineG("z", plot_vec3_f32_z, &state, num, 0); // ZXY order to match RGB colors with default color map
	ImPlot_PlotLineG("x", plot_vec3_f32_x, &state, num, 0);
	ImPlot_PlotLineG("y", plot_vec3_f32_y, &state, num, 0);

	ImPlot_EndPlot();
}

static void
on_sink_debug_var(const char *name, void *ptr, struct draw_state *state)
{
	bool gui_header = !state->inhibit_sink_headers;

	struct debug_record *dr = ensure_debug_record_created(ptr, state);
	if (dr == NULL) {
		return;
	}

	if (gui_header) {
		const ImGuiTreeNodeFlags_ flags = ImGuiTreeNodeFlags_DefaultOpen;
		if (!igCollapsingHeaderBoolPtr(name, NULL, flags)) {
			return;
		}
	}

	gui_window_record_render(&dr->rw, state->p);
}

static void
on_native_images_debug_var(const char *name, void *ptr, struct draw_state *state)
{
	struct debug_scene *ds = state->ds;
	struct u_native_images_debug *unid = (struct u_native_images_debug *)ptr;

	struct gui_widget_native_images *gwni = gui_widget_native_images_storage_ensure(&ds->gwnis, unid);
	if (gwni == NULL) {
		return;
	}

	gui_widget_native_images_update(gwni, unid);
	gui_widget_native_images_render(gwni, state->p);
}

static void
on_button_var(const char *name, void *ptr)
{
	struct u_var_button *btn = (struct u_var_button *)ptr;
	ImVec2 dims = {btn->width, btn->height};
	const char *label = strlen(btn->label) == 0 ? name : btn->label;
	bool disabled = btn->disabled;

	if (disabled) {
		igPushStyleVarFloat(ImGuiStyleVar_Alpha, 0.6f);
		igPushItemFlag(ImGuiItemFlags_Disabled, true);
	}

	if (igButton(label, dims)) {
		btn->cb(btn->ptr);
	}

	if (disabled) {
		igPopItemFlag();
		igPopStyleVar(1);
	}
}

static void
on_combo_var(const char *name, void *ptr)
{
	struct u_var_combo *combo = (struct u_var_combo *)ptr;
	igComboStr(name, combo->value, combo->options, combo->count);
}

static void
on_histogram_f32_var(const char *name, void *ptr)
{
	struct u_var_histogram_f32 *h = (struct u_var_histogram_f32 *)ptr;
	ImVec2 zero = {h->width, h->height};
	igPlotHistogramFloatPtr(name, h->values, h->count, 0, NULL, FLT_MAX, FLT_MAX, zero, sizeof(float));
}

static void
on_curve_var(const char *name, void *ptr)
{
	struct u_var_curve *c = (struct u_var_curve *)ptr;
	ImVec2 size = {igGetWindowContentRegionWidth(), 256};

	bool shown = ImPlot_BeginPlot(name, c->xlabel, c->ylabel, size, 0, 0, 0, 0, 0);
	if (!shown) {
		return;
	}

	ImPlot_PlotLineG(c->label, plot_curve_point, c, c->count, 0);
	ImPlot_EndPlot();
}

static void
on_curves_var(const char *name, void *ptr)
{
	struct u_var_curves *cs = (struct u_var_curves *)ptr;
	ImVec2 size = {igGetWindowContentRegionWidth(), 256};

	bool shown = ImPlot_BeginPlot(name, cs->xlabel, cs->ylabel, size, 0, 0, 0, 0, 0);
	if (!shown) {
		return;
	}

	for (int i = 0; i < cs->curve_count; i++) {
		struct u_var_curve *c = &cs->curves[i];
		ImPlot_PlotLineG(c->label, plot_curve_point, c, c->count, 0);
	}
	ImPlot_EndPlot();
}

static void
on_draggable_f32_var(const char *name, void *ptr)
{
	struct u_var_draggable_f32 *d = (struct u_var_draggable_f32 *)ptr;
	igDragFloat(name, &d->val, d->step, d->min, d->max, "%+f", ImGuiSliderFlags_None);
}

static void
on_draggable_u16_var(const char *name, void *ptr)
{
	struct u_var_draggable_u16 *d = (struct u_var_draggable_u16 *)ptr;
	igDragScalar(name, ImGuiDataType_U16, d->val, d->step, &d->min, &d->max, NULL, ImGuiSliderFlags_None);
}

static void
on_gui_header(const char *name, struct draw_state *state)
{

	assert(state->vis_i == 0 && "Do not mix GUI_HEADER with GUI_HEADER_BEGIN/END");
	state->vis_stack[state->vis_i] = igCollapsingHeaderBoolPtr(name, NULL, 0);
}

static void
on_gui_header_begin(const char *name, struct draw_state *state)
{
	bool is_open = igCollapsingHeaderBoolPtr(name, NULL, 0);
	state->vis_stack[state->vis_i] = is_open;
	if (is_open) {
		igIndent(8.0f);
	}
}

static void
on_gui_header_end(void)
{
	igDummy((ImVec2){0, 8.0f});
	igUnindent(8.0f);
}

static void
on_root_enter(struct u_var_root_info *info, void *priv)
{
	struct draw_state *state = (struct draw_state *)priv;
	state->vis_i = 0;
	state->vis_stack[0] = true;

	igBegin(info->name, NULL, 0);
}

static void
on_elem(struct u_var_info *info, void *priv)
{
	const char *name = info->name;
	void *ptr = info->ptr;
	enum u_var_kind kind = info->kind;

	struct draw_state *state = (struct draw_state *)priv;

	bool visible = state->vis_stack[state->vis_i];

	// Handle the visibility stack.
	switch (kind) {
	case U_VAR_KIND_GUI_HEADER_BEGIN: // Increment stack and copy the current visible stack.
		state->vis_i++;
		state->vis_stack[state->vis_i] = visible;
		break;
	case U_VAR_KIND_GUI_HEADER_END: // Decrement the stack.
		state->vis_i--;
		break;
	case U_VAR_KIND_GUI_HEADER: // Always visible.
		on_gui_header(name, state);
		return; // Not doing anything more.
	default: break;
	}

	// Check balanced GUI_HEADER_BEGIN/END pairs
	assert(state->vis_i >= 0 && state->vis_i < MAX_HEADER_NESTING);

	if (!visible) {
		return;
	}

	const float drag_speed = 0.2f;
	const float power = 1.0f;
	ImGuiInputTextFlags i_flags = ImGuiInputTextFlags_None;
	ImGuiInputTextFlags ro_i_flags = ImGuiInputTextFlags_ReadOnly;

	switch (kind) {
	case U_VAR_KIND_BOOL: igCheckbox(name, (bool *)ptr); break;
	case U_VAR_KIND_RGB_F32: on_color_rgb_f32(name, ptr); break;
	case U_VAR_KIND_RGB_U8: on_color_rgb_u8(name, ptr); break;
	case U_VAR_KIND_U8: igDragScalar(name, ImGuiDataType_U8, ptr, drag_speed, NULL, NULL, NULL, power); break;
	case U_VAR_KIND_U16: igDragScalar(name, ImGuiDataType_U16, ptr, drag_speed, NULL, NULL, NULL, power); break;
	case U_VAR_KIND_U64: igDragScalar(name, ImGuiDataType_U64, ptr, drag_speed, NULL, NULL, NULL, power); break;
	case U_VAR_KIND_I32: igInputInt(name, (int *)ptr, 1, 10, i_flags); break;
	case U_VAR_KIND_I64: igInputScalar(name, ImGuiDataType_S64, ptr, NULL, NULL, NULL, i_flags); break;
	case U_VAR_KIND_VEC3_I32: igInputInt3(name, (int *)ptr, i_flags); break;
	case U_VAR_KIND_F32: igInputFloat(name, (float *)ptr, 1, 10, "%+f", i_flags); break;
	case U_VAR_KIND_F64: igInputDouble(name, (double *)ptr, 0.1, 1, "%+f", i_flags); break;
	case U_VAR_KIND_F32_ARR: on_f32_arr(name, ptr); break;
	case U_VAR_KIND_TIMING: on_timing(name, ptr); break;
	case U_VAR_KIND_VEC3_F32: igInputFloat3(name, (float *)ptr, "%+f", i_flags); break;
	case U_VAR_KIND_POSE: on_pose(name, ptr); break;
	case U_VAR_KIND_LOG_LEVEL: igComboStr(name, (int *)ptr, "Trace\0Debug\0Info\0Warn\0Error\0\0", 5); break;
	case U_VAR_KIND_RO_TEXT: igText("%s: '%s'", name, (char *)ptr); break;
	case U_VAR_KIND_RO_FTEXT: igText(ptr ? (char *)ptr : "%s", name); break;
	case U_VAR_KIND_RO_I32: igInputScalar(name, ImGuiDataType_S32, ptr, NULL, NULL, NULL, ro_i_flags); break;
	case U_VAR_KIND_RO_U32: igInputScalar(name, ImGuiDataType_U32, ptr, NULL, NULL, NULL, ro_i_flags); break;
	case U_VAR_KIND_RO_F32: igInputScalar(name, ImGuiDataType_Float, ptr, NULL, NULL, "%+f", ro_i_flags); break;
	case U_VAR_KIND_RO_I64: igInputScalar(name, ImGuiDataType_S64, ptr, NULL, NULL, NULL, ro_i_flags); break;
	case U_VAR_KIND_RO_U64: igInputScalar(name, ImGuiDataType_S64, ptr, NULL, NULL, NULL, ro_i_flags); break;
	case U_VAR_KIND_RO_F64: igInputScalar(name, ImGuiDataType_Double, ptr, NULL, NULL, "%+f", ro_i_flags); break;
	case U_VAR_KIND_RO_VEC3_I32: igInputInt3(name, (int *)ptr, ro_i_flags); break;
	case U_VAR_KIND_RO_VEC3_F32: igInputFloat3(name, (float *)ptr, "%+f", ro_i_flags); break;
	case U_VAR_KIND_RO_QUAT_F32: igInputFloat4(name, (float *)ptr, "%+f", ro_i_flags); break;
	case U_VAR_KIND_RO_FF_VEC3_F32: on_ff_vec3_var(info, state->p); break;
	case U_VAR_KIND_GUI_HEADER: assert(false && "Should be handled before this"); break;
	case U_VAR_KIND_GUI_HEADER_BEGIN: on_gui_header_begin(name, state); break;
	case U_VAR_KIND_GUI_HEADER_END: on_gui_header_end(); break;
	case U_VAR_KIND_SINK_DEBUG: on_sink_debug_var(name, ptr, state); break;
	case U_VAR_KIND_NATIVE_IMAGES_DEBUG: on_native_images_debug_var(name, ptr, state); break;
	case U_VAR_KIND_DRAGGABLE_F32: on_draggable_f32_var(name, ptr); break;
	case U_VAR_KIND_BUTTON: on_button_var(name, ptr); break;
	case U_VAR_KIND_COMBO: on_combo_var(name, ptr); break;
	case U_VAR_KIND_DRAGGABLE_U16: on_draggable_u16_var(name, ptr); break;
	case U_VAR_KIND_HISTOGRAM_F32: on_histogram_f32_var(name, ptr); break;
	case U_VAR_KIND_CURVE: on_curve_var(name, ptr); break;
	case U_VAR_KIND_CURVES: on_curves_var(name, ptr); break;
	default: igLabelText(name, "Unknown tag '%i'", kind); break;
	}
}

static void
on_root_exit(struct u_var_root_info *info, void *priv)
{
	struct draw_state *state = (struct draw_state *)priv;
	assert(state->vis_i == 0 && "Unbalanced GUI_HEADER_BEGIN/END pairs");
	state->vis_i = 0;
	state->vis_stack[0] = false;

	igEnd();
}


/*
 *
 * Advanced UI.
 *
 */

static bool g_show_advanced_gui = false;

static void
advanced_scene_render(struct debug_scene *ds, struct gui_program *p)
{
	struct draw_state state = {p, ds, {0}, 0, false};

	u_var_visit(on_root_enter, on_root_exit, on_elem, &state);

	igBegin("Advanced UI", NULL, 0);
	igCheckbox("Show advanced UI", &g_show_advanced_gui);
	igEnd();
}


/*
 *
 * Curated UI.
 *
 */

/*!
 * Which window are we searching.
 */
enum search_type
{
	SEARCH_INVALID,
	SEARCH_GUI_CONTROL,
	SEARCH_IPC_SERVER,
	SEARCH_SPACE_OVERSEER,
	SEARCH_SLAM_TRACKER,
	SEARCH_READBACK,
	SEARCH_HAND_TRACKER,
	SEARCH_APP_TIMING,
	SEARCH_COMPOSITOR,
	SEARCH_COMPOSITOR_TIMING,
};

/*!
 * Extra state for curated debug UI.
 */
struct curated_state
{
	//! Has the be first.
	struct draw_state ds;

	enum search_type search;

	struct
	{
		struct u_var_info *sink;
		struct u_var_info *enable;
	} readback; //!< Compositor readback variables.

	struct
	{
		struct u_var_info *view_0;
		struct u_var_info *enable;
	} mirror; //!< Compositor mirror variables.

	struct
	{
		struct
		{
			struct u_var_info *min;
		} apps[4];
		uint32_t app_count;

		struct u_var_info *present_to_display;
	} timing; //!< Both compositor and app timing related things.

	struct
	{
		struct u_var_info *size;
		struct u_var_info *detect;
		struct u_var_info *cams;
		struct u_var_info *graph;
	} hand_tracking;

	struct u_var_info *clear;

	struct u_var_info *ipc_running;
};

#define CHECK_RAW(NAME, TYPE)                                                                                          \
	if (strcmp(info->raw_name, NAME) == 0) {                                                                       \
		cs->search = TYPE;                                                                                     \
	}

#define CHECK(NAME, FIELD)                                                                                             \
	if (strcmp(info->name, NAME) == 0) {                                                                           \
		cs->FIELD = info;                                                                                      \
	}

#define DRAW(FIELD)                                                                                                    \
	if (cs.FIELD != NULL) {                                                                                        \
		on_elem(cs.FIELD, &cs.ds);                                                                             \
	}

static void
curated_on_root_enter(struct u_var_root_info *info, void *priv)
{
	struct curated_state *cs = (struct curated_state *)priv;

	CHECK_RAW("GUI Control", SEARCH_GUI_CONTROL);
	CHECK_RAW("IPC Server", SEARCH_IPC_SERVER);
	CHECK_RAW("Tracking Factory", SEARCH_INVALID);
	CHECK_RAW("Space Overseer", SEARCH_SPACE_OVERSEER);
	CHECK_RAW("Prober", SEARCH_INVALID);
	CHECK_RAW("SLAM Tracker", SEARCH_SLAM_TRACKER);
	CHECK_RAW("Vive Device", SEARCH_INVALID);
	CHECK_RAW("V4L2 Frameserver", SEARCH_INVALID);
	CHECK_RAW("Hand-tracking async shim!", SEARCH_INVALID);
	CHECK_RAW("Controller emulation!", SEARCH_INVALID);
	CHECK_RAW("Camera-based Hand Tracker", SEARCH_HAND_TRACKER);
	CHECK_RAW("App timing info", SEARCH_APP_TIMING);
	CHECK_RAW("Compositor", SEARCH_COMPOSITOR);
	CHECK_RAW("Compositor timing info", SEARCH_COMPOSITOR_TIMING);
	CHECK_RAW("Readback", SEARCH_READBACK);

	// If we have too many app timing structs, ignore them.
	if (cs->search == SEARCH_APP_TIMING && cs->timing.app_count >= ARRAY_SIZE(cs->timing.apps)) {
		cs->search = SEARCH_INVALID;
	}
}

static void
curated_on_elem(struct u_var_info *info, void *priv)
{
	struct curated_state *cs = (struct curated_state *)priv;

	switch (cs->search) {
	case SEARCH_INVALID: // Invalid
		break;
	case SEARCH_GUI_CONTROL: // GUI Control
		CHECK("Clear Colour", clear);
		break;
	case SEARCH_READBACK: // Readback
		CHECK("Readback left eye to debug GUI", readback.enable)
		CHECK("Left view!", readback.sink)
		break;
	case SEARCH_HAND_TRACKER: // Camera-based Hand Tracker
		CHECK("Hand size (Meters between wrist and middle-proximal joint)", hand_tracking.size)
		CHECK("Estimate hand sizes", hand_tracking.detect)
		CHECK("Annotated camera feeds", hand_tracking.cams)
		CHECK("Model inputs and outputs", hand_tracking.graph)
		break;
	case SEARCH_COMPOSITOR: // Compositor main.
		CHECK("Debug: Disable fast path", mirror.enable)
		CHECK("View[0]", mirror.view_0)
		break;
	case SEARCH_COMPOSITOR_TIMING: // Compositor timing info
		CHECK("Present to display offset(ms)", timing.present_to_display)
		break;
	case SEARCH_IPC_SERVER: // IPC Server
		CHECK("running", ipc_running)
		break;
	case SEARCH_APP_TIMING: // App timing info
		// App count is incremented on root exit, so app_count is the current one.
		CHECK("Minimum app time(ms)", timing.apps[cs->timing.app_count].min)
		break;
	case SEARCH_SPACE_OVERSEER:
		// Nothing yet.
		break;
	case SEARCH_SLAM_TRACKER:
		// Nothing yet.
		break;
	}
}

static void
curated_on_root_exit(struct u_var_root_info *info, void *priv)
{
	struct curated_state *cs = (struct curated_state *)priv;

	if (cs->search == SEARCH_APP_TIMING) {
		cs->timing.app_count++;
	}
}

static void
curated_render(struct debug_scene *ds, struct gui_program *p)
{
	struct curated_state cs = XRT_STRUCT_INIT;
	cs.ds = (struct draw_state){p, ds, {0}, 0, false};
	cs.ds.vis_stack[0] = true; // Make sure things are visible.

	// Collect the variables.
	u_var_visit(curated_on_root_enter, curated_on_root_exit, curated_on_elem, &cs);

	// Always enable the mirror sink.
	if (cs.mirror.enable != NULL) {
		*(bool *)cs.mirror.enable->ptr = true;
	}

	// Always set the clear colour.
	if (cs.clear != NULL) {
		*(struct xrt_colour_rgb_f32 *)cs.clear->ptr = (struct xrt_colour_rgb_f32){0.f, 0.f, 0.f};
	}

	// Start drawing.
	igBegin("Monado", NULL, 0);

	// Top exit button.
	ImVec2 button_dims = {48, 24};
	igSameLine(igGetWindowWidth() - button_dims.x - 8, -1);
	if (igButton("Exit", button_dims) && cs.ipc_running != NULL) {
		*(bool *)cs.ipc_running->ptr = false;
	}

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (igBeginTabBar("Tabs", tab_bar_flags)) {
		if (igBeginTabItem("Main", NULL, 0)) {
			DRAW(ipc_running);
			igCheckbox("Show advanced UI", &g_show_advanced_gui);
			igEndTabItem();
		}

		if (igBeginTabItem("Timing", NULL, 0)) {
			for (uint32_t i = 0; i < cs.timing.app_count; i++) {
				igText("App %u", i + 1);
				DRAW(timing.apps[i].min);
			}

			if (cs.timing.present_to_display != NULL) {
				igText("Compositor");
				DRAW(timing.present_to_display);
			}

			igEndTabItem();
		}

		// Close tab bar.
		igEndTabBar();
	}

	// If available draw the mirror native images the background.
	if (cs.mirror.view_0 != NULL) {
		draw_native_images_to_background(cs.mirror.view_0, &cs.ds);
	}

	igEnd();
}


/*
 *
 * Sink interception.
 *
 */

static void
on_root_enter_sink(struct u_var_root_info *info, void *priv)
{}

static void
on_elem_sink_debug_remove(struct u_var_info *info, void *null_ptr)
{
	void *ptr = info->ptr;
	enum u_var_kind kind = info->kind;

	if (kind != U_VAR_KIND_SINK_DEBUG) {
		return;
	}

	struct u_sink_debug *usd = (struct u_sink_debug *)ptr;
	u_sink_debug_set_sink(usd, NULL);
}

static void
on_root_exit_sink(struct u_var_root_info *info, void *priv)
{}


/*
 *
 * Scene functions.
 *
 */

static void
scene_render(struct gui_scene *scene, struct gui_program *p)
{
	struct debug_scene *ds = (struct debug_scene *)scene;

	static bool first = true;
	if (first) {
		g_show_advanced_gui = !debug_get_bool_option_curated_gui();
		first = false;
	}

	if (g_show_advanced_gui) {
		advanced_scene_render(ds, p);
	} else {
		curated_render(ds, p);
	}
}

static void
scene_destroy(struct gui_scene *scene, struct gui_program *p)
{
	struct debug_scene *ds = (struct debug_scene *)scene;

	// Remove the sink interceptors.
	u_var_visit(on_root_enter_sink, on_root_exit_sink, on_elem_sink_debug_remove, NULL);

	if (ds->xfctx != NULL) {
		xrt_frame_context_destroy_nodes(ds->xfctx);
		ds->xfctx = NULL;
	}

	free(ds);
}


/*
 *
 * 'Exported' functions.
 *
 */

void
gui_scene_debug(struct gui_program *p)
{
	// Only create devices if we have a instance and no system devices.
	if (p->instance != NULL && p->xsysd == NULL) {
		gui_prober_select(p);
	}

	struct debug_scene *ds = U_TYPED_CALLOC(struct debug_scene);

	ds->base.render = scene_render;
	ds->base.destroy = scene_destroy;

	gui_scene_push_front(p, &ds->base);
}
