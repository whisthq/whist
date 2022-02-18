/**
 * @copyright Copyright 2022 Whist Technologies, Inc.
 * @file ltr.c
 * @brief Codec-independent decision logic for long-term references.
 */
#include "whist/core/whist.h"
#include "whist/utils/linked_list.h"

#include "ltr.h"

enum {
    // The number of frames to wait before generating a new intra
    // frame if we don't have any useful response from the far end.
    LTR_NEW_INTRA_FRAME_DELAY = 60,
    // The maximum number of frames to keep in the inflight list.
    // When there are more than this, the excess are assumed to be no
    // longer interesting and moved to the done list.  (This shouldn't
    // happen unless the RTT is unreasonably high.)
    LTR_MAX_INFLIGHT_FRAMES = 60,
    // The maximum number of frames to keep around in the done list.
    // They are not necessary for things to work, but are useful for
    // determining what happened when something goes wrong.
    LTR_MAX_DONE_FRAMES = 60,
};

typedef struct {
    LINKED_LIST_HEADER;

    // Long-term reference action taken when encoding this frame.
    LTRAction action;

    // frame counter for this frame.  Must be positive.
    uint64_t frame_counter;

    // Frame counter for the frame this depends on (so, was encoded with
    // reference to).  If zero the frame has no reference, so is an
    // intra frame.
    uint64_t reference_frame_counter;

    // Frame ID assigned by the caller for tracking acks/nacks.
    uint32_t frame_id;

    // Whether the frame has been sent.
    bool sent;
    // Whether the frame has been acked.
    bool ack;
    // Whether the frame has been nacked.
    bool nack;
} LTRFrame;

struct LTRState {
    // Pending frames, which have had their types set but have not yet
    // been returned from the encoder.
    LinkedList pending_list;
    // In-flight frames, which have been sent but no ack or nack has yet
    // been returned from the client.
    LinkedList inflight_list;
    // Done frames, which have already been acked or nacked but are still
    // useful to keep around for tracking purposes.
    LinkedList done_list;

    // Counter of the current frame.  This is the usual reference frame
    // when a new frame is created, but if it is ever nacked (typically
    // because an older frame is depends on is nacked) then we will need
    // to use an older frame as reference to fix the stream.
    uint64_t frame_counter;

    // Set if an intra frame has been requested explictly.
    bool intra_frame_needed;
    // Set if the last frame sent (i.e. the one at frame_counter) has
    // been nacked, so we need to recover.
    bool last_frame_is_bad;
    // Set if a reference to a long-term frame is outstanding.  We won't
    // try anything else to fix the stream until this has a response.
    bool refer_long_term_outstanding;
    // Set if the creation of a new long-term frame is outstanding.  We
    // won't try to make a new one until this is complete.
    bool create_long_term_outstanding;

    // Set if we have a known-good long-term frame.  This should only
    // be unset at the beginning of a stream, thereafter we always try
    // to maintain a known-good long-term frame.
    bool have_good_long_term_frame;
    // The slot index of the current known-good frame.
    int good_long_term_frame_index;

    // Counter of the most recent intra frame.  Frames older than this
    // should not affect the current stream state.
    uint64_t intra_frame_counter;
    // If refer_long_term_outstanding is set, the counter of the frame
    // containing the reference.
    uint64_t refer_long_term_frame_counter;
    // If create_long_term_outstanding is set, the counter of the frame
    // intended to become a new long-term frame.
    uint64_t create_long_term_frame_counter;
    // If have_good_long_term_frame is set, the counter of the frame
    // which is known-good.
    uint64_t good_long_term_frame_counter;
};

static void ltr_dump_frame(LTRFrame *frame) {
    LOG_DEBUG("    { action { %d, %d }  fc %" PRIu64 "  ref %" PRIu64 "  id %" PRIu32
              " sent %d ack %d nack %d }",
              frame->action.frame_type, frame->action.long_term_frame_index, frame->frame_counter,
              frame->reference_frame_counter, frame->frame_id, frame->sent, frame->ack,
              frame->nack);
}

static void ltr_dump_state(LTRState *ltr) {
    // State dump for debug purposes.

    LOG_DEBUG("LTR State:");
    LOG_DEBUG("  frame_counter %" PRIu64 "  intra_frame_needed %d  last_frame_is_bad %d.",
              ltr->frame_counter, ltr->intra_frame_needed, ltr->last_frame_is_bad);
    LOG_DEBUG("  good_long_term %d (%d, %" PRIu64 ")  intra_frame %" PRIu64 ".",
              ltr->have_good_long_term_frame, ltr->good_long_term_frame_index,
              ltr->good_long_term_frame_counter, ltr->intra_frame_counter);
    LOG_DEBUG("  refer_long_term %d (%" PRIu64 ")  create_long_term %d (%" PRIu64 ").",
              ltr->refer_long_term_outstanding, ltr->refer_long_term_frame_counter,
              ltr->create_long_term_outstanding, ltr->create_long_term_frame_counter);

    // Really verbose - state of all recent frames.
    if (0) {
        LOG_DEBUG("  Pending list:");
        linked_list_for_each(&ltr->pending_list, LTRFrame, frame) ltr_dump_frame(frame);
        LOG_DEBUG("  Inflight list:");
        linked_list_for_each(&ltr->inflight_list, LTRFrame, frame) ltr_dump_frame(frame);
        LOG_DEBUG("  Done list:");
        linked_list_for_each(&ltr->done_list, LTRFrame, frame) ltr_dump_frame(frame);
    }
}

LTRState *ltr_create(void) {
    LTRState *ltr = safe_malloc(sizeof(*ltr));
    memset(ltr, 0, sizeof(*ltr));

    return ltr;
}

static void ltr_clear_list(LinkedList *list) {
    linked_list_for_each(list, LTRFrame, iter) {
        linked_list_remove(list, iter);
        free(iter);
    }
}

void ltr_destroy(LTRState *ltr) {
    ltr_clear_list(&ltr->pending_list);
    ltr_clear_list(&ltr->inflight_list);
    ltr_clear_list(&ltr->done_list);
    free(ltr);
}

static void ltr_clear_old_frames(LTRState *ltr) {
    // When lists get too long, assume nothing interesting is going to
    // happen with the excess.
    LTRFrame *frame;
    while (linked_list_size(&ltr->inflight_list) > LTR_MAX_INFLIGHT_FRAMES) {
        frame = linked_list_extract_head(&ltr->inflight_list);
        linked_list_add_tail(&ltr->done_list, frame);
    }
    while (linked_list_size(&ltr->done_list) > LTR_MAX_DONE_FRAMES) {
        frame = linked_list_extract_head(&ltr->done_list);
        free(frame);
    }
}

static int ltr_pick_next_action(LTRState *ltr, LTRAction *action) {
    if (LOG_LONG_TERM_REFERENCE_FRAMES) {
        ltr_dump_state(ltr);
    }

    ltr_clear_old_frames(ltr);

    LTRFrame *frame = safe_malloc(sizeof(*frame));
    memset(frame, 0, sizeof(*frame));

    if (  // Need an intra frame at the start of the stream.
        ltr->frame_counter == 0 ||
        // An intra frame was forced (e.g. on resolution change).
        ltr->intra_frame_needed ||
        // Our previous intra frame hasn't got through, so try another.
        (!ltr->have_good_long_term_frame &&
         ltr->frame_counter + 1 - ltr->intra_frame_counter >= LTR_NEW_INTRA_FRAME_DELAY) ||
        // Stream is broken, and we don't have a good long-term frame to fix it.
        (ltr->last_frame_is_bad && !ltr->have_good_long_term_frame)) {
        frame->action = (LTRAction){.frame_type = VIDEO_FRAME_TYPE_INTRA};
        ltr->intra_frame_needed = false;
        ltr->last_frame_is_bad = false;

    } else if (ltr->last_frame_is_bad) {
        // Fix the stream by referring to the good long-term frame.
        frame->action = (LTRAction){
            .frame_type = VIDEO_FRAME_TYPE_REFER_LONG_TERM,
            .long_term_frame_index = ltr->good_long_term_frame_index,
        };
        ltr->last_frame_is_bad = false;
        frame->reference_frame_counter = ltr->good_long_term_frame_counter;

    } else if (ltr->have_good_long_term_frame && !ltr->refer_long_term_outstanding &&
               !ltr->create_long_term_outstanding) {
        // If nothing else is happening, try to make a new long-term frame.
        frame->action = (LTRAction){
            .frame_type = VIDEO_FRAME_TYPE_CREATE_LONG_TERM,
            .long_term_frame_index = !ltr->good_long_term_frame_index,
        };
        frame->reference_frame_counter = ltr->frame_counter;

    } else {
        // Nothing special to do, make a normal frame.
        frame->action = (LTRAction){.frame_type = VIDEO_FRAME_TYPE_NORMAL};
        frame->reference_frame_counter = ltr->frame_counter;
    }

    frame->frame_counter = ++ltr->frame_counter;
    linked_list_add_tail(&ltr->pending_list, frame);

    if (frame->action.frame_type == VIDEO_FRAME_TYPE_INTRA) {
        // If this is an intra frame then we don't want frames before it
        // to affect the stream after (though we do still want to track
        // them).
        ltr->intra_frame_counter = frame->frame_counter;
        ltr->refer_long_term_outstanding = false;
        ltr->create_long_term_outstanding = false;
        ltr->have_good_long_term_frame = false;

    } else if (frame->action.frame_type == VIDEO_FRAME_TYPE_REFER_LONG_TERM) {
        // If we are referring to a long-term frame then any previous
        // creation of a long-term frame is now useless.
        ltr->refer_long_term_frame_counter = frame->frame_counter;
        ltr->refer_long_term_outstanding = true;
        ltr->create_long_term_outstanding = false;

    } else if (frame->action.frame_type == VIDEO_FRAME_TYPE_CREATE_LONG_TERM) {
        ltr->create_long_term_frame_counter = frame->frame_counter;
        ltr->create_long_term_outstanding = true;
    }

    *action = frame->action;
    return 0;
}

static int ltr_mark_frame_sent(LTRState *ltr, uint32_t frame_id) {
    LTRFrame *frame = linked_list_extract_head(&ltr->pending_list);
    if (!frame) {
        // There are no pending frames, something has gone wrong.
        return -1;
    }

    frame->frame_id = frame_id;
    frame->sent = true;

    linked_list_add_tail(&ltr->inflight_list, frame);
    return 0;
}

int ltr_get_next_action(LTRState *ltr, LTRAction *action, uint32_t frame_id) {
    ltr_pick_next_action(ltr, action);
    return ltr_mark_frame_sent(ltr, frame_id);
}

static void ltr_ack_frame_internal(LTRState *ltr, LTRFrame *frame) {
    // Recursively ack frame, and all its references.

    // If this frame was previously nacked then something has gone wrong.
    FATAL_ASSERT(!frame->nack);

    // If we've already acked this frame then there is nothing to do.
    if (frame->ack) return;

    if (LOG_LONG_TERM_REFERENCE_FRAMES) {
        LOG_INFO("ACK for frame %d (action { %d, %d }, fc %" PRIu64 " ref %" PRIu64 ").",
                 frame->frame_id, frame->action.frame_type, frame->action.long_term_frame_index,
                 frame->frame_counter, frame->reference_frame_counter);
    }
    frame->ack = true;

    if (frame->frame_counter >= ltr->intra_frame_counter) {
        // If this was the most recent reference to a long-term frame then
        // we are now in a good state and can start creating new long-term
        // reference frames again.
        if (frame->action.frame_type == VIDEO_FRAME_TYPE_REFER_LONG_TERM) {
            if (frame->frame_counter == ltr->refer_long_term_frame_counter) {
                ltr->refer_long_term_outstanding = false;
            } else {
                // This is an old frame which is no longer useful because
                // some following frames are already known to be bad.
            }
        }

        // If this frame creates a long-term reference then have a new good
        // long term reference to use.
        if (frame->action.frame_type == VIDEO_FRAME_TYPE_INTRA) {
            ltr->have_good_long_term_frame = true;
            ltr->good_long_term_frame_index = frame->action.long_term_frame_index;
            ltr->good_long_term_frame_counter = frame->frame_counter;
        } else if (frame->action.frame_type == VIDEO_FRAME_TYPE_CREATE_LONG_TERM) {
            if (frame->frame_counter == ltr->create_long_term_frame_counter) {
                ltr->have_good_long_term_frame = true;
                ltr->good_long_term_frame_index = frame->action.long_term_frame_index;
                ltr->good_long_term_frame_counter = frame->frame_counter;
                ltr->create_long_term_outstanding = false;
            } else {
                // This did create a long-term frame at the far end, but
                // it is no longer useful because some later frames were
                // bad and we have already recovered using a previous
                // long-term frame.
            }
        }
    }

    // Reference frame must be earlier in the list.
    LTRFrame *ref = linked_list_prev(frame);
    while (ref) {
        if (ref->frame_counter == frame->reference_frame_counter) {
            ltr_ack_frame_internal(ltr, ref);
            break;
        }
        ref = linked_list_prev(ref);
    }

    // Move frame to done list after frames which it refers to.
    linked_list_remove(&ltr->inflight_list, frame);
    linked_list_add_tail(&ltr->done_list, frame);
}

static void ltr_nack_frame_internal(LTRState *ltr, LTRFrame *frame, bool pending) {
    // Recursively nack frame, and all its dependents.

    // If this frame was previously acked then something has gone wrong.
    FATAL_ASSERT(!frame->ack);

    // If we've already nacked this frame then there is nothing to do.
    if (frame->nack) return;

    if (LOG_LONG_TERM_REFERENCE_FRAMES) {
        LOG_INFO("NACK for frame %d (action { %d, %d }, fc %" PRIu64 " ref %" PRIu64 ").",
                 frame->frame_id, frame->action.frame_type, frame->action.long_term_frame_index,
                 frame->frame_counter, frame->reference_frame_counter);
    }
    frame->nack = true;

    // If this is the current frame then the stream is now broken.
    if (ltr->frame_counter == frame->frame_counter) {
        ltr->last_frame_is_bad = true;
    }

    if (frame->frame_counter >= ltr->intra_frame_counter) {
        // If this was a reference to a long-term frame then that reference
        // did not work and we will need to make a new one.
        if (frame->action.frame_type == VIDEO_FRAME_TYPE_REFER_LONG_TERM) {
            ltr->refer_long_term_outstanding = false;
        }

        // If this was a creating a long-term frame then it has failed and
        // we will need to fix the stream and try again.
        if (frame->action.frame_type == VIDEO_FRAME_TYPE_CREATE_LONG_TERM) {
            ltr->create_long_term_outstanding = false;
        }
    }

    // Frames referring to this one must be later in the list.
    LTRFrame *next = linked_list_next(frame);

    // Move frame to done list before the frames depending on it.
    linked_list_remove(&ltr->inflight_list, frame);
    linked_list_add_tail(&ltr->done_list, frame);

    LTRFrame *ref = next;
    while (ref) {
        next = linked_list_next(ref);
        if (ref->reference_frame_counter == frame->frame_counter) {
            ltr_nack_frame_internal(ltr, ref, false);
        }
        ref = next;
    }

    if (!pending) {
        // The pending list might also contain frames which are now useless.
        linked_list_for_each(&ltr->pending_list, LTRFrame, iter) {
            if (iter->reference_frame_counter == frame->frame_counter) {
                ltr_nack_frame_internal(ltr, iter, true);
            }
        }
    }
}

static LTRFrame *ltr_find_frame_by_id(LTRState *ltr, uint32_t frame_id) {
    linked_list_for_each(&ltr->inflight_list, LTRFrame, iter) {
        if (iter->frame_id == frame_id) return iter;
    }
    return NULL;
}

int ltr_mark_frame_received(LTRState *ltr, uint32_t frame_id) {
    LTRFrame *frame = ltr_find_frame_by_id(ltr, frame_id);
    if (!frame) {
        // Unknown old frame.
        return -1;
    }
    ltr_ack_frame_internal(ltr, frame);
    return 0;
}

int ltr_mark_frame_not_received(LTRState *ltr, uint32_t frame_id) {
    LTRFrame *frame = ltr_find_frame_by_id(ltr, frame_id);
    if (!frame) {
        // Unknown old frame.
        return -1;
    }
    ltr_nack_frame_internal(ltr, frame, false);
    return 0;
}

void ltr_mark_stream_broken(LTRState *ltr) {
    // Find the most-recently-sent frame.
    LTRFrame *frame = linked_list_tail(&ltr->inflight_list);
    if (!frame) {
        // The stream is broken but the far end says it has received all
        // outstanding frames - something has gone wrong.  The current
        // state is inconsistent, but we can recover by starting again
        // with a new intra frame.
        ltr->intra_frame_needed = true;
        LOG_WARNING(
            "Stream marked as broken but all outstanding frames have "
            "been received - starting again with a new intra frame.");
        ltr_dump_state(ltr);
        return;
    }
    ltr_nack_frame_internal(ltr, frame, false);
}

void ltr_force_intra(LTRState *ltr) { ltr->intra_frame_needed = true; }
