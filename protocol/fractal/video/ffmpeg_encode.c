#include "ffmpeg_encode.h"

FFmpegEncoder* create_ffmpeg_encoder(int in_width, int in_height, int out_width, int out_height,
                                     int bitrate, CodecType codec_type) {
    // TODO: UNFINISHED
    FFmpegEncoderCreator encoder_precedence[] = {create_nvenc_encoder, create_sw_encoder};
    for (unsigned int i = 0; i < sizeof(encoder_precedence) / sizeof(VideoEncoderCreator); ++i) {
        encoder->ffmpeg_encoder =
            encoder_precedence[i](in_width, in_height, out_width, out_height, bitrate, codec_type);

        if (!encoder) {
            LOG_WARNING("FFmpeg encoder: Failed, trying next encoder");
            encoder = NULL;
        } else {
            LOG_INFO("Video encoder: Success!");
            break;
        }
    }
}

int ffmpeg_encoder_frame_intake(FFmpegEncoder* encoder, void* rgb_pixels, int pitch) {
    /*
        Copy frame data in rgb_pixels and pitch to the software frame, and to the hardware frame if
       possible.

        Arguments:
            encoder (VideoEncoder*): video encoder containing encoded frames
            rgb_pixels (void*): pixel data for the frame
            pitch (int): Pitch data for the frame

        Returns:
            (int): 0 on success, -1 on failure
     */
    if (!encoder) {
        LOG_ERROR("ffmpeg_encoder_frame_intake received NULL encoder!");
        return -1;
    }
    memset(encoder->sw_frame->data, 0, sizeof(encoder->sw_frame->data));
    memset(encoder->sw_frame->linesize, 0, sizeof(encoder->sw_frame->linesize));
    encoder->sw_frame->data[0] = (uint8_t*)rgb_pixels;
    encoder->sw_frame->linesize[0] = pitch;
    encoder->sw_frame->pts++;

    if (encoder->hw_frame) {
        int res = av_hwframe_transfer_data(encoder->hw_frame, encoder->sw_frame, 0);
        if (res < 0) {
            LOG_ERROR("Unable to transfer frame to hardware frame: %s", av_err2str(res));
            return -1;
        }
        encoder->hw_frame->pict_type = encoder->sw_frame->pict_type;
    }
    return 0;
}

void ffmpeg_set_iframe(FFmpegEncoder* encoder) {
    if (!encoder) {
        LOG_ERROR("ffmpeg_set_iframe received NULL encoder!");
        return;
    }
    LOG_ERROR("ffmpeg set_iframe doesn't work very well! This might not work!");
    encoder->sw_frame->pict_type = AV_PICTURE_TYPE_I;
    encoder->sw_frame->pts +=
        encoder->context->gop_size - (encoder->sw_frame->pts % encoder->context->gop_size);
    encoder->sw_frame->key_frame = 1;
}

void ffmpeg_unset_iframe(FFmpegEncoder* encoder) {
    if (!encoder) {
        LOG_ERROR("ffmpeg_unset_iframe received NULL encoder!");
        return;
    }
    LOG_ERROR("ffmpeg unset_iframe doesn't work very well! This might not work!");
    encoder->sw_frame->pict_type = AV_PICTURE_TYPE_NONE;
    encoder->sw_frame->key_frame = 0;
}

void destroy_ffmpeg_encoder(FFmpegEncoder* encoder) {
    if (!encoder) {
        return;
    }

    if (encoder->context) {
        avcodec_free_context(&encoder->context);
    }

    if (encoder->filter_graph) {
        avfilter_graph_free(&encoder->filter_graph);
    }

    if (encoder->hw_device_ctx) {
        av_buffer_unref(&encoder->hw_device_ctx);
    }

    av_frame_free(&encoder->hw_frame);
    av_frame_free(&encoder->sw_frame);
    av_frame_free(&encoder->filtered_frame);

    // free the buffer and encoder
    free(encoder->sw_frame_buffer);
    // TODO: check if we actually need to free the pointer
    free(encoder);
}

// Goes through NVENC/QSV/SOFTWARE and sees which one works, cascading to the
// next one when the previous one doesn't work
int ffmpeg_encoder_send_frame(FFmpegEncoder *encoder) {
    /*
        Send a frame through the filter graph, then encode it.

        Arguments:
            encoder (VideoEncoder*): encoder used to encode the frame

        Returns:
            (int): 0 on success, -1 on failure
    */
    AVFrame *active_frame = encoder->sw_frame;
    if (encoder->hw_frame) {
        active_frame = encoder->hw_frame;
    }

    int res = av_buffersrc_add_frame(encoder->filter_graph_source, active_frame);
    if (res < 0) {
        LOG_WARNING("Error submitting frame to the filter graph: %s", av_err2str(res));
    }

    if (encoder->hw_frame) {
        // have to re-create buffers after sending to filter graph
        av_hwframe_get_buffer(encoder->context->hw_frames_ctx, encoder->hw_frame, 0);
    }

    int res_buffer;

    // submit all available frames to the encoder
    while ((res_buffer = av_buffersink_get_frame(encoder->filter_graph_sink,
                                                 encoder->filtered_frame)) >= 0) {
        int res_encoder = avcodec_send_frame(encoder->context, encoder->filtered_frame);

        // unref the frame so it may be reused
        av_frame_unref(encoder->filtered_frame);

        if (res_encoder < 0) {
            LOG_WARNING("Error sending frame for encoding: %s", av_err2str(res_encoder));
            return -1;
        }
    }
    if (res_buffer < 0 && res_buffer != AVERROR(EAGAIN) && res_buffer != AVERROR_EOF) {
        LOG_WARNING("Error getting frame from the filter graph: %d -- %s", res_buffer,
                    av_err2str(res_buffer));
        return -1;
    }

    return 0;
}

int ffmpeg_encoder_receive_packet(FFmpegEncoder *encoder, AVPacket *packet) {
    /*
        Wrapper around FFmpeg's avcodec_receive_packet. Get an encoded packet from the encoder
       and store it in packet.

        Arguments:
            encoder (VideoEncoder*): encoder used to encode the frame
            packet (AVPacket*): packet in which to store encoded data

        Returns:
            (int): 1 on EAGAIN (no more packets), 0 on success (call this function again), and
       -1 on failure.
    */
    int res_encoder;

    // receive_packet already calls av_packet_unref, no need to reinitialize packet
    res_encoder = avcodec_receive_packet(encoder->context, packet);
    if (res_encoder == AVERROR(EAGAIN) || res_encoder == AVERROR(EOF)) {
        return 1;
    } else if (res_encoder < 0) {
        LOG_ERROR("Error getting frame from the encoder: %s", av_err2str(res_encoder));
        return -1;
    }

    return 0;
}
