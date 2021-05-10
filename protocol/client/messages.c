
// UPDATER CODE - HANDLES ALL PERIODIC UPDATES
bool tried_to_update_dimension;
bool has_initialized_updated;
clock tcp_check_timer;
clock last_ping_timer;
int last_ping_id;
int last_pong_id;


static bool updater_initialized = false;

void init_update() {
    /*
        Initialize client update handler.
        Anything that will be continuously be called (within `update()`)
        that changes program state should be initialized in here.
    */

    tried_to_update_dimension = false;

    start_timer((clock*)&last_tcp_check_timer);
    start_timer((clock*)&latency_timer);
    ping_id = 1;
    ping_failures = -2;

    init_clipboard_synchronizer(true);

    updater_initialized = true;
}

void destroy_update() {
    /*
        Runs the destruction sequence for anything that
        was initialized in `init_update()` and needs to be
        destroyed.
    */

    updater_initialized = false;
    destroy_clipboard_synchronizer();
}

void update_tcp_messages() {
    // Check for a new clipboard update from the server, if it's been 25ms since
    // the last time we checked the TCP socket, and the clipboard isn't actively
    // busy
    if (get_timer(last_tcp_check_timer) > 25.0 / MS_IN_SECOND &&
        !is_clipboard_synchronizing()) {
        // Check if TCP connction is active
        int result = ack(&packet_tcp_context);
        if (result < 0) {
            LOG_ERROR("Lost TCP Connection (Error: %d)", get_last_network_error());
            // TODO: Should exit or recover protocol if TCP connection is lost
        }

        // Receive tcp buffer, if a full packet has been received
        FractalPacket* tcp_packet = read_tcp_packet(&packet_tcp_context, true);
        if (tcp_packet) {
            handle_server_message((FractalServerMessage*)tcp_packet->data,
                                  (size_t)tcp_packet->payload_size);
            free_tcp_packet(tcp_packet);
        }

        // Update the last tcp check timer
        start_timer((clock*)&last_tcp_check_timer);
    }
}


void update_udp_messages() {
    /*
        Check all pending updates, and act on those pending updates
        to actually update the state of our program.
        This function expects to be called at minimum every 5ms to keep
        the program up-to-date.
    */

    if (!updater_initialized) {
        LOG_ERROR("Tried to update, but updater not initialized!");
    }

    FractalClientMessage fmsg = {0};

    // REFACTOR: This only runs once -- why does it need to be here?

    // If we haven't yet tried to update the dimension, and the dimensions don't
    // line up, then request the proper dimension
    if (!tried_to_update_dimension &&
        (server_width != output_width || server_height != output_height ||
         server_codec_type != output_codec_type)) {
        send_message_dimensions();
        tried_to_update_dimension = true;
    }

    // If the code has triggered a mbps update, then notify the server of the
    // newly desired mbps
    if (update_mbps) {
        update_mbps = false;
        fmsg.type = MESSAGE_MBPS;
        fmsg.mbps = max_bitrate / (double)BYTES_IN_KILOBYTE / BYTES_IN_KILOBYTE;
        LOG_INFO("Asking for server MBPS to be %f", fmsg.mbps);
        send_fmsg(&fmsg);
    }

    update_ping();
}
