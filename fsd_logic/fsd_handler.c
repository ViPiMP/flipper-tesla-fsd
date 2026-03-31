#include "fsd_handler.h"
#include <string.h>

void fsd_state_init(FSDState* state, TeslaHWVersion hw) {
    memset(state, 0, sizeof(FSDState));
    state->hw_version = hw;
    // Default to fastest speed profile
    state->speed_profile = (hw == TeslaHW_HW4) ? 4 : 2;
}

void fsd_set_bit(CANFRAME* frame, int bit, bool value) {
    int byte_idx = bit / 8;
    int bit_idx = bit % 8;
    uint8_t mask = (uint8_t)(1U << bit_idx);
    if(value) {
        frame->buffer[byte_idx] |= mask;
    } else {
        frame->buffer[byte_idx] &= (uint8_t)(~mask);
    }
}

uint8_t fsd_read_mux_id(const CANFRAME* frame) {
    return frame->buffer[0] & 0x07;
}

bool fsd_is_selected_in_ui(const CANFRAME* frame) {
    return (frame->buffer[4] >> 6) & 0x01;
}

TeslaHWVersion fsd_detect_hw_version(const CANFRAME* frame) {
    if(frame->canId != CAN_ID_GTW_CAR_CONFIG) return TeslaHW_Unknown;
    // GTW_dasHw: byte0 bits 6-7 (2 bits, big-endian notation bit7|2@0+)
    uint8_t das_hw = (frame->buffer[0] >> 6) & 0x03;
    switch(das_hw) {
    case 2:
        return TeslaHW_HW3;
    case 3:
        return TeslaHW_HW4;
    default:
        return TeslaHW_Unknown;
    }
}

void fsd_handle_follow_distance(FSDState* state, const CANFRAME* frame) {
    uint8_t fd = (frame->buffer[5] & 0xE0) >> 5;

    if(state->hw_version == TeslaHW_HW3) {
        switch(fd) {
        case 1: state->speed_profile = 2; break;
        case 2: state->speed_profile = 1; break;
        case 3: state->speed_profile = 0; break;
        default: break;
        }
    } else {
        switch(fd) {
        case 1: state->speed_profile = 3; break;
        case 2: state->speed_profile = 2; break;
        case 3: state->speed_profile = 1; break;
        case 4: state->speed_profile = 0; break;
        case 5: state->speed_profile = 4; break;
        default: break;
        }
    }
}

bool fsd_handle_autopilot_frame(FSDState* state, CANFRAME* frame) {
    uint8_t mux = fsd_read_mux_id(frame);
    bool fsd_ui = fsd_is_selected_in_ui(frame);
    bool modified = false;

    if(state->hw_version == TeslaHW_HW3) {
        // --- HW3 ---
        if(mux == 0 && fsd_ui) {
            int raw = (int)((frame->buffer[3] >> 1) & 0x3F) - 30;
            int offset = raw * 5;
            if(offset < 0) offset = 0;
            if(offset > 100) offset = 100;
            state->speed_offset = offset;

            fsd_set_bit(frame, 46, true);
            // Write speed profile (V12/V13 format): byte6 bits 1-2
            frame->buffer[6] &= ~0x06;
            frame->buffer[6] |= (uint8_t)((state->speed_profile & 0x03) << 1);
            state->fsd_enabled = true;
            modified = true;
        }
        if(mux == 1) {
            fsd_set_bit(frame, 19, false); // nag suppression
            state->nag_suppressed = true;
            modified = true;
        }
        if(mux == 2 && fsd_ui) {
            frame->buffer[0] &= ~0xC0;
            frame->buffer[1] &= ~0x3F;
            frame->buffer[0] |= (uint8_t)((state->speed_offset & 0x03) << 6);
            frame->buffer[1] |= (uint8_t)(state->speed_offset >> 2);
            modified = true;
        }
    } else {
        // --- HW4 (FSD V14) ---
        if(mux == 0 && fsd_ui) {
            fsd_set_bit(frame, 46, true);
            fsd_set_bit(frame, 60, true);
            state->fsd_enabled = true;
            modified = true;
        }
        if(mux == 1) {
            fsd_set_bit(frame, 19, false); // nag suppression
            fsd_set_bit(frame, 47, true);
            state->nag_suppressed = true;
            modified = true;
        }
        if(mux == 2) {
            frame->buffer[7] &= ~(0x07 << 4);
            frame->buffer[7] |= (uint8_t)((state->speed_profile & 0x07) << 4);
            modified = true;
        }
    }

    if(modified) state->frames_modified++;
    return modified;
}
