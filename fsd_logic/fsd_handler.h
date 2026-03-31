#pragma once

#include "../libraries/mcp_can_2515.h"
#include <stdbool.h>
#include <stdint.h>

#define CAN_ID_GTW_CAR_CONFIG 0x398  // 920 - HW version detection
#define CAN_ID_FOLLOW_DIST    0x3F8  // 1016 - follow distance / speed profile
#define CAN_ID_AP_CONTROL     0x3FD  // 1021 - autopilot control (FSD target)

typedef enum {
    TeslaHW_Unknown = 0,
    TeslaHW_HW3,
    TeslaHW_HW4,
} TeslaHWVersion;

typedef struct {
    TeslaHWVersion hw_version;
    int speed_profile;     // HW3: 0-2, HW4: 0-4
    int speed_offset;      // HW3 only
    bool fsd_enabled;      // currently modifying frames
    bool nag_suppressed;   // bit19 cleared
    uint32_t frames_modified;
} FSDState;

/** Initialize FSD state with defaults (speed_profile = fastest) */
void fsd_state_init(FSDState* state, TeslaHWVersion hw);

/** Set a single bit in a CAN frame buffer */
void fsd_set_bit(CANFRAME* frame, int bit, bool value);

/** Read mux ID from frame (data[0] & 0x07) */
uint8_t fsd_read_mux_id(const CANFRAME* frame);

/** Check if FSD is selected in vehicle UI (data[4] bit6) */
bool fsd_is_selected_in_ui(const CANFRAME* frame);

/** Detect HW version from GTW_carConfig frame (CAN ID 0x398).
 *  Returns TeslaHW_Unknown if not determinable. */
TeslaHWVersion fsd_detect_hw_version(const CANFRAME* frame);

/** Handle CAN ID 0x3F8 - update speed profile from follow distance */
void fsd_handle_follow_distance(FSDState* state, const CANFRAME* frame);

/** Handle CAN ID 0x3FD - modify autopilot frame for FSD unlock.
 *  Returns true if the frame was modified and should be re-sent. */
bool fsd_handle_autopilot_frame(FSDState* state, CANFRAME* frame);
