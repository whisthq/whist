#pragma once

/*
============================
Flags for client only
============================
*/

// the protocol analyzer is started automatically after debug_console starts.
// but in rare case, you might want to disable it to make performance plotting more accurate
#define DISABLE_PROTOCOL_ANALYZER false

// if enabled, plotter will start sampling on startup, and export on **graceful** quit
#define CLIENT_SIDE_PLOTTER_START_SAMPLING_BY_DEFAULT true
// the file to export if above is enabled
#define CLIENT_SIDE_DEFAULT_EXPORT_FILE "/tmp/plot.json"

// flags for enable/disable plotting of groups of datasets
#define PLOT_AUDIO_ALGO false                // audio algo
#define PLOT_RENDER_VIDEO false              // time of calling render_video()
#define PLOT_SDL_PRESENT_FRAME_BUFFER false  // time of calling sdl_present_xxxx()s

#define PLOT_VIDEO_FIRST_SEEN_TO_DECODE \
    false  // how long it takes between video frame is seen and send to decoder

#define PLOT_CLIENT_UDP_SOCKET_RECV_QUEUE false
/*
============================
Flags for server only
============================
*/
// if enabled, plotter will start sampling on startup, and export on **graceful** quit
#define SERVER_SIDE_PLOTTER_START_SAMPLING_BY_DEFAULT false
// the file to export if above is enabled
#define SERVER_SIDE_DEFAULT_EXPORT_FILE "/plot.json"

// plot server udp message handling time
#define PLOT_SERVER_UDP_MESSAGE_HANDING false
/*
============================
Flags for both client and server
============================
*/

#define PLOT_UDP_RECV_GAP false  // plot the gaps between recvs() are called on hotpath

#define PLOT_UDP_SEQ false
