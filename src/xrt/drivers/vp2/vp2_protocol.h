// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vive Pro 2 HID protocol definitions.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_vp2
 */

#pragma once

#include "xrt/xrt_byte_order.h"

#include <assert.h>
#include <stdint.h>


//! Size of every report, including the report ID.
#define VP2_DATA_SIZE 64

//! Largest payload after struct vp2_feature_header, larger ones are rejected with @ref VP2_ACK_INVALID.
#define VP2_PAYLOAD_SIZE 60

/*!
 * Report IDs.
 *
 * VP2_REPORT_ID_COMMAND and VP2_REPORT_ID_FEATURE carry the same binary commands (struct vp2_feature_header followed
 * by the payload), the MCU handles them the same way. Commands that reply do so with an input report on the same ID.
 *
 * VP2_REPORT_ID_STRING takes an ASCII command right after the report ID and replies with [id][size][ascii]. The
 * commands start with "mfg-", "usr-" or "eng-". "mfg-r-" reads factory data and "mfg-s-" queries the hardware, but
 * "mfg-s-" also has commands that reboot or write protect the MCU. Unknown commands reply with "-22". For example:
 * - "mfg-r-devsn", "mfg-r-pcbsn", "mfg-r-pcbid" and "mfg-r-skuid" read factory strings.
 * - "mfg-r-ps_info" reads the proximity sensor and its calibration.
 * - "mfg-s-bridgeid" reads the video bridge chip ID, firmware version and hotplug state.
 * - "mfg-s-audioinfo" reads the audio version, amp power limit, gain, sample rate and button.
 * - "mfg-s-hdmi" reads the reply of the last display request getter, see @ref VP2_COMMAND_DISPLAY_REQUEST.
 * - "mfg-s-panel" reads the display flags, power state, CPLD register 3, the frame period of each panel and the video
 *   bridge IRQ line.
 * - "usr-w-brightness=<N>" doesn't set the brightness, it power cycles and reprograms the panels.
 */
enum vp2_report_id
{
	VP2_REPORT_ID_COMMAND = 0x01, //!< Binary commands as output reports
	VP2_REPORT_ID_STRING = 0x02,  //!< ASCII commands
	VP2_REPORT_ID_STATUS = 0x03,  //!< struct vp2_status_report
	VP2_REPORT_ID_FEATURE =
	    0x04, //!< Binary commands as feature reports, reading it returns struct vp2_feature_state
};

/*!
 * Binary commands, the sub_id of struct vp2_feature_header.
 *
 * VP2_COMMAND_DISPLAY_REQUEST takes an ASCII request and is handled in the background, one at a time. Requests that
 * arrive while one is still running are dropped. Some of them:
 * - "wireless,<0/1>" is persisted, it has to be 0 for dtd 3 and above.
 * - "dtd,<N>" selects the timing, a single digit, see @ref enum vp2_resolution. 6 and above fall back to 0.
 * - "chipreset" resets the video bridge and sets it up again with the EDID for the current dtd.
 * - "setbrightness,<N>" sets the brightness, the HTC driver sends 80 to 130.
 * - Getters like "fwname", "powermode", "framerate", "errcnt" and "panelstatus" put their reply in a buffer that is
 *   read with "mfg-s-hdmi" on VP2_REPORT_ID_STRING. Requests run in the same loop that handles the reports, so the
 *   reply is ready by the time the MCU handles the next report.
 */
enum vp2_command
{
	VP2_COMMAND_GET_SOFTWARE_VERSION =
	    0x2266, //!< Replies with the MCU version as 16 hex digits, e.g. "0000000000203056"
	VP2_COMMAND_DISPLAY_REQUEST = 0x2970, //!< ASCII display request, see above, no reply
	VP2_COMMAND_CODEC_REQUEST = 0x2971,   //!< "codecreg=<reg>,<value>" or "hpampreg=<reg>,<value>" in hex, no reply
	VP2_COMMAND_FEATURE_EVENT = 0x2978,   //!< @ref struct vp2_feature_event, no reply
	VP2_COMMAND_FEATURE_STATE = 0x2987,   //!< @ref struct vp2_feature_state, read as a feature report
	VP2_COMMAND_STATUS_REPORT = 0x2CD0,   //!< @ref struct vp2_status_report
	VP2_COMMAND_DEBUG_REPORT = 0x2CD2,    //!< Named by the HTC driver, never sent by the MCU
	VP2_COMMAND_SYSTEM_REBOOT = 0x5566,   //!< Replies with VP2_ACK_OK, then resets the MCU
	VP2_COMMAND_REPORT = 0x7788,          //!< Named by the HTC driver, never sent by the MCU

	VP2_COMMAND_LOG_READ_START = 0xB0EA,    //!< Replies with the size of the log as a __le32, 0xFFFFFFFF if none
	VP2_COMMAND_LOG_READ = 0xB0EB,          //!< @ref struct vp2_config_read_request, replies with up to 60 bytes
	VP2_COMMAND_LOG_READ_ABORT = 0xB0EC,    //!< Replies with a __le32 0
	VP2_COMMAND_LOG_READ_END = 0xB0ED,      //!< Replies with a __le32 0
	VP2_COMMAND_CONFIG_READ_START = 0xB1EA, //!< Replies with the size of the config blob as a __le32
	VP2_COMMAND_CONFIG_READ = 0xB1EB,       //!< @ref struct vp2_config_read_request, replies with up to 60 bytes
	VP2_COMMAND_CONFIG_READ_ABORT = 0xB1EC, //!< Replies with a __le32 0
	VP2_COMMAND_CONFIG_READ_END = 0xB1ED,   //!< Replies with a __le32 0
};

/*!
 * Replies of commands that only acknowledge, as ASCII after a struct vp2_feature_header with the same report ID and
 * command. Unknown commands get VP2_ACK_INVALID.
 */
#define VP2_ACK_OK "1"
#define VP2_ACK_FAILED "-1"
#define VP2_ACK_INVALID "-22"

/*!
 * Transfers, the family is the high byte of the command and the step is the low byte, see VP2_TRANSFER_COMMAND.
 *
 * Write transfers start with struct vp2_transfer_start, send the data in work steps of up to 60 bytes, then end with
 * an empty end step that checks the CRC. Every step replies with VP2_ACK_OK or VP2_ACK_FAILED.
 */
enum vp2_transfer_family
{
	VP2_TRANSFER_APP = 0xA0,          //!< MCU application, only handled by the bootloader
	VP2_TRANSFER_IAP = 0xA1,          //!< Rewrites the bootloader, don't use
	VP2_TRANSFER_FPGA = 0xA2,         //!< CPLD image
	VP2_TRANSFER_AUDIO = 0xA3,        //!< Audio codec firmware
	VP2_TRANSFER_VBRIDGE = 0xA4,      //!< Video bridge firmware
	VP2_TRANSFER_CONFIG_WRITE = 0xA5, //!< Rewrites the config blob, don't use
	VP2_TRANSFER_LOG_READ = 0xB0,     //!< See VP2_COMMAND_LOG_READ
	VP2_TRANSFER_CONFIG_READ = 0xB1,  //!< See VP2_COMMAND_CONFIG_READ
};

enum vp2_transfer_step
{
	VP2_TRANSFER_STEP_START = 0xEA,
	VP2_TRANSFER_STEP_WORK = 0xEB,
	VP2_TRANSFER_STEP_ABORT = 0xEC,
	VP2_TRANSFER_STEP_END = 0xED,
};

#define VP2_TRANSFER_COMMAND(family, step) ((uint16_t)(((family) << 8) | (step)))

//! Starting value of the transfer CRC-32, "Vive" in ASCII. In zlib terms the CRC is crc32(VP2_TRANSFER_CRC_SEED, data).
#define VP2_TRANSFER_CRC_SEED 0x56697665u

//! Size of the header in front of the JSON in the config blob, struct vp2_config_header.
#define VP2_CONFIG_HEADER_SIZE 128

//! Largest config blob the MCU will return, including the header.
#define VP2_CONFIG_MAX_SIZE 0x8000

//! Magic of struct vp2_config_header, the CRC-32 of "viveconfig".
#define VP2_CONFIG_MAGIC 0x3EE4DFDCu

/*!
 * Valid mask bits of struct vp2_feature_event. Bits 4 and 63 are accepted but do nothing.
 *
 * VP2_FEATURE_EVENT_PROXIMITY_OVERRIDE and VP2_FEATURE_EVENT_WIRELESS_AUTH are only handled in wireless mode. The auth
 * token is big endian, bit 7 of the first byte is a separate flag and the other 23 bits have to match the top 23 bits
 * of the CRC-32 of the MCU's 12 character serial number (generated from its unique ID) followed by
 * @ref vp2_status_report::nonce. Once accepted the proximity override is honoured until the authentication times out.
 *
 * VP2_FEATURE_EVENT_THERMAL_WATCHDOG controls a check that runs every 5 seconds: the MCU stops feeding its watchdog,
 * and so resets itself, while @ref vp2_status_report::thermal_adc is nonzero and below 0x371.
 */
#define VP2_FEATURE_EVENT_STATUS_REPORT (1ull << 0)
#define VP2_FEATURE_EVENT_AMP_GAIN (1ull << 1)
#define VP2_FEATURE_EVENT_BRIGHTNESS_REFRESH (1ull << 2)
#define VP2_FEATURE_EVENT_AMP_POWER_LIMIT (1ull << 3)
#define VP2_FEATURE_EVENT_AUDIO_ROUTE (1ull << 5)
#define VP2_FEATURE_EVENT_AUDIO_PROFILE (1ull << 6)
#define VP2_FEATURE_EVENT_DISPLAY_POWER (1ull << 48)
#define VP2_FEATURE_EVENT_POWER_STATE (1ull << 49)
#define VP2_FEATURE_EVENT_PROXIMITY_OVERRIDE (1ull << 50)
#define VP2_FEATURE_EVENT_WIRELESS_AUTH (1ull << 51)
#define VP2_FEATURE_EVENT_THERMAL_WATCHDOG (1ull << 62)

#define VP2_DISPLAY_POWER_XOR 0xC4
#define VP2_POWER_STATE_XOR 0x78
#define VP2_PROXIMITY_OVERRIDE_XOR 0x10
#define VP2_THERMAL_WATCHDOG_XOR 0x80

/*!
 * Power states for @ref VP2_FEATURE_EVENT_POWER_STATE, the HTC driver uses ready, idle and standby. 1 is treated the
 * same as ready.
 */
enum vp2_power_state
{
	VP2_POWER_STATE_READY = 2,   //!< Display on
	VP2_POWER_STATE_IDLE = 3,    //!< Display fully powered off, stays off until ready is sent
	VP2_POWER_STATE_STANDBY = 4, //!< Backlights off, the panels aren't reprogrammed when going back to ready
	VP2_POWER_STATE_SLEEP = 5,   //!< Display and sensors off
};

//! Bits of @ref vp2_feature_state::display_flags.
enum vp2_display_flags
{
	VP2_DISPLAY_FLAG_ON = 1 << 0,                 //!< Display on
	VP2_DISPLAY_FLAG_BRIDGE_ON = 1 << 5,          //!< Video bridge powered
	VP2_DISPLAY_FLAG_PANELS_INITIALIZED = 1 << 6, //!< Set the first time the panels power on, never cleared
	VP2_DISPLAY_FLAG_WIRELESS = 1 << 7,           //!< The persisted "wireless,1" display request setting
};

//! Bits of vp2_feature_event::thermal_watchdog, before the XOR.
enum vp2_thermal_watchdog_flags
{
	VP2_THERMAL_WATCHDOG_SKIP_CHECK = 1 << 0,  //!< Don't check the thermal ADC
	VP2_THERMAL_WATCHDOG_ALWAYS_FEED = 1 << 1, //!< Keep feeding the watchdog whatever the thermal ADC says
};

//! Audio buttons in struct vp2_status_report, only one is reported at a time.
enum vp2_audio_button
{
	VP2_AUDIO_BUTTON_NONE = 0,
	VP2_AUDIO_BUTTON_VOLUME_UP = 1,
	VP2_AUDIO_BUTTON_VOLUME_DOWN = 2,
	VP2_AUDIO_BUTTON_MUTE_MIC = 4,
};

//! @ref vp2_status_report::proximity_trend
enum vp2_proximity_trend
{
	VP2_PROXIMITY_TREND_STEADY = 0,
	VP2_PROXIMITY_TREND_FALLING = 1,
	VP2_PROXIMITY_TREND_RISING = 2,
};

//! Proximity values above this mean the headset is being worn.
#define VP2_PROXIMITY_THRESHOLD 150

//! Unit of @ref vp2_status_report::ipd in meters.
#define VP2_IPD_UNIT_METERS 0.00001f

#pragma pack(push, 1)

//! Header of the binary commands, the payload follows.
struct vp2_feature_header
{
	uint8_t id;
	__le16 sub_id; //!< @ref enum vp2_command
	uint8_t size;  //!< Size of the payload, up to @ref VP2_PAYLOAD_SIZE
};

//! Request for VP2_COMMAND_CONFIG_READ and VP2_COMMAND_LOG_READ, sent after the report ID.
struct vp2_config_read_request
{
	__le16 sub_id; //!< @ref VP2_COMMAND_CONFIG_READ
	uint8_t size;  //!< Size of offset
	__le32 offset; //!< Offset into the config blob (including the header) or the log
};

//! Payload of the start step of a write transfer.
struct vp2_transfer_start
{
	__le32 size; //!< Size of the data, checked against a limit for each family
	__le32 crc;  //!< CRC-32 of the data, see @ref VP2_TRANSFER_CRC_SEED
};

/*!
 * Payload of @ref VP2_COMMAND_FEATURE_EVENT. Only the fields with their bit set in valid_mask are applied. Display
 * power and power state are ignored while the CPLD reports an abnormal state, see vp2_feature_state::cpld_state.
 */
struct vp2_feature_event
{
	__le64 valid_mask;            //!< VP2_FEATURE_EVENT_* bits
	uint8_t status_report_enable; //!< Starts or stops the status reports, stays set across sessions
	uint8_t amp_gain; //!< Headphone amp attenuation, higher is quieter, ignored while amp_power_limit is on
	uint8_t brightness_refresh; //!< Value is unused, only refreshes @ref vp2_feature_state::brightness
	uint8_t amp_power_limit;    //!< 1 by default, the HTC driver's sound boost sends 0
	uint8_t unused_12;
	uint8_t audio_route_mode;    //!< 0, 1 or 2, 128 restores the previous mode
	uint8_t audio_profile_index; //!< Codec and amp register preset stored in the headset's flash
	uint8_t unused_15[25];
	uint8_t display_power;          //!< 0 off or 1 on, XORed with @ref VP2_DISPLAY_POWER_XOR
	uint8_t power_state;            //!< enum vp2_power_state, XORed with @ref VP2_POWER_STATE_XOR
	uint8_t proximity_override;     //!< Wireless only, 0 absent, 1 present, 0x80 keep alive, see the XOR define
	uint8_t wireless_auth_token[3]; //!< Wireless only, see @ref VP2_FEATURE_EVENT_WIRELESS_AUTH
	uint8_t unused_46[8];
	uint8_t thermal_watchdog; //!< @ref enum vp2_thermal_watchdog_flags, XORed with @ref VP2_THERMAL_WATCHDOG_XOR
	uint8_t unused_55;
};

static_assert(sizeof(struct vp2_feature_event) == 56, "bad struct size");

/*!
 * Read with a @ref VP2_REPORT_ID_FEATURE feature report, the MCU fills in the header itself. The versions are parsed
 * from hex strings, so a string of "0000000000203056" becomes 0x203056.
 */
struct vp2_feature_state
{
	uint8_t id;    //!< @ref VP2_REPORT_ID_FEATURE
	__le16 sub_id; //!< @ref VP2_COMMAND_FEATURE_STATE
	uint8_t size;  //!< @ref VP2_PAYLOAD_SIZE
	uint8_t unused_0[2];
	__le16 bridge_revision;    //!< Video bridge register 0x300A
	__le32 bootloader_version; //!< Same as "mfg-s-iapswver"
	__le32 mcu_version;        //!< Same as @ref VP2_COMMAND_GET_SOFTWARE_VERSION
	uint8_t cpld_version;      //!< 0 while a CPLD transfer is running
	uint8_t audio_version;     //!< 0 while an audio transfer is running
	uint8_t bridge_version;    //!< High nibble major, low nibble minor, 0 while a bridge transfer is running
	uint8_t unused_15[3];
	uint8_t wireless_mode;           //!< Always 0 in the firmware on my unit.
	uint8_t firmware_variant;        //!< Selects the transfer size limits
	__le16 proximity_calibration[8]; //!< Factory proximity sensor data, "mfg-r-ps_user_data <N>" reads one
	__le16 ipd_adc_min;              //!< Factory IPD ADC value at the narrowest position
	__le16 ipd_adc_max;              //!< Factory IPD ADC value at the widest position
	uint8_t unused_40[4];
	__le64 unknown_version; //!< Always 0x303070
	uint8_t amp_gain;       //!< 0xFF while the audio is powered off
	uint8_t brightness;     //!< 0 while the display is powered off
	uint8_t display_flags;  //!< @ref enum vp2_display_flags
	uint8_t cpld_state;     //!< 3 normally, derived from CPLD status bits
	uint8_t status_report_enabled;
	uint8_t amp_power_limit;
	uint8_t power_state; //!< @ref enum vp2_power_state, or 0/1 from CPLD state changes
	uint8_t hardware_id; //!< From the factory data
};

static_assert(sizeof(struct vp2_feature_state) == VP2_DATA_SIZE, "bad struct size");

//! Status report, sent every 50ms while @ref VP2_FEATURE_EVENT_STATUS_REPORT is enabled.
struct vp2_status_report
{
	uint8_t id;                  //!< @ref VP2_REPORT_ID_STATUS
	__le16 sub_id;               //!< @ref VP2_COMMAND_STATUS_REPORT
	uint8_t size;                //!< Size of the rest of the report
	__le32 ipd_adc;              //!< Raw IPD slider position, only the low 16 bits are used
	uint8_t system_button;       //!< 1 while pressed
	uint8_t audio_button;        //!< @ref enum vp2_audio_button
	uint8_t audio_route_mode;    //!< @ref vp2_feature_event::audio_route_mode
	uint8_t audio_profile_index; //!< @ref vp2_feature_event::audio_profile_index
	__le16 proximity_trend;      //!< @ref enum vp2_proximity_trend
	__le16 proximity;            //!< See @ref VP2_PROXIMITY_THRESHOLD
	__le16 ipd;                  //!< See @ref VP2_IPD_UNIT_METERS, clamped to 5700..7050
	uint8_t reserved[38];        //!< Always 0
	uint8_t earphone;            //!< 1 in the codec's earphone mode, always 0 on most boards
	__le32 thermal_adc;          //!< Only the low 16 bits are used, see @ref VP2_FEATURE_EVENT_THERMAL_WATCHDOG
	uint8_t wireless_auth;       //!< 1 while wireless authentication is active, always 0 on a cable
	__be16
	    nonce; //!< Low 16 bits of thermal_adc times the MCU uptime in ms, see @ref VP2_FEATURE_EVENT_WIRELESS_AUTH
};

static_assert(sizeof(struct vp2_status_report) == VP2_DATA_SIZE, "bad struct size");

/*!
 * Header of the config blob, the JSON follows. @ref VP2_COMMAND_CONFIG_READ offsets start at this header.
 */
struct vp2_config_header
{
	__le32 magic;      //!< @ref VP2_CONFIG_MAGIC
	__le32 total_size; //!< Header and JSON, the size @ref VP2_COMMAND_CONFIG_READ_START replies with
	__le32 json_size;
	uint8_t unused_12[52];
	char json_sha256[64]; //!< SHA-256 of the JSON as hex, not null terminated
};

static_assert(sizeof(struct vp2_config_header) == VP2_CONFIG_HEADER_SIZE, "bad struct size");

#pragma pack(pop)
