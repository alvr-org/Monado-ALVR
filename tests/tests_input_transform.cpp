// Copyright 2018-2020, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Input transform tests.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

#include "math/m_mathinclude.h"

#include "catch_amalgamated.hpp"

#include <xrt/xrt_defines.h>

#include <oxr/actions/oxr_binding_registry.h>
#include <oxr/actions/oxr_input_transform.h>
#include <oxr/oxr_logger.h>
#include <oxr/oxr_objects.h>

using Catch::Generators::values;

struct mock_binding_point
{
	oxr_binding_point_template templ = {};
	oxr_binding_point point = {};

	mock_binding_point(const char *path,
	                   enum oxr_binding_value_transform value_transform = OXR_BINDING_VALUE_TRANSFORM_NONE)
	{
		templ.path = path;
		templ.value_transform = value_transform;
		point.templ = &templ;
		point.value_transform = value_transform;
	}
};

TEST_CASE("input_transform")
{
	struct oxr_logger log;
	oxr_log_init(&log, "test");
	struct oxr_sink_logger slog = {};

	struct oxr_input_transform *transforms = NULL;
	size_t transform_count = 0;

	oxr_input_value_tagged input = {};
	oxr_input_value_tagged output = {};
	mock_binding_point mock_float{"/mock_float"};
	mock_binding_point mock_bool{"/mock_bool"};
	mock_binding_point mock_pose{"/mock_pose"};
	mock_binding_point mock{"/mock"};
	mock_binding_point mock_vec2{"/mock_vec2"};
	mock_binding_point mock_vec2_x{"/mock_vec2/x", OXR_BINDING_VALUE_TRANSFORM_VEC2_GET_X};
	mock_binding_point mock_vec2_y{"/mock_vec2/y", OXR_BINDING_VALUE_TRANSFORM_VEC2_GET_Y};

	SECTION("Float action")
	{
		XrActionType action_type = XR_ACTION_TYPE_FLOAT_INPUT;

		SECTION("From Vec1 -1 to 1 identity")
		{
			input.type = XRT_INPUT_TYPE_VEC1_MINUS_ONE_TO_ONE;

			CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type, "float_action",
			                                       &mock_float.point, &transforms, &transform_count));

			// Just identity
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);

			SECTION("Roundtrip")
			{
				auto value = GENERATE(values({-1.f, -0.5f, 0.f, -0.f, 0.5f, 1.f}));
				input.value.vec1.x = value;

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(input.value.vec1.x == output.value.vec1.x);
			}
		}

		SECTION("From Vec1 0 to 1 identity")
		{
			input.type = XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE;

			CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type, "float_action",
			                                       &mock_float.point, &transforms, &transform_count));

			// Just identity
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);

			SECTION("Roundtrip")
			{
				auto value = GENERATE(values({0.f, -0.f, 0.5f, 1.f}));
				input.value.vec1.x = value;

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(input.value.vec1.x == output.value.vec1.x);
			}
		}

		SECTION("From Vec2 input")
		{
			input.type = XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE;
			input.value.vec2.x = -1;
			input.value.vec2.y = 1;

			SECTION("path component x")
			{
				CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
				                                       "float_action", &mock_vec2_x.point, &transforms,
				                                       &transform_count));

				// A get-x
				CHECK(transform_count == 1);
				CHECK(transforms != nullptr);

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(input.value.vec2.x == output.value.vec1.x);
			}

			SECTION("path component y")
			{
				CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
				                                       "float_action", &mock_vec2_y.point, &transforms,
				                                       &transform_count));

				// A get-y
				CHECK(transform_count == 1);
				CHECK(transforms != nullptr);

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(input.value.vec2.y == output.value.vec1.x);
			}

			SECTION("no component")
			{
				CHECK_FALSE(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
				                                             "float_action", &mock_vec2.point,
				                                             &transforms, &transform_count));

				// Shouldn't make a transform, not possible
				CHECK(transform_count == 0);
				CHECK(transforms == nullptr);

				// shouldn't do anything, but shouldn't explode.
				CHECK_FALSE(oxr_input_transform_process(transforms, transform_count, &input, &output));
			}
		}

		SECTION("From bool input")
		{
			input.type = XRT_INPUT_TYPE_BOOLEAN;
			CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type, "float_action",
			                                       &mock_bool.point, &transforms, &transform_count));

			// A bool-to-float
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);

			SECTION("False")
			{
				input.value.boolean = false;

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(0.0f == output.value.vec1.x);
			}

			SECTION("True")
			{
				input.value.boolean = true;

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(1.0f == output.value.vec1.x);
			}
		}
	}

	SECTION("Bool action")
	{
		XrActionType action_type = XR_ACTION_TYPE_BOOLEAN_INPUT;
		SECTION("From Bool identity")
		{
			input.type = XRT_INPUT_TYPE_BOOLEAN;

			CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type, "bool_action",
			                                       &mock_bool.point, &transforms, &transform_count));
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);

			SECTION("Roundtrip")
			{
				auto value = GENERATE(values({0, 1}));
				input.value.boolean = bool(value);
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(input.value.boolean == output.value.boolean);
			}
		}

		SECTION("From Vec1 -1 to 1")
		{
			input.type = XRT_INPUT_TYPE_VEC1_MINUS_ONE_TO_ONE;

			CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type, "bool_action",
			                                       &mock_float.point, &transforms, &transform_count));
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);

			SECTION("True")
			{
				auto value = GENERATE(values({0.5f, 1.f}));
				input.value.vec1.x = value;

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(output.value.boolean == true);
			}

			SECTION("False")
			{
				auto value = GENERATE(values({0.0f, -1.f}));
				input.value.vec1.x = value;

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(output.value.boolean == false);
			}
		}

		SECTION("From Vec1 0 to 1")
		{
			input.type = XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE;

			CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type, "bool_action",
			                                       &mock_float.point, &transforms, &transform_count));
			// A bool to float
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);

			SECTION("True")
			{
				auto value = GENERATE(values({0.95f, 1.f}));
				input.value.vec1.x = value;

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(output.value.boolean == true);
			}

			SECTION("False")
			{
				auto value = GENERATE(values({0.0f, 0.5f}));
				input.value.vec1.x = value;

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(output.value.boolean == false);
			}
		}

		SECTION("From Vec2")
		{
			input.type = XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE;
			input.value.vec2.x = -1;
			input.value.vec2.y = 1;

			SECTION("x")
			{
				CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
				                                       "float_action", &mock_vec2_x.point, &transforms,
				                                       &transform_count));
				CHECK(transform_count == 2);
				CHECK(transforms != nullptr);

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(false == output.value.boolean);
			}

			SECTION("y")
			{
				CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
				                                       "float_action", &mock_vec2_y.point, &transforms,
				                                       &transform_count));
				CHECK(transform_count == 2);
				CHECK(transforms != nullptr);

				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(true == output.value.boolean);
			}

			SECTION("no component")
			{
				CHECK_FALSE(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
				                                             "float_action", &mock.point, &transforms,
				                                             &transform_count));

				// Shouldn't make a transform, not possible
				CHECK(transform_count == 0);
				CHECK(transforms == nullptr);

				// shouldn't do anything, but shouldn't explode.
				CHECK_FALSE(oxr_input_transform_process(transforms, transform_count, &input, &output));
			}
		}
	}

	SECTION("Vector2 action")
	{
		XrActionType action_type = XR_ACTION_TYPE_VECTOR2F_INPUT;
		input.type = XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE;

		SECTION("From Vec2 identity")
		{
			CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type, "vec2_action",
			                                       &mock_vec2.point, &transforms, &transform_count));
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);
		}

		SECTION("Reject Vec2 bound through X component")
		{
			CHECK_FALSE(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
			                                             "vec2_action", &mock_vec2_x.point, &transforms,
			                                             &transform_count));
			CHECK(transform_count == 0);
			CHECK(transforms == nullptr);
		}

		SECTION("Reject Vec2 bound through Y component")
		{
			CHECK_FALSE(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
			                                             "vec2_action", &mock_vec2_y.point, &transforms,
			                                             &transform_count));
			CHECK(transform_count == 0);
			CHECK(transforms == nullptr);
		}
	}

	SECTION("Pose action")
	{
		XrActionType action_type = XR_ACTION_TYPE_POSE_INPUT;

		SECTION("From Pose identity")
		{
			input.type = XRT_INPUT_TYPE_POSE;
			CHECK(oxr_input_transform_create_chain(&log, &slog, input.type, action_type, "pose_action",
			                                       &mock_pose.point, &transforms, &transform_count));
			// Identity, just so this binding doesn't get culled.
			CHECK(transform_count == 1);
		}

		SECTION("From other input")
		{
			auto input_type = GENERATE(values({
			    XRT_INPUT_TYPE_BOOLEAN,
			    XRT_INPUT_TYPE_VEC1_MINUS_ONE_TO_ONE,
			    XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE,
			    XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE,
			    XRT_INPUT_TYPE_VEC3_MINUS_ONE_TO_ONE,
			}));

			CAPTURE(input_type);
			input.type = input_type;

			CHECK_FALSE(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
			                                             "pose_action", &mock.point, &transforms,
			                                             &transform_count));

			// not possible
			CHECK(transform_count == 0);
			CHECK(transforms == nullptr);
		}

		SECTION("Reject Vec2 binding points")
		{
			input.type = XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE;

			CHECK_FALSE(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
			                                             "pose_action", &mock_vec2.point, &transforms,
			                                             &transform_count));
			CHECK(transform_count == 0);
			CHECK(transforms == nullptr);

			CHECK_FALSE(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
			                                             "pose_action", &mock_vec2_x.point, &transforms,
			                                             &transform_count));
			CHECK(transform_count == 0);
			CHECK(transforms == nullptr);

			CHECK_FALSE(oxr_input_transform_create_chain(&log, &slog, input.type, action_type,
			                                             "pose_action", &mock_vec2_y.point, &transforms,
			                                             &transform_count));
			CHECK(transform_count == 0);
			CHECK(transforms == nullptr);
		}
	}

	oxr_log_slog(&log, &slog);
	oxr_input_transform_destroy(&transforms);
	CHECK(NULL == transforms);
}


struct dpad_test_case
{
	float x;
	float y;
	enum oxr_dpad_region active_regions;
};


TEST_CASE("input_transform_dpad")
{
	struct oxr_logger log;
	oxr_log_init(&log, "test");
	struct oxr_sink_logger slog = {};

	struct oxr_input_transform *transforms = NULL;
	size_t transform_count = 0;

	oxr_input_value_tagged input = {};
	oxr_input_value_tagged output = {};

	struct oxr_dpad_binding_modification *dpad_binding_modification = NULL;
	enum xrt_input_type activation_input_type = XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE;
	struct xrt_input activation_input = {};
	enum oxr_dpad_region dpad_region = OXR_DPAD_REGION_UP;

	SECTION("Default settings")
	{
		XrActionType action_type = XR_ACTION_TYPE_BOOLEAN_INPUT;

		SECTION("without an activation input")
		{
			input.type = XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE;

			CHECK(oxr_input_transform_create_chain_dpad(
			    &log, &slog, input.type, action_type, "/mock_vec2/dpad_up", dpad_binding_modification,
			    dpad_region, activation_input_type, NULL, &transforms, &transform_count));
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);
			CHECK(transforms[0].type == INPUT_TRANSFORM_DPAD);


			SECTION("up region is off in center")
			{
				input.value.vec2.x = 0.0f;
				input.value.vec2.y = 0.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(false == output.value.boolean);
			}

			SECTION("up region is on when pointing up")
			{
				input.value.vec2.x = 0.0f;
				input.value.vec2.y = 1.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(true == output.value.boolean);
			}

			struct dpad_test_case cases[9] = {
			    // obvious
			    {0.0f, 0.0f, OXR_DPAD_REGION_CENTER},
			    {0.0f, 1.0f, OXR_DPAD_REGION_UP},
			    {0.0f, -1.0f, OXR_DPAD_REGION_DOWN},
			    {-1.0f, 0.0f, OXR_DPAD_REGION_LEFT},
			    {1.0f, 0.0f, OXR_DPAD_REGION_RIGHT},
			    // boundary cases
			    {1.0f, 1.0f, OXR_DPAD_REGION_UP},
			    {-1.0f, -1.0f, OXR_DPAD_REGION_DOWN},
			    {-1.0f, 1.0f, OXR_DPAD_REGION_LEFT},
			    {1.0f, -1.0f, OXR_DPAD_REGION_RIGHT},
			};

			for (uint32_t i = 0; i < ARRAY_SIZE(cases); i++) {
				DYNAMIC_SECTION("with (x, y) of (" << cases[i].x << ", " << cases[i].y << ")")
				{
					input.value.vec2.x = cases[i].x;
					input.value.vec2.y = cases[i].y;
					CHECK(
					    oxr_input_transform_process(transforms, transform_count, &input, &output));
					CHECK(cases[i].active_regions == transforms[0].data.dpad_state.active_regions);
				}
			}
		}
		SECTION("with a boolean activation input")
		{
			input.type = XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE;
			input.value.vec2.x = 0.0f;
			input.value.vec2.y = 1.0f;

			activation_input_type = XRT_INPUT_TYPE_BOOLEAN;

			CHECK(oxr_input_transform_create_chain_dpad(
			    &log, &slog, input.type, action_type, "/mock_vec2/dpad_up", dpad_binding_modification,
			    dpad_region, activation_input_type, &activation_input, &transforms, &transform_count));
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);
			CHECK(transforms[0].type == INPUT_TRANSFORM_DPAD);

			SECTION("when activation input is set to true")
			{
				activation_input.value.boolean = true;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(true == output.value.boolean);
			}
			SECTION("when activation input is set to false")
			{
				activation_input.value.boolean = false;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(false == output.value.boolean);
			}
		}
		SECTION("with a float activation input")
		{
			input.type = XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE;
			input.value.vec2.x = 0.0f;
			input.value.vec2.y = 1.0f;

			activation_input_type = XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE;

			CHECK(oxr_input_transform_create_chain_dpad(
			    &log, &slog, input.type, action_type, "/mock_vec2/dpad_up", dpad_binding_modification,
			    dpad_region, activation_input_type, &activation_input, &transforms, &transform_count));
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);
			CHECK(transforms[0].type == INPUT_TRANSFORM_DPAD);

			SECTION("when activation input is set to 1.0")
			{
				activation_input.value.vec1.x = 1.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(true == output.value.boolean);
			}
			SECTION("when activation input is set to 0.0")
			{
				activation_input.value.vec1.x = 0.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(false == output.value.boolean);
			}
			SECTION("when activation input varies")
			{
				activation_input.value.vec1.x = 0.45f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(false == output.value.boolean);
				activation_input.value.vec1.x = 0.6f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(true == output.value.boolean);
				activation_input.value.vec1.x = 0.45f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(true == output.value.boolean);
				activation_input.value.vec1.x = 0.35f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(false == output.value.boolean);
			}
		}
	}
	SECTION("Sticky enabled")
	{
		XrActionType action_type = XR_ACTION_TYPE_BOOLEAN_INPUT;

		struct oxr_dpad_binding_modification dpad_binding_modification_val = {
		    XR_NULL_PATH, // XrPath binding, unused at this stage
		    {
		        0.5f,          // float forceThreshold
		        0.4f,          // float forceThresholdReleased
		        0.5f,          // float centerRegion
		        (float)M_PI_2, // float wedgeAngle
		        true,          // bool isSticky
		    }};
		dpad_binding_modification = &dpad_binding_modification_val;

		SECTION("without an activation input")
		{
			input.type = XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE;
			input.value.vec2.x = 0.0f;
			input.value.vec2.y = 1.0f;

			CHECK(oxr_input_transform_create_chain_dpad(
			    &log, &slog, input.type, action_type, "/mock_vec2/dpad_up", dpad_binding_modification,
			    dpad_region, activation_input_type, NULL, &transforms, &transform_count));
			CHECK(transform_count == 1);
			CHECK(transforms != nullptr);
			CHECK(transforms[0].type == INPUT_TRANSFORM_DPAD);

			SECTION("up region is off in center")
			{
				input.value.vec2.x = 0.0f;
				input.value.vec2.y = 0.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(false == output.value.boolean);
			}

			SECTION("up region is on when pointing up")
			{
				input.value.vec2.x = 0.0f;
				input.value.vec2.y = 1.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(true == output.value.boolean);
			}
			SECTION("up region is off when pointing down")
			{
				input.value.vec2.x = 0.0f;
				input.value.vec2.y = -1.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(false == output.value.boolean);
			}

			SECTION("up region stays on when stick moves clockwise to down")
			{
				input.value.vec2.x = 0.0f;
				input.value.vec2.y = 1.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(true == output.value.boolean);
				input.value.vec2.x = 1.0f;
				input.value.vec2.y = 0.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(true == output.value.boolean);
				input.value.vec2.x = 0.0f;
				input.value.vec2.y = -1.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(true == output.value.boolean);
				input.value.vec2.x = 0.0f;
				input.value.vec2.y = 0.0f;
				CHECK(oxr_input_transform_process(transforms, transform_count, &input, &output));
				CHECK(false == output.value.boolean);
			}
		}
	}

	oxr_log_slog(&log, &slog);
	oxr_input_transform_destroy(&transforms);
	CHECK(NULL == transforms);
}
