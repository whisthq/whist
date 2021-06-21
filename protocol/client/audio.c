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

extern bool has_video_rendered_yet;

// Hold information about audio data as the packets come in
typedef struct AudioPacket {
    int id;
    int size;
    int nacked_for;
    int nacked_amount;
    char data[MAX_PAYLOAD_SIZE];
} AudioPacket;

#define LOG_AUDIO false

// system audio queue + our buffer limits, in decompressed bytes
#define AUDIO_QUEUE_LOWER_LIMIT 18000
#define AUDIO_QUEUE_UPPER_LIMIT 59000
#define TARGET_AUDIO_QUEUE_LIMIT 28000

#define MAX_NUM_AUDIO_FRAMES 25
// this is the maximum number of packets an encoded audio frame can be
// split into. it has been observed to be a good number given our bitrate
#define MAX_NUM_AUDIO_INDICES 3
#define RECV_AUDIO_BUFFER_SIZE (MAX_NUM_AUDIO_FRAMES * MAX_NUM_AUDIO_INDICES)
AudioPacket receiving_audio[RECV_AUDIO_BUFFER_SIZE];

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

// Audio Rendering
typedef struct RenderContext {
    // Whether or not the audio is encoded
    bool encoded;
    // Raw audio packets
    AudioPacket audio_packets[MAX_NUM_AUDIO_INDICES];
} RenderContext;

// holds information related to decoding and rendering audio
static AudioContext volatile audio_context;
// holds the current audio frame to play
static RenderContext volatile audio_render_context;
// true if and only if the audio packet in audio_render_context should be played
static bool volatile rendering_audio = false;

// sample rate of audio signal
static int volatile audio_frequency = -1;
// true if and only if we should connect to a new audio device when playing audio
static bool volatile audio_refresh = false;

// when the last nack was sent
static clock nack_timer;
// the last audio packet we nacked
static int last_nacked_id = -1;
// the largest audio packet ID we have received so far
static int max_received_id = -1;
// the last ID we've processed for rendering
static int last_played_id = -1;

static double decoded_bytes_per_packet = 8192.0 / MAX_NUM_AUDIO_INDICES;

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

void audio_nack(int id, int index) {
    /*
      Send a negative acknowledgement to the server if an audio
      packet is missing

      Arguments:
          id (int): missing packet ID
          index (int): missing packet index
  */
    LOG_INFO("Missing Audio Packet ID %d, Index %d. NACKing...", id, index);
    // Initialize and populate a FractalClientMessage
    FractalClientMessage fmsg = {0};
    fmsg.type = MESSAGE_AUDIO_NACK;
    fmsg.nack_data.id = id;
    fmsg.nack_data.index = index;
    send_fmsg(&fmsg);
}

void nack_missing_packets() {
    /*
      Nack up to MAX_NACKED packets from last_nacked_id to max_received_id - 4. The -4 accounts for
      the fact that packets can arrive out of order, but abrupt jumps indicate that a packet is
      probably missing.
    */
    int num_nacked = 0;
    // make sure we're not nacking too frequently
    if (last_played_id > -1 && get_timer(nack_timer) > 6.0 / MS_IN_SECOND) {
        last_nacked_id = max(last_played_id, last_nacked_id);
        // nack up to MAX_NACKED packets
        for (int i = last_nacked_id + 1; i < max_received_id - 4 && num_nacked < MAX_NACKED; i++) {
            int i_buffer_index = i % RECV_AUDIO_BUFFER_SIZE;
            AudioPacket* i_packet = &receiving_audio[i_buffer_index];
            if (i_packet->id == -1 && i_packet->nacked_amount < 2) {
                // check if we never received this packet
                i_packet->nacked_amount++;
                int id = i / MAX_NUM_AUDIO_INDICES;
                int index = i % MAX_NUM_AUDIO_INDICES;
                audio_nack(id, index);
                num_nacked++;

                start_timer(&nack_timer);
            }
            last_nacked_id = i;
        }
    }
}

void catchup_audio() {
    /*
      Catch up to the most recently received ID if no audio has played yet and clean out the ring
      buffer. The logic inside the if statement should only run once: when we have received a
      packet, but not yet updated the rest of the audio state.
     */

    if (last_played_id == -1 && has_video_rendered_yet && max_received_id > 0) {
        last_played_id = max_received_id - 1;
        while (last_played_id % MAX_NUM_AUDIO_INDICES != MAX_NUM_AUDIO_INDICES - 1) {
            last_played_id++;
        }

        // Clean out the old packets
        for (int i = 0; i < RECV_AUDIO_BUFFER_SIZE; i++) {
            if (receiving_audio[i].id <= last_played_id) {
                receiving_audio[i].id = -1;
                receiving_audio[i].nacked_amount = 0;
            }
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

    if (next_to_play_id % MAX_NUM_AUDIO_INDICES != 0) {
        LOG_WARNING("Next to play (ID: %d) isn't at start of audio frame", next_to_play_id);
        return false;
    }

    // check that the next frame we want to render is ready
    bool valid = true;
    for (int i = next_to_play_id; i < next_to_play_id + MAX_NUM_AUDIO_INDICES; i++) {
        if (receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].id != i) {
            valid = false;
        }
    }
    return valid;
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
        (int)((max_received_id - last_played_id) * decoded_bytes_per_packet) + audio_device_queue;

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
    int next_to_play_id = last_played_id + 1;
    for (int i = next_to_play_id; i < next_to_play_id + MAX_NUM_AUDIO_INDICES; i++) {
        AudioPacket* packet = &receiving_audio[i % RECV_AUDIO_BUFFER_SIZE];
        packet->id = -1;
        packet->nacked_amount = 0;
    }
    // increment to indicate we've skipped these packets
    last_played_id += MAX_NUM_AUDIO_INDICES;
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
        LOG_WARNING("Audio queue full, skipping ID %d (Queued: %d)",
                    next_to_play_id / MAX_NUM_AUDIO_INDICES, audio_device_queue);
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
    audio_render_context.encoded = USING_AUDIO_ENCODE_DECODE;
    int next_to_play_id = last_played_id + 1;
    for (int i = 0; i < MAX_NUM_AUDIO_INDICES; i++) {
        // move the packet into the render context
        int buffer_index = next_to_play_id + i;
        memcpy((AudioPacket*)&audio_render_context.audio_packets[i],
               &receiving_audio[buffer_index % RECV_AUDIO_BUFFER_SIZE], sizeof(AudioPacket));
        // Reset packet in receiving audio buffer
        AudioPacket* packet = &receiving_audio[buffer_index % RECV_AUDIO_BUFFER_SIZE];
        packet->id = -1;
        packet->nacked_amount = 0;
    }
    // increment to indicate that we've processed these packets
    last_played_id += MAX_NUM_AUDIO_INDICES;
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

int get_next_audio_frame(uint8_t* data) {
    /*
      Get the next (encoded) audio frame from the render context and decode it into the data buffer

      Arguments:
          decoded_data (uint8_t*): Data buffer to receive the decoded audio data
    */
    // TODO: frame global variable, like in video? or just take the ring buffer entry
    // TODO: call audio_decoder_send_packets(audio_context.audio_decoder, ring buffer entry->data,
    // ring buffer entry->size); setup the frame
    AVPacket* encoded_packet = av_packet_alloc();
    // reconstruct the audio frame from the indices.
    for (int i = 0; i < MAX_NUM_AUDIO_INDICES; i++) {
        AudioPacket* packet = (AudioPacket*)&audio_render_context.audio_packets[i];
        int old_size = encoded_packet->size;
        int res = av_grow_packet(encoded_packet, packet->size);
        if (res != 0) {
            return res;
        }
        memcpy(encoded_packet->data + old_size, packet->data, packet->size);
    }

    // Decode encoded audio
    int res = audio_decoder_decode_packet(audio_context.audio_decoder, encoded_packet);
    av_packet_free(&encoded_packet);

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
    start_timer(&nack_timer);
    for (int i = 0; i < RECV_AUDIO_BUFFER_SIZE; i++) {
        receiving_audio[i].id = -1;
        receiving_audio[i].nacked_amount = 0;
        receiving_audio[i].nacked_for = -1;
    }
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

        // this buffer will always hold the decoded data
        static uint8_t decoded_data[MAX_AUDIO_FRAME_SIZE];
        // decode the frame into the buffer
        int res = get_next_audio_frame(decoded_data);
        if (res == 0) {
            // play the audio
            res = SDL_QueueAudio(audio_context.dev, decoded_data,
                                 audio_decoder_get_frame_data_size(audio_context.audio_decoder));

            if (res < 0) {
                LOG_ERROR("Could not play audio!");
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

    // Return if there's nothing to play
    if (last_played_id == -1) {
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

    // Find pending audio packets and NACK them
    nack_missing_packets();
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
    // make sure that we do not handle packets that construct frames that are bigger than we expect
    if (packet->num_indices > MAX_NUM_AUDIO_INDICES) {
        LOG_WARNING("Packet Index too large!");
        return -1;
    }
    // also don't handle packets if the frequency is too high
    if (audio_frequency == MAX_FREQ) {
        return 0;
    }

    int audio_id = packet->id * MAX_NUM_AUDIO_INDICES + packet->index;
    AudioPacket* buffer_pkt = &receiving_audio[audio_id % RECV_AUDIO_BUFFER_SIZE];

    if (audio_id == buffer_pkt->id) {
        // check if we've already received the audio packet
        LOG_WARNING("Already received audio packet: %d", audio_id);
    } else if (audio_id < buffer_pkt->id || audio_id <= last_played_id) {
        // check if we've gotten an old packet
        LOG_INFO("Old audio packet received: %d, last played id is %d", audio_id, last_played_id);
    }
    // audio_id > buffer_pkt->id && audio_id > last_played_id
    else {
        // If a packet already exists there, we're forced to skip it
        if (buffer_pkt->id != -1) {
            int old_last_played_id = last_played_id;

            if (last_played_id < buffer_pkt->id && last_played_id > 0) {
                // We'll make it like we already played this packet
                last_played_id = buffer_pkt->id;
                buffer_pkt->id = -1;
                buffer_pkt->nacked_amount = 0;

                // And we'll skip the whole frame
                while (last_played_id % MAX_NUM_AUDIO_INDICES != MAX_NUM_AUDIO_INDICES - 1) {
                    // "Play" that packet
                    last_played_id++;
                    receiving_audio[last_played_id % RECV_AUDIO_BUFFER_SIZE].id = -1;
                    receiving_audio[last_played_id % RECV_AUDIO_BUFFER_SIZE].nacked_amount = 0;
                }
            }

            LOG_INFO(
                "Audio packet being overwritten before being played! ID %d "
                "replaced with ID %d, when the Last Played ID was %d. Last "
                "Played ID is Now %d",
                buffer_pkt->id, audio_id, old_last_played_id, last_played_id);
        }

        if (packet->is_a_nack) {
            // check if this is packet we nacked for
            if (buffer_pkt->nacked_for == audio_id) {
                LOG_INFO("NACK for Audio ID %d, Index %d Received!", packet->id, packet->index);
            } else if (buffer_pkt->nacked_for == -1) {
                LOG_INFO(
                    "NACK for Audio ID %d, Index %d Received! But not "
                    "needed.",
                    packet->id, packet->index);
            } else {
                LOG_ERROR("NACK for Audio ID %d, Index %d Received, but of unexpected index?",
                          packet->id, packet->index);
            }
        }

        if (buffer_pkt->nacked_for == audio_id) {
            LOG_INFO(
                "Packet for Audio ID %d, Index %d Received! But it was already "
                "NACK'ed!",
                packet->id, packet->index);
        }
        buffer_pkt->nacked_for = -1;

#if LOG_AUDIO
        LOG_DEBUG("Receiving Audio Packet %d (%d), trying to render %d\n", audio_id,
                  packet->payload_size, last_played_id + 1);
#endif
        // set the buffer slot to the data of the audio ID
        buffer_pkt->id = audio_id;
        max_received_id = max(buffer_pkt->id, max_received_id);
        buffer_pkt->size = packet->payload_size;
        memcpy(buffer_pkt->data, packet->data, packet->payload_size);

        if (packet->index + 1 == packet->num_indices) {
            // this will fill in any remaining indices to have size 0
            for (int i = audio_id + 1; i % MAX_NUM_AUDIO_INDICES != 0; i++) {
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].id = i;
                receiving_audio[i % RECV_AUDIO_BUFFER_SIZE].size = 0;
                max_received_id = max(max_received_id, i);
            }
        }
    }

    return 0;
}
