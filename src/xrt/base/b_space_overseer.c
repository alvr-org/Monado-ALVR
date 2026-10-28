// Copyright 2023, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief A implementation of the @ref xrt_space_overseer interface.
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup base
 */

#include "xrt/xrt_space.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_session.h"
#include "xrt/xrt_tracking.h"

#include "os/os_time.h"

#include "math/m_space.h"

#include "util/u_misc.h"
#include "util/u_hashmap.h"
#include "util/u_logging.h"
#include "util/u_pretty_print.h"
#include "b_space_overseer.h"
#include "b_tracked_device.h"

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <pthread.h>


/*
 *
 * Structs and defines.
 *
 */

struct b_space_overseer;

/*!
 * Keeps track of what kind of space it is.
 */
enum u_space_type
{
	U_SPACE_TYPE_NULL,
	U_SPACE_TYPE_POSE,
	U_SPACE_TYPE_OFFSET,
	U_SPACE_TYPE_ROOT,

	/*!
	 * Space designed to be attachable to others, most importantly it is
	 * re-attachable, and in order to move all of the spaces that has this
	 * space as it's parent/next we need a node that can be updated.
	 */
	U_SPACE_TYPE_ATTACHABLE,

	/*!
	 * A pose space whose device was removed. Resolving through this space
	 * yields an invalid relation with no flags set.
	 */
	U_SPACE_TYPE_ZOMBIE,
};

/*!
 * Representing a single space, can be several ones. There should only be one
 * root space per overseer.
 */
struct u_space
{
	struct xrt_space base;

	/*!
	 * The space this space is in.
	 */
	struct u_space *next;

	/*!
	 * The type of the space.
	 */
	enum u_space_type type;

	union {
		struct
		{
			struct xrt_device *xdev;
			enum xrt_input_name xname;

			/*!
			 * Set on pose spaces so space_destroy can take the graph
			 * lock and look up the tracked device via xdev.
			 */
			struct b_space_overseer *overseer;
		} pose;

		struct
		{
			struct xrt_pose pose;
		} offset;
	};
};

/*!
 * Default implementation of the xrt_space_overseer object.
 */
struct b_space_overseer
{
	struct xrt_space_overseer base;

	//! Main graph lock.
	pthread_rwlock_t lock;

	//! Map from xdev to tracked device, each entry holds a reference.
	struct u_hashmap_int *xdev_map;

	//! Map from xrt_tracking_origin to space, each entry holds a reference.
	struct u_hashmap_int *xto_map;

	//! Tracks usage of reference spaces.
	struct xrt_reference ref_space_use[XRT_SPACE_REFERENCE_TYPE_COUNT];

	//! Event sink to broadcast events to all sessions.
	struct xrt_session_event_sink *broadcast;

	/*!
	 * The notify device, usually the head device. Used to notify when
	 * reference spaces are used and not used. Must not change during
	 * runtime.
	 */
	struct xrt_device *notify;

	/*!
	 * Can we do a recenter of the local and local_floor spaces, protected
	 * by the lock.
	 *
	 * This requires that local and local_floor are either null or offset
	 * spaces and that they share the same parent.
	 */
	bool can_do_local_spaces_recenter;

	/*!
	 * Create independent local and local_floor per application
	 */
	bool per_app_local_spaces;
};


/*
 *
 * Helper functions.
 *
 */

static inline struct u_space *
u_space(struct xrt_space *xs)
{
	return (struct u_space *)xs;
}

static inline struct b_space_overseer *
b_space_overseer(struct xrt_space_overseer *xso)
{
	return (struct b_space_overseer *)xso;
}

static const char *
type_to_small_string(enum xrt_reference_space_type type)
{
	switch (type) {
	case XRT_SPACE_REFERENCE_TYPE_VIEW: return "view";
	case XRT_SPACE_REFERENCE_TYPE_LOCAL: return "local";
	case XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR: return "local_floor";
	case XRT_SPACE_REFERENCE_TYPE_STAGE: return "stage";
	case XRT_SPACE_REFERENCE_TYPE_UNBOUNDED: return "unbounded";
	}

	return "invalid";
}

static struct u_space *
get_semantic_space(struct b_space_overseer *uso, enum xrt_reference_space_type type)
{
	switch (type) {
	case XRT_SPACE_REFERENCE_TYPE_VIEW: return u_space(uso->base.semantic.view);
	case XRT_SPACE_REFERENCE_TYPE_LOCAL: return u_space(uso->base.semantic.local);
	case XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR: return u_space(uso->base.semantic.local_floor);
	case XRT_SPACE_REFERENCE_TYPE_STAGE: return u_space(uso->base.semantic.stage);
	case XRT_SPACE_REFERENCE_TYPE_UNBOUNDED: return u_space(uso->base.semantic.unbounded);
	}

	return NULL;
}

/*!
 * A lot of code here uses u_space directly and need to change reference count
 * so this helper is here to make that easier.
 */
static inline void
u_space_reference(struct u_space **dst, struct u_space *src)
{
	struct u_space *old_dst = *dst;

	if (old_dst == src) {
		return;
	}

	if (src) {
		xrt_reference_inc(&src->base.reference);
	}

	*dst = src;

	if (old_dst) {
		if (xrt_reference_dec_and_is_zero(&old_dst->base.reference)) {
			old_dst->base.destroy(&old_dst->base);
		}
	}
}

/*!
 * Helper function when clearing a hashmap to also unreference a space.
 */
static void
hashmap_unreference_space_items(void *item, void *priv)
{
	struct u_space *us = (struct u_space *)item;
	u_space_reference(&us, NULL);
}

static struct b_tracked_device *
find_xdev_tracked_device_read_locked(struct b_space_overseer *uso, struct xrt_device *xdev)
{
	void *ptr = NULL;
	uint64_t key = xdev->id.val;
	u_hashmap_int_find(uso->xdev_map, key, &ptr);

	return (struct b_tracked_device *)ptr;
}

static struct u_space *
find_xto_space_read_locked(struct b_space_overseer *uso, struct xrt_tracking_origin *xto)
{
	void *ptr = NULL;
	uint64_t key = (uint64_t)(intptr_t)xto;
	u_hashmap_int_find(uso->xto_map, key, &ptr);

	if (ptr == NULL) {
		U_LOG_E("Looking for space belonging to unknown xrt_tracking_origin! '%s'", xto->name);
	}
	assert(ptr != NULL);

	return (struct u_space *)ptr;
}

static bool
space_is_offset_compatible(struct u_space *us)
{
	return us != NULL && (us->type == U_SPACE_TYPE_NULL || us->type == U_SPACE_TYPE_OFFSET);
}

/*!
 * Updates the offset of a NULL or OFFSET space.
 */
static void
update_offset_write_locked(struct u_space *us, const struct xrt_pose *new_offset)
{
	assert(us->type == U_SPACE_TYPE_NULL || us->type == U_SPACE_TYPE_OFFSET);

	if (m_pose_is_identity(new_offset)) { // Small optimisation.
		us->type = U_SPACE_TYPE_NULL;
		U_ZERO(&us->offset.pose);
	} else {
		us->type = U_SPACE_TYPE_OFFSET;
		us->offset.pose = *new_offset;
	}
}

/*!
 * Returns the offset for an offset space or an identity pose, it's valid to
 * call on all spaces.
 */
static void
get_offset_or_ident_read_locked(const struct u_space *us, struct xrt_pose *offset)
{
	if (us->type == U_SPACE_TYPE_OFFSET) {
		*offset = us->offset.pose;
	} else {
		*offset = (struct xrt_pose)XRT_POSE_IDENTITY;
	}
}


/*
 *
 * Reference space to device notification code.
 *
 */

static void
notify_ref_space_usage_device(struct b_space_overseer *uso, enum xrt_reference_space_type type, bool used)
{
	struct xrt_device *xdev = NULL;
	enum xrt_input_name name = 0;

	struct u_space *uspace = get_semantic_space(uso, type);
	if (uspace == NULL) {
		// This is weird, should always be a space.
		return;
	}

	if (uspace->type == U_SPACE_TYPE_POSE) {
		xdev = uspace->pose.xdev;
		name = uspace->pose.xname;
	} else {
		xdev = uso->notify;
	}

	if (xdev == NULL || !xdev->supported.ref_space_usage) {
		return;
	}

	xrt_device_ref_space_usage(xdev, type, name, used);
}


/*
 *
 * Zombie space helpers.
 *
 */

static void
zombiefy_space_write_locked(struct u_space *us)
{
	assert(us->type == U_SPACE_TYPE_POSE);

	us->type = U_SPACE_TYPE_ZOMBIE;
	us->pose.xdev = NULL;
	us->pose.xname = 0;
	us->pose.overseer = NULL;
}

static void
zombiefy_pose_space_cb(struct xrt_space *xs, void *priv)
{
	(void)priv;

	struct u_space *us = u_space(xs);
	if (us->type != U_SPACE_TYPE_POSE) {
		return;
	}

	zombiefy_space_write_locked(us);
}

static void
hashmap_unreference_tracked_device_items(void *item, void *priv)
{
	(void)priv;

	struct b_tracked_device *btd = (struct b_tracked_device *)item;

	/*
	 * Pose spaces may outlive the overseer teardown path long enough that
	 * the tracked-device destroy would otherwise see a non-empty pose list.
	 * Zombify and clear first, matching remove_device.
	 */
	b_tracked_device_for_each_pose_space(btd, zombiefy_pose_space_cb, NULL);
	b_tracked_device_clear_pose_spaces(btd);
	b_tracked_device_reference(&btd, NULL);
}


/*
 *
 * Graph traversing functions.
 *
 */

/*!
 * For each space, push the relation of that space and then traverse by calling
 * @p push_then_traverse again with the parent space. That means traverse goes
 * from a leaf space to a the root space, relations are pushed in the same
 * order.
 */
static void
push_then_traverse(struct xrt_relation_chain *xrc, struct u_space *space, int64_t at_timestamp_ns)
{
	switch (space->type) {
	case U_SPACE_TYPE_ZOMBIE:
		m_relation_chain_push_relation(xrc, &(struct xrt_space_relation)XRT_SPACE_RELATION_ZERO);
		return;
	case U_SPACE_TYPE_NULL: break; // No-op
	case U_SPACE_TYPE_POSE: {
		assert(space->pose.xdev != NULL);
		assert(space->pose.xname != 0);

		struct xrt_space_relation xsr;
		xrt_device_get_tracked_pose(space->pose.xdev, space->pose.xname, at_timestamp_ns, &xsr);
		m_relation_chain_push_relation(xrc, &xsr);
	} break;
	case U_SPACE_TYPE_OFFSET: m_relation_chain_push_pose_if_not_identity(xrc, &space->offset.pose); break;
	case U_SPACE_TYPE_ROOT: return;      // Stops the traversing.
	case U_SPACE_TYPE_ATTACHABLE: break; // No-op
	}

	// Please tail-call optimise this miss compiler.
	assert(space->next != NULL);
	push_then_traverse(xrc, space->next, at_timestamp_ns);
}

/*!
 * For each space, traverse by calling @p traverse_then_push_inverse again with
 * the parent space then push the inverse of the relation of that. That means
 * traverse goes from a leaf space to a the root space, relations are pushed in
 * the reversed order.
 */
static void
traverse_then_push_inverse(struct xrt_relation_chain *xrc, struct u_space *space, int64_t at_timestamp_ns)
{
	// Done traversing.
	switch (space->type) {
	case U_SPACE_TYPE_ZOMBIE:
		m_relation_chain_push_relation(xrc, &(struct xrt_space_relation)XRT_SPACE_RELATION_ZERO);
		return;
	case U_SPACE_TYPE_NULL: break;
	case U_SPACE_TYPE_POSE: break;
	case U_SPACE_TYPE_OFFSET: break;
	case U_SPACE_TYPE_ROOT: return;      // Stops the traversing.
	case U_SPACE_TYPE_ATTACHABLE: break; // No-op
	}

	// Can't tail-call optimise this one :(
	assert(space->next != NULL);
	traverse_then_push_inverse(xrc, space->next, at_timestamp_ns);

	switch (space->type) {
	case U_SPACE_TYPE_NULL: break; // No-op
	case U_SPACE_TYPE_POSE: {
		assert(space->pose.xdev != NULL);
		assert(space->pose.xname != 0);

		struct xrt_space_relation xsr;
		xrt_device_get_tracked_pose(space->pose.xdev, space->pose.xname, at_timestamp_ns, &xsr);
		m_relation_chain_push_inverted_relation(xrc, &xsr);
	} break;
	case U_SPACE_TYPE_ZOMBIE: assert(false); break; // Handled above.
	case U_SPACE_TYPE_OFFSET: m_relation_chain_push_inverted_pose_if_not_identity(xrc, &space->offset.pose); break;
	case U_SPACE_TYPE_ROOT: assert(false); // Should not get here.
	case U_SPACE_TYPE_ATTACHABLE: break;   // No-op
	}
}

static void
build_relation_chain_read_locked(struct b_space_overseer *uso,
                                 struct xrt_relation_chain *xrc,
                                 struct u_space *base,
                                 struct u_space *target,
                                 int64_t at_timestamp_ns)
{
	assert(xrc != NULL);
	assert(base != NULL);
	assert(target != NULL);

	push_then_traverse(xrc, target, at_timestamp_ns);
	traverse_then_push_inverse(xrc, base, at_timestamp_ns);
}

static void
build_relation_chain(struct b_space_overseer *uso,
                     struct xrt_relation_chain *xrc,
                     struct u_space *base,
                     struct u_space *target,
                     int64_t at_timestamp_ns)
{
	pthread_rwlock_rdlock(&uso->lock);
	build_relation_chain_read_locked(uso, xrc, base, target, at_timestamp_ns);
	pthread_rwlock_unlock(&uso->lock);
}

static inline void
special_resolve(struct xrt_relation_chain *xrc, struct xrt_space_relation *out_relation)
{
	// A space chain with zero step is always valid.
	if (xrc->step_count == 0) {
		out_relation->pose = (struct xrt_pose)XRT_POSE_IDENTITY;
		out_relation->linear_velocity = (struct xrt_vec3)XRT_VEC3_ZERO;
		out_relation->angular_velocity = (struct xrt_vec3)XRT_VEC3_ZERO;
		out_relation->relation_flags =                     //
		    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |     //
		    XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |   //
		    XRT_SPACE_RELATION_POSITION_VALID_BIT |        //
		    XRT_SPACE_RELATION_POSITION_TRACKED_BIT |      //
		    XRT_SPACE_RELATION_LINEAR_VELOCITY_VALID_BIT | //
		    XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT;
	} else {
		m_relation_chain_resolve(xrc, out_relation);
	}
}


/*
 *
 * Direct space functions.
 *
 */

static void
space_destroy(struct xrt_space *xs)
{
	struct u_space *us = u_space(xs);

	if (us->type == U_SPACE_TYPE_POSE && us->pose.xdev != NULL) {
		struct b_space_overseer *uso = us->pose.overseer;
		assert(uso != NULL);

		pthread_rwlock_wrlock(&uso->lock);
		struct b_tracked_device *btd = find_xdev_tracked_device_read_locked(uso, us->pose.xdev);
		if (btd != NULL) {
			b_tracked_device_untrack_pose_space(btd, xs);
		}
		pthread_rwlock_unlock(&uso->lock);
	}

	assert(us->next != NULL || us->type == U_SPACE_TYPE_ROOT);

	u_space_reference(&us->next, NULL);

	free(us);
}

/*!
 * Creates a space, returns with a reference of one. The lock doesn't need to be
 * held as this function is not modifying any of the currently existing spaces
 * only creating a new one that is not pointed to by any other spaces.
 */
static struct u_space *
create_space(enum u_space_type type, struct u_space *parent)
{
	assert(parent != NULL || type == U_SPACE_TYPE_ROOT);

	struct u_space *us = U_TYPED_CALLOC(struct u_space);
	us->base.reference.count = 1;
	us->base.destroy = space_destroy;
	us->type = type;

	u_space_reference(&us->next, parent);

	return us;
}

static void
create_and_set_root_space(struct b_space_overseer *uso)
{
	assert(uso->base.semantic.root == NULL);

	struct u_space *us = create_space(U_SPACE_TYPE_ROOT, NULL);

	// Created with one reference.
	uso->base.semantic.root = &us->base;
}


/*
 *
 * Device helpers.
 *
 */

/*!
 * Helper function to add a device to the space overseer. This function
 * handles creating or finding a space for the device's tracking origin
 * and linking the device to that space.
 */
static xrt_result_t
add_device_helper(struct b_space_overseer *uso, struct xrt_device *xdev)
{
	struct xrt_tracking_origin *torig = xdev->tracking_origin;
	assert(torig != NULL);

	struct xrt_space *root = uso->base.semantic.root;
	uint64_t key = (uint64_t)(intptr_t)torig;
	struct xrt_space *xs = NULL;

	// Need to take the write lock.
	pthread_rwlock_wrlock(&uso->lock);

	// Does this tracking origin already have space.
	void *ptr = NULL;
	u_hashmap_int_find(uso->xto_map, key, &ptr);

	if (ptr != NULL) {
		xs = (struct xrt_space *)ptr;
	} else if (torig->type == XRT_TRACKING_TYPE_ATTACHABLE) {
		/*
		 * If we ever make b_space_overseer sub-classable make sure
		 * this calls the right function, can't call interface function
		 * as the lock is held here.
		 */
		xs = (struct xrt_space *)create_space(U_SPACE_TYPE_ATTACHABLE, u_space(root));
		u_hashmap_int_insert(uso->xto_map, key, xs);
	} else {
		/*
		 * If we ever make b_space_overseer sub-classable make sure
		 * this calls the right function, can't call interface function
		 * as the lock is held here.
		 */
		xs = (struct xrt_space *)create_space(U_SPACE_TYPE_OFFSET, u_space(root));

		update_offset_write_locked(u_space(xs), &torig->initial_offset);

		u_hashmap_int_insert(uso->xto_map, key, xs);
	}

	pthread_rwlock_unlock(&uso->lock);

	b_space_overseer_link_space_to_device(uso, xs, xdev);

	return XRT_SUCCESS;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
create_offset_space(struct xrt_space_overseer *xso,
                    struct xrt_space *parent,
                    const struct xrt_pose *offset,
                    struct xrt_space **out_space)
{
	assert(out_space != NULL);
	assert(*out_space == NULL);

	struct u_space *uparent = u_space(parent);
	struct u_space *us = NULL;

	if (m_pose_is_identity(offset)) { // Small optimisation.
		us = create_space(U_SPACE_TYPE_NULL, uparent);
	} else {
		us = create_space(U_SPACE_TYPE_OFFSET, uparent);
		us->offset.pose = *offset;
	}

	// Created with one references.
	*out_space = &us->base;

	return XRT_SUCCESS;
}

static xrt_result_t
create_pose_space(struct xrt_space_overseer *xso,
                  struct xrt_device *xdev,
                  enum xrt_input_name name,
                  struct xrt_space **out_space)
{
	assert(out_space != NULL);
	assert(*out_space == NULL);

	struct b_space_overseer *uso = b_space_overseer(xso);

	/*
	 * Write lock: the xdev map lookup alone only needs a read lock, and
	 * create_space does not touch the graph. But b_tracked_device has no
	 * lock of its own, so track_pose_space mutates pose_spaces under this
	 * graph lock and must be exclusive (also vs. untrack / remove_device).
	 * A per-device lock would let this take a read lock for the lookup and
	 * then lock only the tracked device while tracking.
	 */
	pthread_rwlock_wrlock(&uso->lock);

	struct b_tracked_device *btd = find_xdev_tracked_device_read_locked(uso, xdev);
	if (btd == NULL) {
		pthread_rwlock_unlock(&uso->lock);
		U_LOG_E("Creating pose space for unknown xrt_device! '%s'", xdev->str);
		return XRT_ERROR_DEVICE_CREATION_FAILED;
	}

	struct u_space *uparent = u_space(b_tracked_device_get_space(btd));
	struct u_space *us = create_space(U_SPACE_TYPE_POSE, uparent);

	us->pose.xdev = xdev;
	us->pose.xname = name;
	us->pose.overseer = uso;

	b_tracked_device_track_pose_space(btd, &us->base);

	pthread_rwlock_unlock(&uso->lock);

	// Created with one references.
	*out_space = &us->base;

	return XRT_SUCCESS;
}

static xrt_result_t
locate_space(struct xrt_space_overseer *xso,
             struct xrt_space *base_space,
             const struct xrt_pose *base_offset,
             int64_t at_timestamp_ns,
             struct xrt_space *space,
             const struct xrt_pose *offset,
             struct xrt_space_relation *out_relation)
{
	struct b_space_overseer *uso = b_space_overseer(xso);

	struct u_space *ubase_space = u_space(base_space);
	struct u_space *uspace = u_space(space);

	struct xrt_relation_chain xrc = {0};

	m_relation_chain_push_pose_if_not_identity(&xrc, offset);

	/*
	 * Crude optimization: If locating a space in itself, we don't actually
	 * need to locate the space itself. Only the offsets need to be applied.
	 *
	 * This lets zombie spaces still be locatable with themselves after they
	 * have been zombiefied, this might right or wrong, unsure but the
	 * decision was made to do it this way.
	 */
	if (uspace != ubase_space) {
		build_relation_chain(uso, &xrc, ubase_space, uspace, at_timestamp_ns);
	}

	m_relation_chain_push_inverted_pose_if_not_identity(&xrc, base_offset);

	/*
	 * Resolve outside the graph lock (build_relation_chain unlocks first).
	 * Same trade-off as locate_device: a concurrent remove_device may leave a
	 * briefly stale tracked pose; locate results are mostly transient.
	 */
	special_resolve(&xrc, out_relation);

	return XRT_SUCCESS;
}

static bool
pose_approx(const struct xrt_pose *a, const struct xrt_pose *b)
{
	const float e = 0.00001f;
	return fabsf(a->orientation.x - b->orientation.x) < e && //
	       fabsf(a->orientation.y - b->orientation.y) < e && //
	       fabsf(a->orientation.z - b->orientation.z) < e && //
	       fabsf(a->orientation.w - b->orientation.w) < e && //
	       fabsf(a->position.x - b->position.x) < e &&       //
	       fabsf(a->position.y - b->position.y) < e &&       //
	       fabsf(a->position.z - b->position.z) < e;
}

static int32_t
find_same_space_before(struct xrt_space **spaces, const struct xrt_pose *offsets, uint32_t space_index)
{
	for (int32_t i = 0; i < (int32_t)space_index; i++) {
		if (spaces[i] == spaces[space_index] && pose_approx(&offsets[i], &offsets[space_index])) {
			return i;
		}
	}
	return -1;
}

static xrt_result_t
locate_spaces(struct xrt_space_overseer *xso,
              struct xrt_space *base_space,
              const struct xrt_pose *base_offset,
              int64_t at_timestamp_ns,
              struct xrt_space **spaces,
              uint32_t space_count,
              const struct xrt_pose *offsets,
              struct xrt_space_relation *out_relations)
{
	struct b_space_overseer *uso = b_space_overseer(xso);

	struct u_space *ubase_space = u_space(base_space);

	/*
	 * Hold the read lock for the whole loop so space graph reads (and the
	 * duplicate-space copy optimisation) stay consistent with respect to
	 * writers such as remove_device / zombify. Resolving under the lock is
	 * deliberate: deferring it would require buffering a relation chain per
	 * space, and get_tracked_pose already dominates the critical section.
	 */
	pthread_rwlock_rdlock(&uso->lock);

	for (uint32_t i = 0; i < space_count; i++) {
		// spaces are allowed to be NULL
		if (spaces[i] == NULL) {
			out_relations[i].relation_flags = XRT_SPACE_RELATION_BITMASK_NONE;
			continue;
		}

		// crude optimization: If space ptr is equal to one already located, don't locate again, just copy
		{
			int32_t found = find_same_space_before(spaces, offsets, i);
			if (found >= 0) {
				out_relations[i] = out_relations[found];
				continue;
			}
		}

		struct u_space *uspace = u_space(spaces[i]);

		struct xrt_relation_chain xrc = {0};

		m_relation_chain_push_pose_if_not_identity(&xrc, &offsets[i]);

		/*
		 * Crude optimization: If locating a space in itself, we don't actually
		 * need to locate the space itself. Only the offsets need to be applied.
		 *
		 * This lets zombie spaces still be locatable with themselves after they
		 * have been zombiefied, this might right or wrong, unsure but the
		 * decision was made to do it this way.
		 */
		if (spaces[i] != base_space) {
			build_relation_chain_read_locked(uso, &xrc, ubase_space, uspace, at_timestamp_ns);
		}

		m_relation_chain_push_inverted_pose_if_not_identity(&xrc, base_offset);

		// For base_space =~= space (approx equals).
		special_resolve(&xrc, &out_relations[i]);
	}

	pthread_rwlock_unlock(&uso->lock);

	return XRT_SUCCESS;
}

static xrt_result_t
locate_device(struct xrt_space_overseer *xso,
              struct xrt_space *base_space,
              const struct xrt_pose *base_offset,
              int64_t at_timestamp_ns,
              struct xrt_device *xdev,
              struct xrt_space_relation *out_relation)
{
	struct b_space_overseer *uso = b_space_overseer(xso);

	struct u_space *ubase_space = u_space(base_space);

	struct xrt_relation_chain xrc = {0};

	// Only need the read lock.
	pthread_rwlock_rdlock(&uso->lock);

	struct b_tracked_device *btd = find_xdev_tracked_device_read_locked(uso, xdev);
	if (btd == NULL) {
		pthread_rwlock_unlock(&uso->lock);
		*out_relation = (struct xrt_space_relation)XRT_SPACE_RELATION_ZERO;
		return XRT_SUCCESS;
	}

	struct u_space *uspace = u_space(b_tracked_device_get_space(btd));
	build_relation_chain_read_locked(uso, &xrc, ubase_space, uspace, at_timestamp_ns);

	/*
	 * Safe to unlock before resolve. A concurrent remove_device can zombify
	 * between unlock and special_resolve, so we may still report a tracked
	 * pose for a just-removed device. That is fine: locate results are
	 * mostly transient frame data, and the device is gone from the map for
	 * subsequent calls.
	 */
	pthread_rwlock_unlock(&uso->lock);

	// Do as much work outside of the lock.
	m_relation_chain_push_inverted_pose_if_not_identity(&xrc, base_offset);
	special_resolve(&xrc, out_relation);

	return XRT_SUCCESS;
}

static xrt_result_t
ref_space_inc(struct xrt_space_overseer *xso, enum xrt_reference_space_type type)
{
	struct b_space_overseer *uso = b_space_overseer(xso);

	// No more checking then this.
	assert(type < XRT_SPACE_REFERENCE_TYPE_COUNT);

	// If it wasn't zero nothing to do.
	if (!xrt_reference_inc_and_was_zero(&uso->ref_space_use[type])) {
		return XRT_SUCCESS;
	}

	U_LOG_D("Ref-space %s in use", type_to_small_string(type));


	/*
	 * We have a reference space that was not in use but is now in used.
	 */

	// Notify any device that might want to know about it.
	notify_ref_space_usage_device(uso, type, true);

	return XRT_SUCCESS;
}

static xrt_result_t
ref_space_dec(struct xrt_space_overseer *xso, enum xrt_reference_space_type type)
{
	struct b_space_overseer *uso = b_space_overseer(xso);

	// No more checking then this.
	assert(type < XRT_SPACE_REFERENCE_TYPE_COUNT);

	// If it is not zero we are done.
	if (!xrt_reference_dec_and_is_zero(&uso->ref_space_use[type])) {
		return XRT_SUCCESS;
	}

	U_LOG_D("Ref-space %s no longer in use", type_to_small_string(type));


	/*
	 * We have a reference space that was in use but is no longer used.
	 */

	// Notify any device that might want to know about it.
	notify_ref_space_usage_device(uso, type, false);

	return XRT_SUCCESS;
}

static xrt_result_t
recenter_local_spaces(struct xrt_space_overseer *xso)
{
	struct b_space_overseer *uso = b_space_overseer(xso);
	xrt_result_t xret;

	// Take the full lock from the start.
	pthread_rwlock_wrlock(&uso->lock);

	// Can we do recentering, check with lock held.
	if (uso->can_do_local_spaces_recenter) {
		goto err_unlock;
	}


	/*
	 * We go from the view to the parent of local/local_view,
	 * they must share the same parent.
	 */

	uint64_t new_ns = os_monotonic_get_ns();
	struct u_space *uview = u_space(xso->semantic.view);
	struct u_space *ulocal = u_space(xso->semantic.local);
	struct u_space *ulocal_floor = u_space(xso->semantic.local_floor);
	assert(uview != NULL);
	assert(ulocal != NULL);
	assert(ulocal_floor != NULL);

	struct u_space *uparent = ulocal->next;
	assert(uparent != NULL);
	assert(uparent == ulocal_floor->next);


	/*
	 * Get the offset of view in the parent space of local and local_floor.
	 */

	struct xrt_relation_chain xrc = {0};
	build_relation_chain_read_locked(uso, &xrc, uparent, uview, new_ns);

	struct xrt_space_relation rel;
	special_resolve(&xrc, &rel);

	bool pos_valid = (rel.relation_flags & XRT_SPACE_RELATION_POSITION_VALID_BIT) != 0;
	bool ori_valid = (rel.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) != 0;

	if (!pos_valid || !ori_valid) {
		goto err_unlock;
	}


	/*
	 * Calculate new offsets for the spaces.
	 */

	// Only save the rotation around y axis.
	rel.pose.orientation.x = 0;
	rel.pose.orientation.z = 0;
	math_quat_normalize(&rel.pose.orientation);

	struct xrt_pose local_offset, local_floor_offset;
	get_offset_or_ident_read_locked(ulocal, &local_offset);
	get_offset_or_ident_read_locked(ulocal_floor, &local_floor_offset);

	// Take the "flat" rotations and apply to both.
	local_offset.orientation = rel.pose.orientation;
	local_floor_offset.orientation = rel.pose.orientation;

	// Keep y offset the same.
	local_offset.position.x = rel.pose.position.x;
	local_offset.position.z = rel.pose.position.z;
	local_floor_offset.position.x = rel.pose.position.x;
	local_floor_offset.position.z = rel.pose.position.z;

	// Update the offsets.
	update_offset_write_locked(ulocal, &local_offset);
	update_offset_write_locked(ulocal_floor, &local_floor_offset);

	// Push the events.
	union xrt_session_event xse = XRT_STRUCT_INIT;

	// Basics
	xse.ref_change.event_type = XRT_SESSION_EVENT_REFERENCE_SPACE_CHANGE_PENDING;
	xse.ref_change.pose_valid = false;
	xse.ref_change.pose_in_previous_space = (struct xrt_pose)XRT_POSE_IDENTITY;
	xse.ref_change.timestamp_ns = os_monotonic_get_ns();

	// Event for local space.
	xse.ref_change.ref_type = XRT_SPACE_REFERENCE_TYPE_LOCAL;
	xret = xrt_session_event_sink_push(uso->broadcast, &xse);
	if (xret != XRT_SUCCESS) {
		U_LOG_E("Failed to push event for LOCAL!");
	}

	// Event for local floor space.
	xse.ref_change.ref_type = XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR;
	xret = xrt_session_event_sink_push(uso->broadcast, &xse);
	if (xret != XRT_SUCCESS) {
		U_LOG_E("Failed to push event LOCAL_FLOOR!");
	}

	pthread_rwlock_unlock(&uso->lock);

	return XRT_SUCCESS;

err_unlock:
	pthread_rwlock_unlock(&uso->lock);

	return XRT_ERROR_RECENTERING_NOT_SUPPORTED;
}

static xrt_result_t
create_local_space(struct xrt_space_overseer *xso,
                   struct xrt_space **out_local_space,
                   struct xrt_space **out_local_floor_space)
{
	assert(xso->semantic.root != NULL);
	struct b_space_overseer *uso = b_space_overseer(xso);
	if (!uso->per_app_local_spaces) {
		xrt_space_reference(out_local_space, xso->semantic.local);
		xrt_space_reference(out_local_floor_space, xso->semantic.local_floor);
		return XRT_SUCCESS;
	}

	struct xrt_pose identity = XRT_POSE_IDENTITY;
	struct xrt_space_relation xsr = XRT_SPACE_RELATION_ZERO;
	xrt_space_overseer_locate_space(xso, xso->semantic.root, &identity, os_monotonic_get_ns(), xso->semantic.view,
	                                &identity, &xsr);

	bool pos_valid = (xsr.relation_flags & XRT_SPACE_RELATION_POSITION_VALID_BIT) != 0;
	bool ori_valid = (xsr.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) != 0;
	if (!pos_valid || !ori_valid) {
		xsr.pose = (struct xrt_pose)XRT_POSE_IDENTITY;
		xsr.pose.position.y = 1.6;
	}

	xsr.pose.orientation.x = 0;
	xsr.pose.orientation.z = 0;
	math_quat_normalize(&xsr.pose.orientation);

	xrt_result_t xret = XRT_SUCCESS;

	if (out_local_space != NULL) {
		xret = create_offset_space(xso, xso->semantic.root, &xsr.pose, out_local_space);
		if (xret != XRT_SUCCESS) {
			U_LOG_E("Failed to create offset space LOCAL!");
			return xret;
		}
	}

	if (out_local_floor_space != NULL) {
		if (xso->semantic.stage != NULL) {
			struct u_space *ustage = u_space(xso->semantic.stage);
			xsr.pose.position.y = ustage->offset.pose.position.y;
		} else {
			xsr.pose.position.y = 0;
		}
		xret = create_offset_space(xso, xso->semantic.root, &xsr.pose, out_local_floor_space);
		if (xret != XRT_SUCCESS) {
			U_LOG_E("Failed to create offset space LOCAL_FLOOR!");
			return xret;
		}
	}

	return xret;
}

static xrt_result_t
get_tracking_origin_offset(struct xrt_space_overseer *xso, struct xrt_tracking_origin *xto, struct xrt_pose *out_offset)
{
	struct b_space_overseer *uso = b_space_overseer(xso);
	xrt_result_t xret = XRT_SUCCESS;

	pthread_rwlock_rdlock(&uso->lock);

	struct u_space *us = find_xto_space_read_locked(uso, xto);
	if (!space_is_offset_compatible(us)) {
		xret = XRT_ERROR_UNSUPPORTED_SPACE_TYPE;
		goto unlock;
	}

	get_offset_or_ident_read_locked(us, out_offset);

unlock:
	pthread_rwlock_unlock(&uso->lock);
	return xret;
}

static xrt_result_t
set_tracking_origin_offset(struct xrt_space_overseer *xso,
                           struct xrt_tracking_origin *xto,
                           const struct xrt_pose *offset)
{
	struct b_space_overseer *uso = b_space_overseer(xso);
	xrt_result_t xret = XRT_SUCCESS;

	pthread_rwlock_rdlock(&uso->lock);

	struct u_space *us = find_xto_space_read_locked(uso, xto);
	if (!space_is_offset_compatible(us)) {
		xret = XRT_ERROR_UNSUPPORTED_SPACE_TYPE;
		goto unlock;
	}

	update_offset_write_locked(us, offset);

unlock:
	pthread_rwlock_unlock(&uso->lock);
	return xret;
}

static xrt_result_t
get_reference_space_offset(struct xrt_space_overseer *xso,
                           enum xrt_reference_space_type type,
                           struct xrt_pose *out_offset)
{
	struct b_space_overseer *uso = b_space_overseer(xso);
	xrt_result_t xret = XRT_SUCCESS;

	pthread_rwlock_rdlock(&uso->lock);

	struct u_space *us = get_semantic_space(uso, type);
	if (!space_is_offset_compatible(us)) {
		xret = XRT_ERROR_UNSUPPORTED_SPACE_TYPE;
		goto unlock;
	}

	get_offset_or_ident_read_locked(us, out_offset);

unlock:
	pthread_rwlock_unlock(&uso->lock);
	return xret;
}

static xrt_result_t
set_reference_space_offset(struct xrt_space_overseer *xso,
                           enum xrt_reference_space_type type,
                           const struct xrt_pose *offset)
{
	if (type == XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR) {
		// LOCAL_FLOOR is calculated from LOCAL and STAGE
		return XRT_ERROR_UNSUPPORTED_SPACE_TYPE;
	}

	struct b_space_overseer *uso = b_space_overseer(xso);

	if (uso->can_do_local_spaces_recenter) {
		return XRT_ERROR_RECENTERING_NOT_SUPPORTED;
	}

	xrt_result_t xret = XRT_SUCCESS;

	pthread_rwlock_wrlock(&uso->lock);

	struct u_space *us = get_semantic_space(uso, type);
	if (!space_is_offset_compatible(us)) {
		xret = XRT_ERROR_UNSUPPORTED_SPACE_TYPE;
		goto unlock;
	}

	// can_do_local_spaces_recenter ensures that local_floor can be offset
	struct u_space *ufloor = u_space(xso->semantic.local_floor);
	struct xrt_pose floor;
	get_offset_or_ident_read_locked(ufloor, &floor);

	if (type == XRT_SPACE_REFERENCE_TYPE_STAGE) {
		floor.position.y = offset->position.y;
	} else if (type == XRT_SPACE_REFERENCE_TYPE_LOCAL) {
		floor.orientation = offset->orientation;
		floor.position.x = offset->position.x;
		floor.position.z = offset->position.z;
	}

	update_offset_write_locked(us, offset);
	update_offset_write_locked(ufloor, &floor);

	// Push the events.
	union xrt_session_event xse = XRT_STRUCT_INIT;

	// Basics
	xse.ref_change.event_type = XRT_SESSION_EVENT_REFERENCE_SPACE_CHANGE_PENDING;
	xse.ref_change.pose_valid = false;
	xse.ref_change.pose_in_previous_space = (struct xrt_pose)XRT_POSE_IDENTITY;
	xse.ref_change.timestamp_ns = os_monotonic_get_ns();

	if (type == XRT_SPACE_REFERENCE_TYPE_STAGE) {
		xse.ref_change.ref_type = XRT_SPACE_REFERENCE_TYPE_STAGE;
		xret = xrt_session_event_sink_push(uso->broadcast, &xse);
		if (xret != XRT_SUCCESS) {
			U_LOG_E("Failed to push event STAGE!");
		}
	} else if (type == XRT_SPACE_REFERENCE_TYPE_LOCAL) {
		xse.ref_change.ref_type = XRT_SPACE_REFERENCE_TYPE_LOCAL;
		xret = xrt_session_event_sink_push(uso->broadcast, &xse);
		if (xret != XRT_SUCCESS) {
			U_LOG_E("Failed to push event LOCAL!");
		}
	} else {
		// did not change STAGE or LOCAL -> LOCAL_FLOOR also did not change
		goto unlock;
	}

	xse.ref_change.ref_type = XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR;
	xret = xrt_session_event_sink_push(uso->broadcast, &xse);
	if (xret != XRT_SUCCESS) {
		U_LOG_E("Failed to push event LOCAL_FLOOR!");
	}

unlock:
	pthread_rwlock_unlock(&uso->lock);
	return xret;
}

static xrt_result_t
add_device(struct xrt_space_overseer *xso, struct xrt_device *xdev)
{
	struct b_space_overseer *uso = b_space_overseer(xso);

	return add_device_helper(uso, xdev);
}

static xrt_result_t
remove_device(struct xrt_space_overseer *xso, struct xrt_device *xdev)
{
	struct b_space_overseer *uso = b_space_overseer(xso);

	pthread_rwlock_wrlock(&uso->lock);

	struct b_tracked_device *btd = find_xdev_tracked_device_read_locked(uso, xdev);
	if (btd != NULL) {
		b_tracked_device_for_each_pose_space(btd, zombiefy_pose_space_cb, NULL);
		b_tracked_device_clear_pose_spaces(btd);

		uint64_t key = xdev->id.val;
		u_hashmap_int_erase(uso->xdev_map, key);
	}

	pthread_rwlock_unlock(&uso->lock);

	/*
	 * Drop the reference the device held on its tracked device outside of the
	 * lock. The per-tracking-origin space in xto_map is shared between devices
	 * so it is intentionally left in place.
	 */
	b_tracked_device_reference(&btd, NULL);

	return XRT_SUCCESS;
}

static xrt_result_t
attach_device(struct xrt_space_overseer *xso, struct xrt_device *xdev, struct xrt_space *space)
{
	struct b_space_overseer *uso = b_space_overseer(xso);

	// Check that the device has the correct tracking origin type.
	if (xdev->tracking_origin == NULL || xdev->tracking_origin->type != XRT_TRACKING_TYPE_ATTACHABLE) {
		U_LOG_E("Device '%s' does not have XRT_TRACKING_TYPE_ATTACHABLE tracking origin type", xdev->str);
		return XRT_ERROR_DEVICE_NOT_ATTACHABLE;
	}

	// If no space is provided, use the root space.
	struct xrt_space *target_space = space;
	if (target_space == NULL) {
		target_space = uso->base.semantic.root;
	}

	xrt_result_t xret = XRT_SUCCESS;
	pthread_rwlock_wrlock(&uso->lock);


	void *ptr = NULL;
	uint64_t key = (uint64_t)(intptr_t)xdev->tracking_origin;
	u_hashmap_int_find(uso->xto_map, key, &ptr);
	if (ptr == NULL) {
		U_LOG_E("Device doesn't have space associated with it!");
		xret = XRT_ERROR_DEVICE_NOT_ATTACHABLE;
		goto err_unlock;
	}

	struct u_space *us = (struct u_space *)ptr;
	if (us->type != U_SPACE_TYPE_ATTACHABLE) {
		U_LOG_E("Device doesn't have a attachable space!");
		xret = XRT_ERROR_DEVICE_NOT_ATTACHABLE;
		goto err_unlock;
	}

	// Update the link.
	u_space_reference(&us->next, u_space(target_space));

err_unlock:
	pthread_rwlock_unlock(&uso->lock);

	return xret;
}

static void
destroy(struct xrt_space_overseer *xso)
{
	struct b_space_overseer *uso = b_space_overseer(xso);

	xrt_space_reference(&uso->base.semantic.unbounded, NULL);
	xrt_space_reference(&uso->base.semantic.stage, NULL);
	xrt_space_reference(&uso->base.semantic.local, NULL);
	xrt_space_reference(&uso->base.semantic.local_floor, NULL);
	xrt_space_reference(&uso->base.semantic.view, NULL);
	xrt_space_reference(&uso->base.semantic.root, NULL);

	// Each device has a reference to its tracked device, make sure to unreference before destroying.
	u_hashmap_int_clear_and_call_for_each(uso->xdev_map, hashmap_unreference_tracked_device_items, uso);
	u_hashmap_int_destroy(&uso->xdev_map);

	u_hashmap_int_clear_and_call_for_each(uso->xto_map, hashmap_unreference_space_items, uso);
	u_hashmap_int_destroy(&uso->xto_map);

	for (int id = 0; id < XRT_MAX_CLIENT_SPACES; id++) {
		struct xrt_space **xslocal_ptr = (struct xrt_space **)&xso->localspace[id];
		xrt_space_reference(xslocal_ptr, NULL);

		struct xrt_space **xslocalfloor_ptr = (struct xrt_space **)&xso->localfloorspace[id];
		xrt_space_reference(xslocalfloor_ptr, NULL);
	}

	pthread_rwlock_destroy(&uso->lock);

	free(uso);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct b_space_overseer *
b_space_overseer_create(struct xrt_session_event_sink *broadcast)
{
	struct b_space_overseer *uso = U_TYPED_CALLOC(struct b_space_overseer);
	uso->base.create_local_space = create_local_space;
	uso->base.create_offset_space = create_offset_space;
	uso->base.create_pose_space = create_pose_space;
	uso->base.locate_space = locate_space;
	uso->base.locate_spaces = locate_spaces;
	uso->base.locate_device = locate_device;
	uso->base.ref_space_inc = ref_space_inc;
	uso->base.ref_space_dec = ref_space_dec;
	uso->base.recenter_local_spaces = recenter_local_spaces;
	uso->base.get_tracking_origin_offset = get_tracking_origin_offset;
	uso->base.set_tracking_origin_offset = set_tracking_origin_offset;
	uso->base.get_reference_space_offset = get_reference_space_offset;
	uso->base.set_reference_space_offset = set_reference_space_offset;
	uso->base.add_device = add_device;
	uso->base.remove_device = remove_device;
	uso->base.attach_device = attach_device;
	uso->base.destroy = destroy;
	uso->broadcast = broadcast;

	XRT_MAYBE_UNUSED int ret = 0;

	ret = pthread_rwlock_init(&uso->lock, NULL);
	assert(ret == 0);

	ret = u_hashmap_int_create(&uso->xdev_map);
	assert(ret == 0);

	ret = u_hashmap_int_create(&uso->xto_map);
	assert(ret == 0);

	create_and_set_root_space(uso);

	return uso;
}

void
b_space_overseer_legacy_setup(struct b_space_overseer *uso,
                              struct xrt_device **xdevs,
                              uint32_t xdev_count,
                              struct xrt_device *head,
                              const struct xrt_pose *local_offset,
                              bool root_is_unbounded,
                              bool per_app_local_spaces)
{
	uso->per_app_local_spaces = per_app_local_spaces;

	// Add all devices to the space overseer.
	for (uint32_t i = 0; i < xdev_count; i++) {
		xrt_result_t xret = add_device_helper(uso, xdevs[i]);
		if (xret != XRT_SUCCESS) {
			U_LOG_E("Failed to add device '%s' to space overseer!", xdevs[i]->str);
		}
	}

	// If these are set something is probably wrong, but just in case unset them.
	assert(uso->base.semantic.view == NULL);
	assert(uso->base.semantic.stage == NULL);
	assert(uso->base.semantic.local == NULL);
	assert(uso->base.semantic.unbounded == NULL);
	xrt_space_reference(&uso->base.semantic.view, NULL);
	xrt_space_reference(&uso->base.semantic.stage, NULL);
	xrt_space_reference(&uso->base.semantic.local, NULL);
	xrt_space_reference(&uso->base.semantic.unbounded, NULL);

	if (head != NULL && head->supported.stage) {
		// stage poses are polled from the driver
		b_space_overseer_create_pose_space(uso, head, XRT_INPUT_GENERIC_STAGE_SPACE_POSE,
		                                   &uso->base.semantic.stage);
	} else {
		// stage offset is managed by space overseer
		b_space_overseer_create_null_space(uso, uso->base.semantic.root, &uso->base.semantic.stage);
	}

	// If the system wants to support the space, set root as unbounded.
	if (root_is_unbounded) {
		xrt_space_reference(&uso->base.semantic.unbounded, uso->base.semantic.root);
	}

	// Set local to the local offset.
	b_space_overseer_create_offset_space(uso, uso->base.semantic.root, local_offset, &uso->base.semantic.local);

	// Set local floor to be under local, but at y == 0 from stage.
	struct xrt_pose local_floor_offset = {
	    local_offset->orientation,
	    {local_offset->position.x, 0.f, local_offset->position.z},
	};

	b_space_overseer_create_offset_space(uso, uso->base.semantic.root, &local_floor_offset,
	                                     &uso->base.semantic.local_floor);

	// Setup view space if we have a head.
	if (head != NULL) {
		b_space_overseer_create_pose_space(uso, head, XRT_INPUT_GENERIC_HEAD_POSE, &uso->base.semantic.view);

		// Set the head to the notify device, for reference space usage.
		uso->notify = head;
	}
}

void
b_space_overseer_create_null_space(struct b_space_overseer *uso, struct xrt_space *parent, struct xrt_space **out_space)
{
	assert(out_space != NULL);
	assert(*out_space == NULL);

	struct u_space *uparent = u_space(parent);
	struct u_space *us = create_space(U_SPACE_TYPE_NULL, uparent);

	// Created with one references.
	*out_space = &us->base;
}

void
b_space_overseer_link_space_to_device(struct b_space_overseer *uso, struct xrt_space *xs, struct xrt_device *xdev)
{
	pthread_rwlock_wrlock(&uso->lock);

	void *ptr = NULL;
	uint64_t key = xdev->id.val;
	u_hashmap_int_find(uso->xdev_map, key, &ptr);
	if (ptr != NULL) {
		/*
		 * Re-linking the same device must not happen in production.
		 * Pose spaces stay on the old b_tracked_device pose list; dropping
		 * that object here skips zombify on remove_device and can leave
		 * destroy asserting on a non-empty pose list.
		 */
		U_LOG_W("Device '%s' (name=%s, id=%" PRIu64 ") already has a space attached!", xdev->str,
		        u_str_xrt_device_name(xdev->name), xdev->id.val);
	}

	struct b_tracked_device *new_btd = b_tracked_device_create(xs);

	u_hashmap_int_insert(uso->xdev_map, key, new_btd);

	pthread_rwlock_unlock(&uso->lock);

	// Dereference old tracked device outside of lock.
	struct b_tracked_device *old_btd = (struct b_tracked_device *)ptr;
	b_tracked_device_reference(&old_btd, NULL);
}
