/**
 * @copyright Copyright (c) 2022 Whist Technologies, Inc.
 * @file ltr.h
 * @brief API for long-term reference handling.
 */
#ifndef WHIST_VIDEO_LTR_H
#define WHIST_VIDEO_LTR_H

#include <stdbool.h>
#include <stdint.h>

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
 * This also supplies the opaque frame_id which will be use to match
 * feedback to the frames which were sent.
 *
 * @param ltr       Long-term reference state.
 * @param action    Action to use when encoding the next frame.
 * @param frame_id  Frame ID which will be given to the frame.
 * @return          0 on success, or -1 if state is broken somehow.
 */
int ltr_get_next_action(LTRState *ltr, LTRAction *action, uint32_t frame_id);

/**
 * Mark a frame as received.
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
int ltr_mark_frame_received(LTRState *ltr, uint32_t frame_id);

/**
 * Mark a frame as not received.
 *
 * Indicates that the far end has definitely not received this frame and
 * will not be able to decode anything transitively depending on it.
 *
 * @param ltr       Long-term reference state.
 * @param frame_id  Frame ID of the frame to nack.
 * @return          0 on success, or -1 if the frame is unknown or if
 *                  the state is inconsistent (e.g. frame already acked).
 */
int ltr_mark_frame_not_received(LTRState *ltr, uint32_t frame_id);

/**
 * Mark stream as requiring recovery.
 *
 * This is equivalent to marking the most-recently-sent frame as not
 * received.
 *
 * @param ltr  Long-term reference state.
 */
void ltr_mark_stream_broken(LTRState *ltr);

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
