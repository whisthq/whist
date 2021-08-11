/**
 * Copyright Fractal Computers, Inc. 2020
 * @file audio.c
 * @brief This file contains all code that interacts directly with processing
 *        audio packets on the client.
============================
Usage
============================

initAudio() must be called first before receiving any audio packets.
updateAudio() gets called immediately after to update the client to the server's
audio format.
*/

/*
============================
Includes
============================
*/

#include "audio.h"
#include "network.h"
#include "ringbuffer.h"

extern bool has_video_rendered_yet;

#define LOG_AUDIO false

// system audio queue + our buffer limits, in decompressed bytes
#define AUDIO_QUEUE_LOWER_LIMIT 18000
#define AUDIO_QUEUE_UPPER_LIMIT 59000
#define TARGET_AUDIO_QUEUE_LIMIT 28000

#define MAX_NUM_AUDIO_FRAMES 25
RingBuffer* audio_ring_buffer;

#define SDL_AUDIO_BUFFER_SIZE 1024

#define MAX_FREQ 128000  // in Hertz

// We only allow 1 nack in each update_audio call because we had too many false nacks in the past.
// Increase this as our nacking becomes more accurate.
#define MAX_NACKED 1

/*
============================
Custom Types
============================
*/

typedef struct AudioContext {
    SDL_AudioDeviceID dev;
    AudioDecoder* audio_decoder;
    int decoder_frequency;
} AudioContext;

// holds information related to decoding and rendering audio
static AudioContext volatile audio_context;
// holds the current audio frame to play
static volatile FrameData audio_render_context;
// true if and only if the audio packet in audio_render_context should be played
static bool volatile rendering_audio = false;

// sample rate of audio signal
static int volatile audio_frequency = -1;
// true if and only if we should connect to a new audio device when playing audio
static bool volatile audio_refresh = false;

// the last ID we've processed for rendering
static int last_played_id = -1;
// 2 channels, 1024 samples per channel, 32 bits (4 bytes) per sample makes 2 * 1024 * 4 = 8192
// bytes per frame!
static int decoded_bytes_per_frame = 8192;

// END AUDIO VARIABLES

// START AUDIO FUNCTIONS

/*
============================
Private Functions
============================
*/

void destroy_audio_device();
void reinit_audio_device();
void audio_nack(int id, int index);
void nack_missing_packets();
void catchup_audio();
void flush_next_audio_frame();
void update_render_context();
int get_next_audio_frame(uint8_t* data);

/*
============================
Private Function Implementations
============================
*/

void destroy_audio_device() {
    /*
        Destroys the audio device
    */
    if (audio_context.dev) {
        SDL_CloseAudioDevice(audio_context.dev);
        audio_context.dev = 0;
    }
    if (audio_context.audio_decoder) {
        destroy_audio_decoder(audio_context.audio_decoder);
        audio_context.audio_decoder = NULL;
    }
}

void reinit_audio_device() {
    /*
        Recreate an audio decoder with @global audio_frequency and use SDL
        to open a new audio device.
    */
    LOG_INFO("Reinitializing audio device");
    destroy_audio_device();

    // cast socket and SDL variables back to their data type for usage
    SDL_AudioSpec wanted_spec = {0}, audio_spec = {0};
    audio_context.decoder_frequency = audio_frequency;
    audio_context.audio_decoder = create_audio_decoder(audio_context.decoder_frequency);

    SDL_zero(wanted_spec);
    SDL_zero(audio_spec);
    wanted_spec.channels = 2;
    wanted_spec.freq = audio_context.decoder_frequency;
    wanted_spec.format = AUDIO_F32SYS;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;

    audio_context.dev =
        SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &audio_spec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
    if (wanted_spec.freq != audio_spec.freq) {
        LOG_WARNING("Got Frequency %d, But Wanted Frequency %d...", audio_spec.freq,
                    wanted_spec.freq);
    } else {
        LOG_INFO("Using Audio Freqency: %d", audio_spec.freq);
    }
    if (audio_context.dev == 0) {
        LOG_ERROR("Failed to open audio: %s", SDL_GetError());
    } else {
        SDL_PauseAudioDevice(audio_context.dev, 0);
    }
}

void sync_audio_device() {
    /*
      Ensure that the server's and client's audio frequencies are the same, and reinitializes the
      audio frequency if not.
     */
    if (audio_context.decoder_frequency != audio_frequency) {
        LOG_INFO("Updating audio frequency to %d!", audio_frequency);
        audio_refresh = true;
    }
    // Note that because sdl_event_handler.c also sets audio_refresh to true when changing audio
    // devices, there is a minor race condition that can lead to reinit_audio_device() being called
    // more times than expected.
    if (audio_refresh) {
        audio_refresh = false;
        reinit_audio_device();
    }
}

void catchup_audio() {
    /*
      Catch up to the most recently received ID if no audio has played yet and clean out the ring
      buffer. The logic inside the if statement should only run before video has started rendering.
     */

    // if nothing has played yet, or if we've fallen far behind (because of a disconnect)
    if ((last_played_id == -1 && has_video_rendered_yet && audio_ring_buffer->max_id > 0) ||
        (last_played_id != -1 &&
         audio_ring_buffer->max_id - last_played_id > MAX_NUM_AUDIO_FRAMES)) {
        last_played_id = audio_ring_buffer->max_id - 1;
    }
    for (int i = 0; i < MAX_NUM_AUDIO_FRAMES; i++) {
        FrameData* frame_data = &audio_ring_buffer->receiving_frames[i];
        if (frame_data->id <= last_played_id) {
            reset_frame(frame_data);
        }
    }
}

bool is_next_audio_frame_valid() {
    /*
      Check if the next frame (i.e. the next MAX_NUM_AUDIO_INDICES packets) is valid - its ring
      buffer index is properly aligned, and the packets have the correct ID.

      Returns:
          (bool): true if the next frame is valid, else false
     */
    // prepare to play the next frame in the buffer
    int next_to_play_id = last_played_id + 1;

    FrameData* frame_data = get_frame_at_id(audio_ring_buffer, next_to_play_id);
    return frame_data->id == next_to_play_id &&
           frame_data->num_packets == frame_data->packets_received;
}

bool buffer_audio(int audio_device_queue) {
    /*
      Waits for audio to accumulate in the audio queue if the audio queue is too low, and returns
      whether or not we are still buffering audio. Ideally, we want about 30ms of audio in the
      buffer.

      Arguments:
          audio_device_queue (int): Size in bytes of the current audio device queue

      Returns:
          (bool) true if we are still buffering audio, else false.
     */

    static bool buffering_audio = false;
    int bytes_until_no_more_audio =
        (audio_ring_buffer->max_id - last_played_id) * decoded_bytes_per_frame + audio_device_queue;

    // If the audio queue is under AUDIO_QUEUE_LOWER_LIMIT, we need to accumulate more in the buffer
    if (!buffering_audio && bytes_until_no_more_audio < AUDIO_QUEUE_LOWER_LIMIT) {
        LOG_INFO("Audio Queue too low: %d. Needs to catch up!", bytes_until_no_more_audio);
        buffering_audio = true;
    }

    // don't play anything until we have enough audio in the queue
    if (buffering_audio) {
        if (bytes_until_no_more_audio >= TARGET_AUDIO_QUEUE_LIMIT) {
            LOG_INFO("Done catching up! Audio Queue: %d", bytes_until_no_more_audio);
            buffering_audio = false;
        }
    }
    return buffering_audio;
}

void flush_next_audio_frame() {
    /*
      Skip the next audio frame in the ring buffer.
     */
    last_played_id++;
    reset_frame(get_frame_at_id(audio_ring_buffer, last_played_id));
}

bool flush_audio(int audio_device_queue) {
    /*
      Flush the audio buffer if the device queue is too full, and return whether or not we are
      flushing audio.

      Arguments:
          audio_device_queue (int): Size in bytes of the current audio queue

      Returns:
          (bool): true if we are still flushing audio, else false.
     */
    // If an audio flush is triggered,
    // We skip audio until the buffer runs down to TARGET_AUDIO_QUEUE_LIMIT
    // Otherwise, we trigger an audio flush when the audio queue surpasses
    // AUDIO_QUEUE_UPPER_LIMIT
    static bool audio_flush_triggered = false;
    int next_to_play_id = last_played_id + 1;
    int real_limit = audio_flush_triggered ? TARGET_AUDIO_QUEUE_LIMIT : AUDIO_QUEUE_UPPER_LIMIT;

    if (audio_device_queue > real_limit) {
        LOG_WARNING("Audio queue full, skipping ID %d (Queued: %d)", next_to_play_id,
                    audio_device_queue);
        flush_next_audio_frame();
        if (!audio_flush_triggered) {
            audio_flush_triggered = true;
        }
    } else {
        audio_flush_triggered = false;
    }
    return audio_flush_triggered;
}

void update_render_context() {
    /*
      Store the next frame to render into the audio render context.
    */
    // we always encode our audio now
    int next_to_play_id = last_played_id + 1;
    FrameData* frame_data = get_frame_at_id(audio_ring_buffer, next_to_play_id);
#if LOG_AUDIO
    LOG_INFO("Moving audio frame ID %d into render context", frame_data->id);
#endif
    audio_render_context = *frame_data;
    // Tell the ring buffer we're rendering this frame
    set_rendering(audio_ring_buffer, frame_data->id);
    // increment to indicate that we've processed the next frame
    last_played_id++;
}

bool is_valid_audio_frequency() {
    /*
      Check if the audio frequency is between 0 and MAX_FREQ.

      Returns:
          (bool): true if the frequency is valid, else false.
     */
    if (audio_frequency > MAX_FREQ) {
        LOG_ERROR("Frequency received was too large: %d, silencing audio now.", audio_frequency);
        return false;
    } else if (audio_frequency < 0) {
        // No audio frequency received yet
        return false;
    } else {
        return true;
    }
}

int send_next_frame_to_decoder() {
    AudioFrame* frame = (AudioFrame*)audio_render_context.frame_buffer;
    if (frame == NULL) {
        LOG_FATAL("Fatal Error! A NULL frame was pulled from the render context!");
    }
    if (audio_decoder_send_packets(audio_context.audio_decoder, frame->data, frame->data_length) <
        0) {
        LOG_WARNING("Failed to send packets to decoder!");
        return -1;
    }
    return 0;
}

int get_next_audio_frame(uint8_t* data) {
    /*
      Get the next (encoded) audio frame from the render context and decode it into the data buffer

      Arguments:
          decoded_data (uint8_t*): Data buffer to receive the decoded audio data
    */
    // TODO: frame global variable, like in video? or just take the ring buffer entry
    // TODO: call audio_decoder_send_packets(audio_context.audio_decoder, ring buffer entry->data,
    // ring buffer entry->size); setup the frame
    // Decode encoded audio
    int res = audio_decoder_get_frame(audio_context.audio_decoder);
    if (res == 0) {
        // Get decoded data
        audio_decoder_packet_readout(audio_context.audio_decoder, data);
    }
    return res;
}

// END AUDIO FUNCTIONS

/*
============================
Public Function Implementations
============================
*/

void init_audio() {
    /*
        Initialize the audio device
    */
    LOG_INFO("Initializing audio system");
    audio_ring_buffer = init_ring_buffer(FRAME_AUDIO, MAX_NUM_AUDIO_FRAMES);

    // Set audio to be reinit'ed
    audio_refresh = true;
    rendering_audio = false;
}

void destroy_audio() {
    LOG_INFO("Destroying audio system");
    // Ensure is thread-safe against arbitrary calls to render_audio
    while (rendering_audio) {
        SDL_Delay(5);
    }
    destroy_audio_device();
}

void enable_audio_refresh() { audio_refresh = true; }

void set_audio_frequency(int new_audio_frequency) { audio_frequency = new_audio_frequency; }

void render_audio() {
    /*
        Actually renders audio frames. Called in multithreaded_renderer. update_audio should
        configure @global audio_render_context to contain the latest audio packet to render.
        This function simply decodes and renders it.
    */
    if (rendering_audio) {
        if (!is_valid_audio_frequency()) {
            // if the audio frequency is bad, stop rendering
            rendering_audio = false;
            return;
        }

        sync_audio_device();

        int res = send_next_frame_to_decoder();

        // this buffer will always hold the decoded data
        static uint8_t decoded_data[MAX_AUDIO_FRAME_SIZE];
        // decode the frame into the buffer
        while ((res = get_next_audio_frame(decoded_data)) == 0) {
            res = SDL_QueueAudio(audio_context.dev, decoded_data,
                                 audio_decoder_get_frame_data_size(audio_context.audio_decoder));

            if (res < 0) {
                LOG_WARNING("Could not play audio!");
            }
        }
        // No longer rendering audio
        rendering_audio = false;
    }
}

void update_audio() {
    /*
      Handle non-rendering aspects of audio: managing the size of the audio queue, nacking for
      missing frames, and configuring the @global audio_render_context to play an audio packet.
      render_audio will actually play this packet.
    */

    if (rendering_audio) {
        // If we're currently rendering an audio packet, don't update audio - the audio_context
        // struct is being used, so a race condition will occur if we call SDL_GetQueuedAudioSize at
        // the same time.
        return;
    }

    int audio_device_queue = 0;
    if (audio_context.dev) {
        // If we have a device, get the queue size
        audio_device_queue = (int)SDL_GetQueuedAudioSize(audio_context.dev);
    }
    // Otherwise, the queue size is 0

#if LOG_AUDIO
    LOG_DEBUG("Queue: %d", audio_device_queue);
#endif

    catchup_audio();

    // Return if there's nothing to play or if last_played_id > max_id (eg ring buffer reset)
    if (last_played_id == -1 || last_played_id > audio_ring_buffer->max_id) {
        return;
    }

    // Return if we are buffering audio to build up the queue.
    if (buffer_audio(audio_device_queue)) {
        return;
    }

    if (is_next_audio_frame_valid()) {
        if (!flush_audio(audio_device_queue)) {
            // move the next frame into the render context
            update_render_context();
            // tell renderer thread to render the audio
            rendering_audio = true;
        }
    } else {
        return;
    }
}

// NOTE that this function is in the hotpath.
// The hotpath *must* return in under ~10000 assembly instructions.
// Please pass this comment into any non-trivial function that this function calls.
int32_t receive_audio(FractalPacket* packet) {
    /*
        This function will store an audio packet in its internal buffer, to be played on later
        calls to update_audio. A buffer is needed so that the audio comes out smoothly. We delay the
        audio by about 30ms to ensure that the buffer is never empty while we call update_audio. If
        the buffer is empty, the speakers will make a "pop" noise.

        Arguments:
            packet (FractalPacket*): FractalPacket wrapped audio data.

        Return:
            ret (int): 0 on success, -1 on failure
    */
    // also don't handle packets if the frequency is too high
    if (audio_frequency == MAX_FREQ) {
        return 0;
    }
    int res = receive_packet(audio_ring_buffer, packet);
    if (res < 0) {
        return res;
    } else if (res > 0) {
        // we overwrote the last frame
        if (last_played_id < packet->id && last_played_id > 0) {
            last_played_id = packet->id - 1;
            reset_frame(get_frame_at_id(audio_ring_buffer, last_played_id));
            LOG_INFO("Last played ID now %d", last_played_id);
        }
    }
    return 0;
}
