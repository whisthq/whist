/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * @file ltr.h
 * @brief API for long-term reference handling.
 */
#ifndef WHIST_VIDEO_LTR_H
#define WHIST_VIDEO_LTR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    /**
     * @brief  A normal frame.
     *
     * It requires the previous frame to have been decoded correctly to
     * decode it.
     */
    VIDEO_FRAME_TYPE_NORMAL,
    /**
     * @brief  An intra frame.
     *
     * It can be decoded without needing any previous data from the
     * stream, and will includes parameter sets if necessary.  If
     * long-term referencing is enabled, it will also be placed in the
     * first long-term reference slot and all others will be cleared.
     */
    VIDEO_FRAME_TYPE_INTRA,
    /**
     * @brief  A frame creating a long term reference.
     *
     * Like a normal frame it requires the previous frame to have been
     * decoded correctly to decode, but it is then also placed in a
     * long-term reference slot to possibly be used by future frames.
     */
    VIDEO_FRAME_TYPE_CREATE_LONG_TERM,
    /**
     * @brief  A frame using a long term reference.
     *
     * This requires a specific long term frame to have been previously
     * decoded correctly in order to decode it.
     */
    VIDEO_FRAME_TYPE_REFER_LONG_TERM,
} VideoFrameType;

/**
 * The long-term reference action to take when encoding the next frame.
 */
typedef struct LTRAction {
    /**
     * Type of frame to make.
     */
    VideoFrameType frame_type;
    /**
     * If the type needs a long-term reference slot, the index of the
     * slot to use.
     */
    int long_term_frame_index;
} LTRAction;

/**
 * Long-term reference state object.
 */
typedef struct LTRState LTRState;

/**
 * Create a new long-term reference state object.
 *
 * @return  Pointer to the object created, or null on failure.
 */
LTRState *ltr_create(void);

/**
 * Create a new long-term reference state object.
 *
 * @param ltr  Long-term reference state to destroy.
 */
void ltr_destroy(LTRState *ltr);

/**
 * Pick the action to use when encoding the next frame.
 *
 * @param ltr     Long-term reference state.
 * @param action  Action to use when encoding the next frame.
 * @return        0 on success, or -1 if state is broken somehow.
 */
int ltr_pick_next_action(LTRState *ltr, LTRAction *action);

/**
 * Mark the most-recently-encoded frame as sent and assign a unique
 * frame ID to it for ack/nack tracking.
 *
 * @param ltr       Long-term reference state.
 * @param frame_id  Frame ID given to the frame.
 * @return          0 on success, or -1 if there is no such frame.
 */
int ltr_mark_frame_sent(LTRState *ltr, uint32_t frame_id);

/**
 * Ack a frame by ID.
 *
 * Indicates that the far end has definitely received this frame and all
 * of its transitive dependencies.  If any of them are long-term
 * reference frames then they are now suitable for use when encoding.
 *
 * @param ltr       Long-term reference state.
 * @param frame_id  Frame ID of the frame to ack.
 * @return          0 on success, or -1 if the frame is unknown or if
 *                  the state is inconsistent (e.g. frame already nacked).
 */
int ltr_ack_frame(LTRState *ltr, uint32_t frame_id);

/**
 * Nack a frame by ID.
 *
 * Indicates that the far end has definitely not received this frame and
 * will not be able to decode anything transitively depending on it.
 *
 * @param ltr       Long-term reference state.
 * @param frame_id  Frame ID of the frame to nack.
 * @return          0 on success, or -1 if the frame is unknown or if
 *                  the state is inconsistent (e.g. frame already acked).
 */
int ltr_nack_frame(LTRState *ltr, uint32_t frame_id);

/**
 * Reset LTR state and ask to generate a new intra frame.
 *
 * If everything appears to be completely broken, start again.  Also
 * necessary when changing resolutions unless the codec supports
 * differently-sized reference frames.
 *
 * @param ltr Long-term reference state.
 */
void ltr_force_intra(LTRState *ltr);

#endif /* WHIST_VIDEO_LTR_H */
