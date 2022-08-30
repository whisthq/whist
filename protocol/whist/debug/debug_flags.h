#pragma once

// the protocol analyzer is started automatically after debug_console starts.
// but in rare case, you might want to disable it to make performance plotting more accurate
#define DISABLE_PROTOCOL_ANALYZER false

// flags for enable/disable plotting of groups of datasets
#define PLOT_AUDIO_ALGO false                // audio algo
#define PLOT_RENDER_VIDEO false              // time of calling render_video()
#define PLOT_SDL_PRESENT_FRAME_BUFFER false  // time of calling sdl_present_xxxx()s

#define PLOT_VIDEO_FIRST_SEEN_TO_DECODE \
    true  // how long it takes between video frame is seen and send to decoder

#define PLOT_UDP_RECV_GAP true  // plot the gaps between recvs() are called on hotpath
