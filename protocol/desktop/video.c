#include "video.h"

#define USE_HARDWARE true

// Global Variables
extern volatile SDL_Window *window;
extern volatile SDL_Renderer* renderer;

extern volatile int server_width;
extern volatile int server_height;
// Keeping track of max mbps
extern volatile double max_mbps;
extern volatile bool update_mbps;

extern volatile SDL_Window* window;
extern volatile int output_width;
extern volatile int output_height;

// START VIDEO VARIABLES
volatile FractalCursorState cursor_state = CURSOR_STATE_VISIBLE;
volatile SDL_Cursor* cursor = NULL;
volatile FractalCursorID last_cursor = (FractalCursorID) SDL_SYSTEM_CURSOR_ARROW;

#define LOG_VIDEO false

struct VideoData {
    struct FrameData* pending_ctx;
    int frames_received;
    int bytes_transferred;
    clock frame_timer;
    int last_statistics_id;
    int last_rendered_id;
    int max_id;
    int most_recent_iframe;

    clock last_iframe_request_timer;

    SDL_Thread* render_screen_thread;
    bool run_render_screen_thread;

    SDL_sem* renderscreen_semaphore;
} VideoData;

typedef struct SDLVideoContext {
    SDL_Texture* texture;

    Uint8* data[4];
    int linesize[4];

    video_decoder_t* decoder;
    struct SwsContext* sws;
} SDLVideoContext;
SDLVideoContext videoContext;

typedef struct FrameData {
    char* frame_buffer;
    int frame_size;
    int id;
    int packets_received;
    int num_packets;
    bool received_indicies[LARGEST_FRAME_SIZE / MAX_PAYLOAD_SIZE + 5];
    bool nacked_indicies[LARGEST_FRAME_SIZE / MAX_PAYLOAD_SIZE + 5];
    bool rendered;

    int last_nacked_index;
    int num_times_nacked;
    clock last_nacked_timer;

    clock last_packet_timer;

    clock frame_creation_timer;
} FrameData;

// mbps that currently works
volatile double working_mbps = MAXIMUM_MBPS;

// Context of the frame that is currently being rendered
volatile struct FrameData renderContext;

// True if RenderScreen is currently rendering a frame
volatile bool rendering = false;
volatile bool skip_render = false;

// Hold information about frames as the packets come in
#define RECV_FRAMES_BUFFER_SIZE 275
struct FrameData receiving_frames[RECV_FRAMES_BUFFER_SIZE];
char frame_bufs[RECV_FRAMES_BUFFER_SIZE][LARGEST_FRAME_SIZE];

bool has_rendered_yet = false;

// END VIDEO VARIABLES

// START VIDEO FUNCTIONS

void updateWidthAndHeight(int width, int height);
int32_t RenderScreen(void* opaque);

void nack(int id, int index) {
    mprintf("Missing Video Packet ID %d Index %d, NACKing...\n", id, index);
    FractalClientMessage fmsg;
    fmsg.type = MESSAGE_VIDEO_NACK;
    fmsg.nack_data.id = id;
    fmsg.nack_data.index = index;
    SendFmsg( &fmsg );
}

bool requestIframe()
{
    if( GetTimer( VideoData.last_iframe_request_timer ) > 250.0 / 1000.0 )
    {
        FractalClientMessage fmsg;
        fmsg.type = MESSAGE_IFRAME_REQUEST;
        SendFmsg( &fmsg );
        StartTimer( &VideoData.last_iframe_request_timer );
        return true;
    } else
    {
        return false;
    }
}

void updateWidthAndHeight(int width, int height) {
    video_decoder_t* decoder = create_video_decoder(width, height, USE_HARDWARE);
    videoContext.decoder = decoder;
    if( !decoder )
    {
        mprintf( "ERROR: Decoder could not be created!\n" );
        exit( -1 );
    }

    enum AVPixelFormat input_fmt = AV_PIX_FMT_YUV420P;
    if(decoder->type != DECODE_TYPE_SOFTWARE) {
        input_fmt = AV_PIX_FMT_NV12;
    }

    struct SwsContext* sws_ctx = NULL;
    if( input_fmt != AV_PIX_FMT_YUV420P )
    {
        sws_ctx = sws_getContext( width, height,
                                  input_fmt, output_width, output_height,
                                  AV_PIX_FMT_YUV420P,
                                  SWS_BILINEAR,
                                  NULL,
                                  NULL,
                                  NULL
        );
    }
    videoContext.sws = sws_ctx;

    server_width = width;
    server_height = height;
}

int32_t RenderScreen(void* opaque) {
    opaque;

    mprintf("RenderScreen running on Thread %d\n", SDL_GetThreadID(NULL));
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    while (VideoData.run_render_screen_thread) {
        int ret = SDL_SemTryWait(VideoData.renderscreen_semaphore);

        if (ret == SDL_MUTEX_TIMEDOUT) {
            SDL_Delay(1);
            continue;
        }

        if (ret < 0) {
            mprintf("Semaphore Error\n");
            return -1;
        }

        if (!rendering) {
            mprintf("Sem opened but rendering is not true!\n");
            continue;
        }

        // Cast to Frame* because this variable is not volatile in this section
        Frame* frame = (Frame*)renderContext.frame_buffer;

#if LOG_VIDEO
        mprintf("Rendering ID %d (Age %f)\n", renderContext.id, GetTimer( renderContext.frame_creation_timer ));
#endif
        mprintf( "Rendering ID %d (Age %f) (Packets %d) %s\n", renderContext.id, GetTimer( renderContext.frame_creation_timer ), renderContext.num_packets, frame->is_iframe ? "(I-Frame)" : "" );

        if( GetTimer( renderContext.frame_creation_timer ) > 25.0 / 1000.0 )
        {
            mprintf( "Late! Rendering ID %d (Age %f) (Packets %d) %s\n", renderContext.id, GetTimer( renderContext.frame_creation_timer ), renderContext.num_packets, frame->is_iframe ? "(I-Frame)" : "" );
        }

        if ((int) (sizeof(Frame) + frame->size) != renderContext.frame_size) {
            mprintf("Incorrect Frame Size! %d instead of %d\n", sizeof(Frame) + frame->size, renderContext.frame_size);
        }

        if (frame->width != server_width || frame->height != server_height) {
            mprintf("Updating client rendering to match server's width and height! From %dx%d to %dx%d\n", server_width, server_height, frame->width, frame->height);
            updateWidthAndHeight(frame->width, frame->height);
        }

        if( !video_decoder_decode( videoContext.decoder, frame->compressed_frame, frame->size ) )
        {
            mprintf( "Failed to video_decoder_decode!\n" );
            rendering = false;
            continue;
        }
         
        if( !skip_render )
        {
            if( videoContext.sws )
            {
                sws_scale( videoContext.sws, (uint8_t const* const*)videoContext.decoder->sw_frame->data,
                           videoContext.decoder->sw_frame->linesize, 0, videoContext.decoder->context->height, videoContext.data,
                           videoContext.linesize );
            } else
            {
                memcpy( videoContext.data, videoContext.decoder->sw_frame->data, sizeof( videoContext.data ) );
                memcpy( videoContext.linesize, videoContext.decoder->sw_frame->linesize, sizeof( videoContext.linesize ) );
            }

            SDL_UpdateYUVTexture(
                videoContext.texture,
                NULL,
                videoContext.data[0],
                videoContext.linesize[0],
                videoContext.data[1],
                videoContext.linesize[1],
                videoContext.data[2],
                videoContext.linesize[2]
            );
        }

        // Set cursor to frame's desired cursor type
        if((FractalCursorID) frame->cursor.cursor_id != last_cursor) {
            if(cursor) {
                SDL_FreeCursor((SDL_Cursor *) cursor);
            }
            cursor = SDL_CreateSystemCursor(frame->cursor.cursor_id);
            SDL_SetCursor((SDL_Cursor *) cursor);

            last_cursor = (FractalCursorID) frame->cursor.cursor_id;
        }

        if(frame->cursor.cursor_state != cursor_state) {
            if(frame->cursor.cursor_state == CURSOR_STATE_HIDDEN) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
            } else {
                SDL_SetRelativeMouseMode(SDL_DISABLE);
            }

            cursor_state = frame->cursor.cursor_state;
        }

        //mprintf("Client Frame Time for ID %d: %f\n", renderContext.id, GetTimer(renderContext.client_frame_timer));

        if( !skip_render )
        {
            SDL_RenderCopy( (SDL_Renderer*)renderer, videoContext.texture, NULL, NULL );
            SDL_RenderPresent( (SDL_Renderer*)renderer );
        }

#if LOG_VIDEO
        mprintf("Rendered %d (Size: %d) (Age %f)\n", renderContext.id, renderContext.frame_size, GetTimer(renderContext.frame_creation_timer));
#endif

        VideoData.last_rendered_id = renderContext.id;
        has_rendered_yet = true;
        rendering = false;
    }

    SDL_Delay(5);
    return 0;
}
// END VIDEO FUNCTIONS

void initVideo() {
    // mbps that currently works
    working_mbps = MAXIMUM_MBPS;

    // True if RenderScreen is currently rendering a frame
    rendering = false;
    has_rendered_yet = false;

    SDL_Texture* texture;

    SDL_SetRenderDrawBlendMode((SDL_Renderer*) renderer, SDL_BLENDMODE_BLEND);

    // Allocate a place to put our YUV image on that screen
    texture = SDL_CreateTexture(
        (SDL_Renderer *) renderer,
        SDL_PIXELFORMAT_YV12,
        SDL_TEXTUREACCESS_STREAMING,
        output_width,
        output_height
    );
    if (!texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }

    videoContext.texture = texture;

    av_image_alloc( videoContext.data, videoContext.linesize, output_width, output_height, AV_PIX_FMT_YUV420P, 1 );

    VideoData.pending_ctx = NULL;
    VideoData.frames_received = 0;
    VideoData.bytes_transferred = 0;
    StartTimer(&VideoData.frame_timer);
    VideoData.last_statistics_id = 1;
    VideoData.last_rendered_id = 0;
    VideoData.max_id = 0;
    VideoData.most_recent_iframe = -1;
    StartTimer( &VideoData.last_iframe_request_timer );

    for (int i = 0; i < RECV_FRAMES_BUFFER_SIZE; i++) {
        receiving_frames[i].id = -1;
    }

    VideoData.renderscreen_semaphore = SDL_CreateSemaphore(0);
    VideoData.run_render_screen_thread = true;
    VideoData.render_screen_thread = SDL_CreateThread(RenderScreen, "RenderScreen", NULL);
}

int last_rendered_index = 0;

void updateVideo() {
    // Get statistics from the last 3 seconds of data
    if (GetTimer(VideoData.frame_timer) > 3) {
        //double time = GetTimer(VideoData.frame_timer);

        // Calculate statistics
        int expected_frames = VideoData.max_id - VideoData.last_statistics_id;
        //double fps = 1.0 * expected_frames / time; // TODO: finish birate throttling alg
        //double mbps = VideoData.bytes_transferred * 8.0 / 1024.0 / 1024.0 / time; // TODO bitrate throttle
        double receive_rate = expected_frames == 0 ? 1.0 : 1.0 * VideoData.frames_received / expected_frames;
        double dropped_rate = 1.0 - receive_rate;

        // Print statistics

        //mprintf("FPS: %f\nmbps: %f\ndropped: %f%%\n\n", fps, mbps, 100.0 * dropped_rate);

        // Adjust mbps based on dropped packets
        if (dropped_rate > 0.4) {
            max_mbps = max_mbps * 0.75;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.2) {
            max_mbps = max_mbps * 0.83;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.1) {
            max_mbps = max_mbps * 0.9;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.05) {
            max_mbps = max_mbps * 0.95;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate > 0.00) {
            max_mbps = max_mbps * 0.98;
            working_mbps = max_mbps;
            update_mbps = true;
        }
        else if (dropped_rate == 0.00) {
            working_mbps = max(max_mbps * 1.05, working_mbps);
            max_mbps = (max_mbps + working_mbps) / 2.0;
            max_mbps = max( max_mbps, MAXIMUM_MBPS );
            update_mbps = true;
        }

        VideoData.bytes_transferred = 0;
        VideoData.frames_received = 0;
        VideoData.last_statistics_id = VideoData.max_id;
        StartTimer(&VideoData.frame_timer);
    }

    if (VideoData.last_rendered_id == -1 && VideoData.most_recent_iframe > 0) {
        VideoData.last_rendered_id = VideoData.most_recent_iframe - 1;
    }

    if (!rendering && VideoData.last_rendered_id >= 0) {
        if (VideoData.most_recent_iframe - 1 > VideoData.last_rendered_id) {
            mprintf("Skipping from %d to i-frame %d!\n", VideoData.last_rendered_id, VideoData.most_recent_iframe);
            for (int i = VideoData.last_rendered_id + 1; i < VideoData.most_recent_iframe; i++) {
                int index = i % RECV_FRAMES_BUFFER_SIZE;
                if (receiving_frames[index].id == i) {
                    mprintf("Frame dropped with ID %d: %d/%d\n", i, receiving_frames[index].packets_received, receiving_frames[index].num_packets);

                    for( int j = 0; j < receiving_frames[index].num_packets; j++ )
                    {
                        if( !receiving_frames[index].received_indicies[j] )
                        {
                            mprintf( "Did not receive ID %d, Index %d\n", i, j );
                        }
                    }
                }
                else {
                    mprintf("Bad ID? %d instead of %d\n", receiving_frames[index].id, i);
                }
            }
            VideoData.last_rendered_id = VideoData.most_recent_iframe - 1;
        }

        int next_render_id = VideoData.last_rendered_id + 1;

        int index = next_render_id % RECV_FRAMES_BUFFER_SIZE;

        struct FrameData* ctx = &receiving_frames[index];

        if (ctx->id == next_render_id) {
            if (ctx->packets_received == ctx->num_packets) {
                //mprintf( "Packets: %d %s\n", ctx->num_packets, ((Frame*)ctx->frame_buffer)->is_iframe ? "(I-frame)" : "" );
                //mprintf("Rendering %d (Age %f)\n", ctx->id, GetTimer(ctx->frame_creation_timer));

                renderContext = *ctx;
                rendering = true;

                skip_render = false;
                int after_render_id = next_render_id + 1;
                int after_index = after_render_id % RECV_FRAMES_BUFFER_SIZE;
                struct FrameData* after_ctx = &receiving_frames[after_index];
                if( after_ctx->id == after_render_id && after_ctx->packets_received == after_ctx->num_packets )
                {
                    skip_render = true;
                    mprintf( "Skip this render\n" );
                }

                SDL_SemPost(VideoData.renderscreen_semaphore);
            }
            else
            {
                if( GetTimer( ctx->last_packet_timer ) > 96.0 / 1000.0 || VideoData.max_id > ctx->id + 3 )
                {
                    if( requestIframe() )
                    {
                        mprintf( "TOO FAR BEHIND! REQUEST FOR IFRAME!\n" );
                    }
                }

                if( (GetTimer( ctx->last_packet_timer ) > 6.0 / 1000.0) && GetTimer( ctx->last_nacked_timer ) > (8.0 + 8.0*ctx->num_times_nacked) / 1000.0 )
                {
                    if( ctx->num_times_nacked == -1 )
                    {
                        ctx->num_times_nacked = 0;
                        ctx->last_nacked_index = -1;
                    }
                    int num_nacked = 0;
                    //mprintf("************NACKING PACKET %d, alive for %f MS\n", ctx->id, GetTimer(ctx->frame_creation_timer));
                    for( int i = ctx->last_nacked_index + 1; i < ctx->num_packets && num_nacked < 1; i++ )
                    {
                        if( !ctx->received_indicies[i] )
                        {
                            num_nacked++;
                            mprintf( "************NACKING VIDEO PACKET %d %d (/%d), alive for %f MS\n", ctx->id, i, ctx->num_packets, GetTimer( ctx->frame_creation_timer ) );
                            ctx->nacked_indicies[i] = true;
                            nack( ctx->id, i );
                        }
                        ctx->last_nacked_index = i;
                    }
                    if( ctx->last_nacked_index == ctx->num_packets - 1 )
                    {
                        ctx->last_nacked_index = -1;
                        ctx->num_times_nacked++;
                    }
                    StartTimer( &ctx->last_nacked_timer );
                }
            }
        } else
        {
            // Next frame not received yet
            struct FrameData* last_ctx = &receiving_frames[VideoData.last_rendered_id % RECV_FRAMES_BUFFER_SIZE];

            if( last_ctx->id == VideoData.last_rendered_id && GetTimer(last_ctx->frame_creation_timer) > 250.0 / 1000.0 )
            {
                if( requestIframe() )
                {
                    mprintf( "Long time, no frames. Request iframe." );
                }
            }
        }
    }
}

int32_t ReceiveVideo(struct RTPPacket* packet) {
    //mprintf("Video Packet ID %d, Index %d (Packets: %d) (Size: %d)\n", packet->id, packet->index, packet->num_indices, packet->payload_size);

    // Find frame in linked list that matches the id
    VideoData.bytes_transferred += packet->payload_size;

    int index = packet->id % RECV_FRAMES_BUFFER_SIZE;

    struct FrameData* ctx = &receiving_frames[index];

    // Check if we have to initialize the frame buffer
    if (packet->id < ctx->id) {
        mprintf("Old packet received!\n");
        return -1;
    }
    else if (packet->id > ctx->id) {
        if (rendering && renderContext.id == ctx->id) {
            mprintf("Error! Currently rendering an ID that will be overwritten! Skipping packet.\n");
            return 0;
        }
        ctx->id = packet->id;
        ctx->frame_buffer = (char *) &frame_bufs[index];
        ctx->packets_received = 0;
        ctx->num_packets = packet->num_indices;
        ctx->last_nacked_index = -1;
        ctx->num_times_nacked = -1;
        ctx->rendered = false;
        ctx->frame_size = 0;
        memset(ctx->received_indicies, 0, sizeof(ctx->received_indicies));
        memset(ctx->nacked_indicies, 0, sizeof(ctx->nacked_indicies));
        StartTimer(&ctx->last_nacked_timer);
        StartTimer(&ctx->frame_creation_timer);
        //mprintf("Initialized packet %d!\n", ctx->id);
    }
    else {
       // mprintf("Already Started: %d/%d - %f\n", ctx->packets_received + 1, ctx->num_packets, GetTimer(ctx->client_frame_timer));
    }

    StartTimer(&ctx->last_packet_timer);

    // If we already received this packet, we can skip
    if (packet->is_a_nack) {
        if (!ctx->received_indicies[packet->index]) {
            mprintf("NACK for Video ID %d, Index %d Received!\n", packet->id, packet->index);
        }
        else {
            mprintf("NACK for Video ID %d, Index %d Received! But didn't need it.\n", packet->id, packet->index);
        }
    } else if (ctx->nacked_indicies[packet->index]) {
        mprintf("Received the original Video ID %d Index %d, but we had NACK'ed for it.\n", packet->id, packet->index);
    }

    // Already received
    if (ctx->received_indicies[packet->index]) {
#if LOG_VIDEO
        mprintf("Skipping duplicate Video ID %d Index %d at time since creation %f %s\n", packet->id, packet->index, GetTimer(ctx->frame_creation_timer), packet->is_a_nack ? "(nack)" : "");
#endif
        return 0;
    }

#if LOG_VIDEO
    //mprintf("Received Video ID %d Index %d at time since creation %f %s\n", packet->id, packet->index, GetTimer(ctx->frame_creation_timer), packet->is_a_nack ? "(nack)" : ""); 
#endif

    VideoData.max_id = max(VideoData.max_id, ctx->id);

    ctx->received_indicies[packet->index] = true;
    if (packet->index > 0 && GetTimer( ctx->last_nacked_timer ) > 6.0 / 1000) {
        int to_index = packet->index - 5;
        for (int i = max(0, ctx->last_nacked_index + 1); i <= to_index; i++) {
            if (!ctx->received_indicies[i]) {
                ctx->nacked_indicies[i] = true;
                nack(packet->id, i);
                break;
            }
        }
        ctx->last_nacked_index = max(ctx->last_nacked_index, to_index);
        StartTimer( &ctx->last_nacked_timer );
    }
    ctx->packets_received++;

    // Copy packet data
    int place = packet->index * MAX_PAYLOAD_SIZE;
    if (place + packet->payload_size >= LARGEST_FRAME_SIZE) {
        mprintf("Packet total payload is too large for buffer!\n");
        return -1;
    }
    memcpy(ctx->frame_buffer + place, packet->data, packet->payload_size);
    ctx->frame_size += packet->payload_size;

    // If we received all of the packets
    if (ctx->packets_received == ctx->num_packets) {
        bool is_iframe = ((Frame*)ctx->frame_buffer)->is_iframe;

        VideoData.frames_received++;

#if LOG_VIDEO
        mprintf("Received Video Frame ID %d (Packets: %d) (Size: %d) %s\n", ctx->id, ctx->num_packets, ctx->frame_size, is_iframe ? "(i-frame)" : "");
#endif

        // If it's an I-frame, then just skip right to it, if the id is ahead of the next to render id
        if (is_iframe) {
            VideoData.most_recent_iframe = max(VideoData.most_recent_iframe, ctx->id);
        }
    }

    return 0;
}

void destroyVideo() {
    VideoData.run_render_screen_thread = false;
    SDL_WaitThread(VideoData.render_screen_thread, NULL);
    SDL_DestroySemaphore(VideoData.renderscreen_semaphore);

    SDL_DestroyTexture(videoContext.texture);
    av_freep( videoContext.data );

    has_rendered_yet = false;
}
