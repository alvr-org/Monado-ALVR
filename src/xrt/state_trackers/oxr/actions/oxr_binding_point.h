// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Runtime binding point data cloned from generated templates.
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_defines.h"

#include "oxr_defines.h"
#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"
#include "oxr_generated_bindings.h"
#include "oxr_subaction_store.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Runtime metadata for one DPAD emulation source, for example
 * `/user/hand/left/input/thumbstick`.
 */
struct oxr_dpad_emulation
{
	//! Owning interaction profile.
	struct oxr_binding_profile *profile;
	//! Immutable generated source template.
	const struct oxr_dpad_emulation_template *templ;
	//! Runtime XrPath for the DPAD source path.
	XrPath path;
	//! Runtime XrPath for the top-level subaction path.
	XrPath subaction_path;
	/*!
	 * XRT position input that DPAD regions are derived from,
	 * this must be of type @ref XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE.
	 */
	enum xrt_input_name input;
	//! Optional XRT input used to activate DPAD emulation.
	enum xrt_input_name activate;
};

/*!
 * Payload for a binding point that directly targets an XRT input.
 */
struct oxr_binding_point_input
{
	//! XRT input name.
	enum xrt_input_name name;
};

/*!
 * Payload for a binding point that directly targets an XRT output.
 */
struct oxr_binding_point_output
{
	//! XRT output name.
	enum xrt_output_name name;
};

/*!
 * Payload for a binding point that resolves to another concrete binding point.
 */
struct oxr_binding_point_upgrade
{
	XrPath boolean_path;
	XrPath float_path;
	XrPath vec2_path;
	XrPath pose_path;
	XrPath haptic_path;
};

/*!
 * Payload for a binding point that represents a DPAD emulation region.
 */
struct oxr_binding_point_dpad
{
	//! DPAD emulation that backs this region.
	struct oxr_dpad_emulation *emulation;
	//! Region represented by this binding point.
	enum oxr_dpad_binding_point binding_point;
};

/*!
 * Runtime description of one bindable path on an interaction profile.
 *
 * Binding points are cloned from generated templates into the
 * @ref oxr_binding_registry and are never mutated after that. All generated
 * points are present in the registry; API validation decides whether an app
 * may suggest a particular path. The common fields identify the point and the
 * union stores data valid for @ref kind.
 *
 * This is the object an @ref oxr_suggested_binding points to. The suggested
 * binding is what the OpenXR client submits (via
 * xrSuggestInteractionProfileBindings); it selects a path and stores a
 * non-owning pointer to the matching registry binding point.
 */
struct oxr_binding_point
{
	//! Owning interaction profile.
	struct oxr_binding_profile *profile;

	//! Immutable generated source template.
	const struct oxr_binding_point_template *templ;

	//! Subaction path this binding point belongs to.
	const struct oxr_binding_subaction *subaction;

	//! Runtime XrPath identity for this binding point.
	XrPath path;

	//! Unique source path returned to applications for this binding point.
	XrPath source_path;

	//! User-facing localized name for this binding point.
	const char *localized_name;

	//! Runtime XrPath for the priority group used for action priority suppression.
	XrPath priority_group;

	//! Controls which fields are valid.
	enum oxr_binding_point_kind kind;

	//! Transform needed to interpret this binding point's raw input value.
	enum oxr_binding_value_transform value_transform;

	//! Kind-specific payload.
	union {
		struct oxr_binding_point_input input;
		struct oxr_binding_point_output output;
		struct oxr_binding_point_upgrade upgrade;
		struct oxr_binding_point_dpad dpad;
	};
};


/*!
 * Short name for a binding point kind, for debug printing.
 */
static inline const char *
oxr_binding_point_kind_to_string(enum oxr_binding_point_kind kind)
{
	switch (kind) {
	case OXR_BINDING_POINT_KIND_INPUT: return "INPUT";
	case OXR_BINDING_POINT_KIND_OUTPUT: return "OUTPUT";
	case OXR_BINDING_POINT_KIND_UPGRADE: return "UPGRADE";
	case OXR_BINDING_POINT_KIND_DPAD: return "DPAD";
	default: return "UNKNOWN";
	}
}


#ifdef __cplusplus
}
#endif
