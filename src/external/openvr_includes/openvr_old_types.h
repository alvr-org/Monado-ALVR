//========= Copyright Valve Corporation ============//

#pragma once

#include "openvr.h"


#ifdef _MSC_VER
#define VR_CAMERA_DECL_ALIGN(x) __declspec(align(x))
#else
#define VR_CAMERA_DECL_ALIGN(x) //
#endif

#if defined(_WIN32)
#define OPENVR_FNTABLE_CALLTYPE __stdcall
#else
#define OPENVR_FNTABLE_CALLTYPE
#endif

namespace vr {

/** These are states the scene application startup process will go through. */
enum EVRApplicationTransitionState
{
	VRApplicationTransition_None = 0,

	VRApplicationTransition_OldAppQuitSent = 10,
	VRApplicationTransition_WaitingForExternalLaunch = 11,

	VRApplicationTransition_NewAppLaunched = 20,
};

/** DEPRECATED - SOFT BOUNDS (use in IVRChaperone_002 and earlier)
 * Corners are in clockwise order.
 * Tracking space center (0,0,0) is the center of the Soft Bounds.
 * It's a a rectangle.
 * 2 sides are parallel to the X axis and 2 sides are parallel to the Z axis.
 * Height of every corner is 0Y (on the floor). */
struct ChaperoneSoftBoundsInfo_t
{
	HmdQuad_t quadCorners;
};

/** DEPRECATED */
struct ChaperoneSeatedBoundsInfo_t
{
	HmdVector3_t vSeatedHeadPosition;
	HmdVector3_t vDeskEdgePositions[2];
};

/** Identifies the graphics API for the associated device */
enum Compositor_DeviceType
{
	Compositor_DeviceType_None,
	Compositor_DeviceType_D3D9,
	Compositor_DeviceType_D3D9Ex,
	Compositor_DeviceType_D3D10,
	Compositor_DeviceType_D3D11,
	Compositor_DeviceType_OpenGL
};

typedef VRTextureBounds_t Compositor_TextureBounds;

/** Allows the application to customize how the overlay appears in the compositor */
struct Compositor_OverlaySettings
{
	uint32_t size; // sizeof(Compositor_OverlaySettings)
	bool curved, antialias;
	float scale, distance, alpha;
	float uOffset, vOffset, uScale, vScale;
	float gridDivs, gridWidth, gridScale;
	HmdMatrix44_t transform;
};

enum EGraphicsAPIConvention
{
	API_DirectX = 0, // Normalized Z goes from 0 at the viewer to 1 at the far clip plane
	API_OpenGL = 1,  // Normalized Z goes from 1 at the viewer to -1 at the far clip plane
};

typedef EGraphicsAPIConvention GraphicsAPIConvention;

struct InputSkeletonActionData_t
{
	// Whether or not this action is currently available to be bound in the active action set
	bool bActive;

	// The origin that caused this action's current state
	VRInputValueHandle_t activeOrigin;
};

enum NotificationError_t
{
	k_ENotificationError_OK = 0,
	k_ENotificationError_Fail = 1,
	k_eNotificationError_InvalidParam = 2,
};

/** Overlay visibility setting controls when the overlay is visible. */
enum VROverlayVisibility
{
	VROverlayVisibility_Manual = 0,        // There is no automatic visibility for the overlay
	VROverlayVisibility_SystemOverlay = 1, // Visibility, input, and transform are controlled by the system button.
};

typedef NotificationBitmap_t NotificationBitmap;

/** Directions for changing focus between overlays with the gamepad */
enum EOverlayDirection
{
	OverlayDirection_Up = 0,
	OverlayDirection_Down = 1,
	OverlayDirection_Left = 2,
	OverlayDirection_Right = 3,

	OverlayDirection_Count = 4,
};

enum EDualAnalogWhich
{
	k_EDualAnalog_Left = 0,
	k_EDualAnalog_Right = 1,
};

/** Describes the component render model */
struct ComponentState_t
{
	HmdMatrix34_t mTrackingToComponentRenderModel;
	HmdMatrix34_t mTrackingToComponentLocal;
	bool bIsStatic;
	bool bIsVisible;
};

enum ECameraVideoStreamFormat
{
	CVS_FORMAT_UNKNOWN = 0,
	CVS_FORMAT_RAW10 = 1, // 10 bits per pixel
	CVS_FORMAT_NV12 = 2,  // 12 bits per pixel
	CVS_FORMAT_RGB24 = 3, // 24 bits per pixel
	CVS_MAX_FORMATS
};

VR_CAMERA_DECL_ALIGN(8)
struct CameraVideoStreamFrame_t
{
	ECameraVideoStreamFormat m_nStreamFormat;

	uint32_t m_nWidth;
	uint32_t m_nHeight;

	uint32_t m_nImageDataSize; // Based on stream format, width, height

	uint32_t m_nFrameSequence; // Starts from 0 when stream starts.

	uint32_t m_nISPFrameTimeStamp; // Driver provided time stamp per driver centric time base
	uint32_t m_nISPReferenceTimeStamp;
	uint32_t m_nSyncCounter;

	uint32_t m_nExposureTime;

	uint32_t m_nBufferIndex; // Identifies which buffer the image data is hosted
	uint32_t m_nBufferCount; // Total number of configured buffers

	double m_flFrameElapsedTime; // Starts from 0 when stream starts. In seconds.

	double m_flFrameCaptureTime; // Relative to when the frame was exposed/captured.
	uint64_t m_nFrameCaptureTicks;

	bool m_bPoseIsValid; // Supplied by HMD layer when used as a tracked camera
	vr::HmdMatrix34_t m_matDeviceToAbsoluteTracking;

	float m_Pad[4];

	void* m_pImageData;
};

}; // namespace vr
