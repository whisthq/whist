/**
 * Copyright (c) 2022 Whist Technologies, Inc.
 * @file network_test.cpp
 * @brief Unit tests for networking.
 */

#include <gtest/gtest.h>
#include "fixtures.hpp"

extern "C" {
#include "whist/core/whist_frame.h"
#include "whist/network/socket.h"
#include "whist/network/socket_internal.h"
#include "whist/utils/atomic.h"
#include "socket_fake.h"
}

// These are needed to hack around use of globals in other code.
extern "C" {
#include "whist/network/network_algorithm.h"
extern int output_width, output_height;
}

class NetworkTest : public CaptureStdoutFixture {};

static const WhistSocketImplementation *test_tcp_implementations[] = {
    &socket_implementation_bsd_tcp,
};

static const WhistSocketImplementation *test_udp_implementations[] = {
    &socket_implementation_bsd_udp,
    &socket_implementation_fake_udp,
};

int test_tcp_server_thread(void *arg) {
    WhistSocket *listener = (WhistSocket *)arg;

    bool exiting = false;
    while (!exiting) {
        WhistSocket *server;

        server = socket_accept(listener, NULL, NULL, "TCP Server");
        EXPECT_TRUE(server);

        while (1) {
            char buf[5];
            int ret;

            ret = socket_recv(server, buf, 5, 0);
            if (ret < 0) break;

            EXPECT_EQ(ret, 5);
            if (!strcmp(buf, "exit")) {
                exiting = true;
                break;
            }
            if (!strcmp(buf, "stop")) {
                break;
            }
            if (!strcmp(buf, "ping")) {
                ret = socket_send(server, "pong", 5, 0);
                EXPECT_EQ(ret, 5);
            }
        }

        socket_close(server);
    }

    return 0;
}

void test_tcp_socket_implementation(const WhistSocketImplementation *impl) {
    int ret;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(15324);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    WhistSocket *listener = socket_create(SOCKET_TYPE_TCP, "TCP Server Listener", impl, NULL);
    EXPECT_TRUE(listener);

    // Needed to allow the test to run repeatedly.
    ret = socket_set_option(listener, SOCKET_OPTION_REUSE_ADDRESS, 1);
    EXPECT_EQ(ret, 0);

    ret = socket_bind(listener, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    EXPECT_EQ(ret, 0);

    ret = socket_listen(listener, 1);
    EXPECT_EQ(ret, 0);

    WhistThread server_thread;
    server_thread =
        whist_create_thread(&test_tcp_server_thread, "TCP Test Server Thread", listener);
    EXPECT_TRUE(server_thread);

    for (int i = 0; i < 3; i++) {
        WhistSocket *client = socket_create(SOCKET_TYPE_TCP, "TCP Client", impl, NULL);
        EXPECT_TRUE(client);

        ret = socket_connect(client, (const struct sockaddr *)&server_addr, sizeof(server_addr));
        EXPECT_EQ(ret, 0);

        ret = socket_send(client, "ping", 5, 0);
        EXPECT_EQ(ret, 5);

        char buf[5];
        ret = socket_recv(client, buf, 5, 0);
        EXPECT_TRUE(!strcmp(buf, "pong"));

        ret = socket_send(client, i < 2 ? "stop" : "exit", 5, 0);
        EXPECT_EQ(ret, 5);

        socket_close(client);
    }

    whist_wait_thread(server_thread, NULL);

    socket_close(listener);
}

// Create all TCP socket implementations on loopback.
TEST_F(NetworkTest, TCPSocketTest) {
    for (unsigned int i = 0; i < ARRAY_LENGTH(test_tcp_implementations); i++) {
        const WhistSocketImplementation *impl = test_tcp_implementations[i];
        impl->global_init();
        test_tcp_socket_implementation(impl);
        impl->global_destroy();
    }
}

void test_udp_socket_implementation(const WhistSocketImplementation *impl) {
    int ret;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(15324);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    WhistSocket *server = socket_create(SOCKET_TYPE_UDP, "UDP Server", impl, NULL);
    EXPECT_TRUE(server);

    ret = socket_bind(server, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    EXPECT_EQ(ret, 0);

    struct sockaddr_in client_addr;
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(15342);
    client_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    WhistSocket *client = socket_create(SOCKET_TYPE_UDP, "UDP Client", impl, NULL);
    EXPECT_TRUE(client);

    ret = socket_bind(client, (const struct sockaddr *)&client_addr, sizeof(client_addr));
    EXPECT_EQ(ret, 0);

    for (int i = 0; i < 5; i++) {
        char buf[5];

        ret = socket_sendto(client, "ping", 5, 0, (const struct sockaddr *)&server_addr,
                            sizeof(server_addr));
        EXPECT_EQ(ret, 5);

        struct sockaddr return_addr;
        socklen_t return_addr_len = sizeof(return_addr);

        ret = socket_recvfrom(server, buf, 5, 0, &return_addr, &return_addr_len);
        EXPECT_EQ(ret, 5);
        EXPECT_TRUE(!strcmp(buf, "ping"));

        ret = socket_sendto(server, "pong", 5, 0, &return_addr, return_addr_len);
        EXPECT_EQ(ret, 5);

        ret = socket_recv(client, buf, 5, 0);
        EXPECT_EQ(ret, 5);
        EXPECT_TRUE(!strcmp(buf, "pong"));
    }

    socket_close(server);
    socket_close(client);
}

// Test all UDP socket implementations on loopback.
TEST_F(NetworkTest, UDPSocketTest) {
    for (unsigned int i = 0; i < ARRAY_LENGTH(test_udp_implementations); i++) {
        const WhistSocketImplementation *impl = test_udp_implementations[i];
        impl->global_init();
        test_udp_socket_implementation(impl);
        impl->global_destroy();
    }
}

void test_socket_options(const WhistSocketImplementation *impl) {
    int ret;

    WhistSocket *sock = socket_create(SOCKET_TYPE_UDP, "UDP Options", impl, NULL);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(15324);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    ret = socket_bind(sock, (const struct sockaddr *)&addr, sizeof(addr));
    EXPECT_EQ(ret, 0);

    char tmp;

    ret = socket_set_option(sock, SOCKET_OPTION_NON_BLOCKING, 1);
    EXPECT_EQ(ret, 0);

    // This should return immediately.
    ret = socket_recv(sock, &tmp, 1, 0);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(socket_get_last_error(sock), SOCKET_ERROR_WOULD_BLOCK);

    ret = socket_set_option(sock, SOCKET_OPTION_NON_BLOCKING, 0);
    EXPECT_EQ(ret, 0);

    ret = socket_set_option(sock, SOCKET_OPTION_RECEIVE_TIMEOUT, 10);
    EXPECT_EQ(ret, 0);

    // This should return after 10 milliseconds.
    ret = socket_recv(sock, &tmp, 1, 0);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(socket_get_last_error(sock), SOCKET_ERROR_WOULD_BLOCK);

    socket_dump_state(sock);

    socket_close(sock);
}

// Test socket options.
TEST_F(NetworkTest, SocketOptionsTest) {
    for (unsigned int i = 0; i < ARRAY_LENGTH(test_udp_implementations); i++) {
        const WhistSocketImplementation *impl = test_udp_implementations[i];
        impl->global_init();
        test_socket_options(impl);
        impl->global_destroy();
    }
}

typedef struct {
    // Human-readable description of network properties.
    const char *description;
    // We should be able to get at least this percentage of frames
    // through these conditions.
    int max_drop_percent;
    // Whether this network is so bad that losing connection is a
    // likely outcome, and should not be an error.
    bool connection_loss_allowed;

    // Packet delay for normal transit.
    uint32_t delay_ms;
    // Uniform random packet jitter, on top of delay.
    uint32_t delay_jitter_ms;

    // Uniform random chance of loss.
    uint32_t random_loss_ppm;
    // Chance of loss given the previous packet wasn't lost.
    uint32_t correlated_loss_good_ppm;
    // Chance of loss given the previous packet was lost.
    uint32_t correlated_loss_bad_ppm;

    // Bitrate allowed in the {down,up}stream direction.
    uint32_t bitrate_limit[2];
    // Maximum buffering in the {down,up}stream direction.
    uint32_t max_buffering[2];
} TestNetworkProperties;

// Test network setups.
static const TestNetworkProperties test_networks[] = {
    // Networks with no bitrate constraints, but variable reliability.
    // Congestion control can do nothing here, but resends and FEC can
    // make it work well.

    {"Low-latency", 0, false, 5, 0, 0, 0, 0},
    {"High-latency", 0, false, 100, 0, 0, 0, 0},
    {"20ms jitter", 0, false, 5, 20, 0, 0, 0},

    {"1% random loss", 5, false, 5, 0, 10000, 0, 0},
    {"1% loss in bunches of 10", 5, false, 5, 0, 0, 1010, 900000},
    {"50ms latency and 1% loss", 5, false, 50, 0, 10000, 0, 0},

    {"5% random loss", 50, false, 5, 0, 50000, 0, 0},
    {"5% loss in bunches of 20", 50, false, 5, 0, 0, 2632, 950000},
    {"30ms jitter and 5% random loss", 75, false, 30, 30, 50000, 0, 0},

    // Networks with bitrate constraints, but completely reliable.
    // Congestion control needs to find the constraints or they won't
    // work at all.

    {"100Mbps, no buffering", 0, false, 5, 0, 0, 0, 0, {100000000, 100000000}, {0, 0}},
    {"10Mbps, 10ms buffering", 10, false, 5, 0, 0, 0, 0, {10000000, 10000000}, {100000, 100000}},
    {"10Mbps, 1s buffering", 10, false, 5, 0, 0, 0, 0, {10000000, 10000000}, {10000000, 10000000}},
    {"5Mbps, 10ms buffering", 50, false, 5, 0, 0, 0, 0, {5000000, 5000000}, {50000, 50000}},
    {"5Mbps, 1s buffering", 50, false, 5, 0, 0, 0, 0, {5000000, 5000000}, {5000000, 5000000}},
    {"1Mbps, 10ms buffering", 100, true, 5, 0, 0, 0, 0, {1000000, 1000000}, {10000, 10000}},
    {"1Mbps, 1s buffering", 100, true, 5, 0, 0, 0, 0, {1000000, 1000000}, {1000000, 1000000}},
};

typedef struct {
    uint32_t server_ip;
    const char *server_ip_string;
    int server_port;
    char aes_key[16];
    int receive_timeout_ms;
    int connection_timeout_ms;

    // Number of frames to send in total.
    unsigned int frame_count;
    // Number of frames to send per second.
    unsigned int frame_rate;

    // There is no way to tell whether a client has connected from the
    // server side, so we make the client signal this when it sees the
    // connection succeed.
    atomic_int client_connected;
    // Flag to set when the server has received everything expected, so
    // the client can finish as well.
    atomic_int client_finished;

    // Number of frames dropped during the test.
    unsigned int dropped_frames;

    // Network properties.
    const TestNetworkProperties *network_properties;
    // Seed for deterministic randomness.
    uint32_t random_seed;
    // Whether the last packet was lost in the given direction.
    bool last_packet_lost[2];
    // Buffered bits as at last_packet_time for each direction.
    size_t buffered_bits[2];
    WhistTimer last_packet_time[2];
} UDPProtocolTestSetup;

static int test_udp_protocol_server_thread(void *arg) {
    UDPProtocolTestSetup *test = (UDPProtocolTestSetup *)arg;
    SocketContext server;
    int ret;

    // Create the server socket.
    ret = create_udp_socket_context(&server, NULL, test->server_port, test->receive_timeout_ms,
                                    test->connection_timeout_ms, false, test->aes_key);
    EXPECT_EQ(ret, true);

    udp_register_nack_buffer(&server, PACKET_VIDEO, LARGEST_VIDEOFRAME_SIZE, 256);

    udp_handle_network_settings(server.context, get_default_network_settings(1920, 1080, 96));

    // Wait for the client to connect.
    while (!atomic_load(&test->client_connected)) {
        socket_update(&server);
    }

    // Keep time to send at the right framerate, and also to let us time
    // out the test if things take too long.
    WhistTimer timer;
    start_timer(&timer);

    // Send data.
    bool connected = true;
    for (uint32_t id = 1; id <= test->frame_count; id++) {
        while (get_timer(&timer) < (double)(id - 1) / test->frame_rate && connected) {
            // Don't send yet.
            connected = socket_update(&server);
        }
        EXPECT_TRUE(connected || test->network_properties->connection_loss_allowed);
        if (!connected) {
            LOG_INFO("Server lost connection at frame %u, ending test early.", id);
            break;
        }

        bool reset = get_pending_stream_reset(&server, PACKET_VIDEO);
        if (reset) {
            LOG_INFO("Stream reset required!");
        }

        // Determine bitrate to use.
        NetworkSettings network_settings = udp_get_network_settings(&server);
        size_t frame_size = (network_settings.bitrate / test->frame_rate) / 8;
        if (frame_size < sizeof(VideoFrame)) {
            frame_size = sizeof(VideoFrame);
        }
        LOG_INFO("Bitrate = %d, frame_size = %zu.", network_settings.bitrate, frame_size);

        // Generate data to send.
        uint8_t *data = (uint8_t *)malloc(frame_size);
        for (size_t i = 0; i < frame_size; i++) data[i] = i & 255;

        // Add frame header over the start of the data.
        VideoFrame frame = {0};
        frame.width = 1920;
        frame.height = 1080;
        frame.codec_type = CODEC_TYPE_H264;
        frame.frame_type = (id == 1 || reset) ? VIDEO_FRAME_TYPE_INTRA : VIDEO_FRAME_TYPE_NORMAL;
        frame.frame_id = id;
        memcpy(data, &frame, sizeof(frame));

        send_packet(&server, PACKET_VIDEO, data, (int)frame_size, id, id == 1);
        LOG_INFO("Server sent frame %u.", id);

        free(data);
    }

    // Wait for the client to receive everything.
    while (!atomic_load(&test->client_finished)) {
        if (get_timer(&timer) > 5.0) {
            LOG_INFO("Timed out waiting for client to finish.");
            break;
        }
        socket_update(&server);
    }

    // Wait for the client to disconnect.
    while (socket_update(&server)) {
        if (get_timer(&timer) > 5.0) {
            LOG_INFO("Timed out waiting for connection to close.");
            break;
        }
    }

    // Destroy the socket.
    destroy_socket_context(&server);

    return 0;
}

static int test_udp_protocol_client_thread(void *arg) {
    UDPProtocolTestSetup *test = (UDPProtocolTestSetup *)arg;
    SocketContext client;
    int ret;

    // Create client socket and connect to the server.
    ret = create_udp_socket_context(&client, test->server_ip_string, test->server_port,
                                    test->receive_timeout_ms, test->connection_timeout_ms, false,
                                    test->aes_key);
    EXPECT_EQ(ret, true);

    udp_register_ring_buffer(&client, PACKET_VIDEO, LARGEST_VIDEOFRAME_SIZE, 256);

    // Signal that the client has connected.
    atomic_store(&test->client_connected, 1);

    // Receive data.
    bool connected = true;
    for (uint32_t id = 1; id <= test->frame_count && connected; id++) {
        bool received = false;
        do {
            connected = socket_update(&client);
            EXPECT_TRUE(connected || test->network_properties->connection_loss_allowed);
            if (!connected) {
                LOG_INFO("Client lost connection at frame %u, ending test early.", id);
                break;
            }

            WhistPacket *pkt = (WhistPacket *)get_packet(&client, PACKET_VIDEO);
            if (pkt) {
                // Verify header fields.
                EXPECT_EQ(pkt->type, PACKET_VIDEO);
                EXPECT_GE((uint32_t)pkt->id, id);
                size_t frame_size = pkt->payload_size;

                // Verify video frame header.
                VideoFrame frame = {0};
                memcpy(&frame, pkt->data, sizeof(frame));
                EXPECT_EQ(frame.width, 1920);
                EXPECT_EQ(frame.height, 1080);
                EXPECT_EQ(frame.codec_type, CODEC_TYPE_H264);
                if (frame.frame_type == VIDEO_FRAME_TYPE_INTRA) {
                    // This was a reset, some packets have been skipped.
                    test->dropped_frames += (frame.frame_id - id);
                    id = frame.frame_id;
                } else {
                    EXPECT_EQ(frame.frame_type, VIDEO_FRAME_TYPE_NORMAL);
                    EXPECT_EQ(frame.frame_id, id);
                }
                for (size_t i = sizeof(frame); i < frame_size; i++) {
                    EXPECT_EQ(pkt->data[i], i & 0xff);
                }

                free_packet(&client, pkt);
                LOG_INFO("Client received frame %u.", id);
                received = true;
            }
        } while (!received);
    }

    // Signal that the client has received everything.
    atomic_store(&test->client_finished, 1);

    // Disconnect and destroy the socket.
    destroy_socket_context(&client);

    return 0;
}

static bool test_network_packet_handler(void *user, FakePacket *pkt) {
    UDPProtocolTestSetup *test = (UDPProtocolTestSetup *)user;
    const TestNetworkProperties *prop = test->network_properties;
    bool lost = false;

    // The server address is fixed, while the client address is chosen
    // by the emphemeral address generation.  Match the server IP to
    // find which direction this packet is going in.
    const struct sockaddr_in *dst = (const struct sockaddr_in *)&pkt->dst_addr.addr;
    int direction =
        (dst->sin_addr.s_addr == test->server_ip && dst->sin_port == htons(test->server_port));

    // A random number based on the packet ID and size, to avoid the
    // test having any dependence on timing or ordering.
    uint32_t random = (test->random_seed ^ pkt->id ^ (uint32_t)pkt->len) * 987654319 + 123456791;

    if (prop->random_loss_ppm) {
        if (random % 1000000 < prop->random_loss_ppm) lost = true;
    }

    if (prop->correlated_loss_good_ppm || prop->correlated_loss_bad_ppm) {
        if (test->last_packet_lost[direction]) {
            if (random % 1000000 < prop->correlated_loss_bad_ppm) lost = true;
        } else {
            if (random % 1000000 < prop->correlated_loss_good_ppm) lost = true;
        }
    }

    uint64_t buffer_delay_us = 0;
    if (prop->bitrate_limit[direction]) {
        uint32_t bits_newly_available =
            get_timer(&test->last_packet_time[direction]) * prop->bitrate_limit[direction];
        if (bits_newly_available >= test->buffered_bits[direction]) {
            // Packet can pass through immediately.
            buffer_delay_us = 0;
            test->buffered_bits[direction] = 8 * pkt->len;
        } else if (prop->max_buffering[direction] == 0) {
            // There is no buffering, so this packet is dropped.
            lost = true;
            test->buffered_bits[direction] -= bits_newly_available;
        } else {
            test->buffered_bits[direction] -= bits_newly_available;
            if (test->buffered_bits[direction] + 8 * pkt->len > prop->max_buffering[direction]) {
                // Buffer is full, we have to drop this packet.
                lost = true;
            } else {
                // We have space for this packet, but buffering still
                // adds delay.
                buffer_delay_us = (UINT64_C(1000000) * test->buffered_bits[direction] /
                                   prop->bitrate_limit[direction]);
                test->buffered_bits[direction] += 8 * pkt->len;
            }
        }
        start_timer(&test->last_packet_time[direction]);
    }

    if (!lost) {
        uint64_t transit_time_us = prop->delay_ms * 1000 + buffer_delay_us;
        if (prop->delay_jitter_ms) {
            transit_time_us += random % (prop->delay_jitter_ms * 1000);
        }
        LOG_DEBUG("Packet %d will spend %" PRIu64 "us in transit.", pkt->id, transit_time_us);
        pkt->arrival_time = pkt->send_time + transit_time_us;
    } else {
        LOG_DEBUG("Packet %d is lost.", pkt->id);
    }

    test->last_packet_lost[direction] = lost;
    return lost;
}

TEST_F(NetworkTest, UDPProtocolTest) {
    whist_init_networking();

    // Hack globals required by the UDP protocol.
    output_width = 1920;
    output_height = 1080;
    network_algo_set_dpi(96);

    UDPProtocolTestSetup test = {0};

    test.server_ip = 0x0a0b0c0d;
    test.server_ip_string = "10.11.12.13";
    test.server_port = 21534;
    memset(test.aes_key, 0, sizeof(test.aes_key));
    test.receive_timeout_ms = 1;
    test.connection_timeout_ms = 1000;

    // 2 seconds of video at 60fps / 4.8Mbps.
    test.frame_count = 120;
    test.frame_rate = 60;

    atomic_init(&test.client_connected, 0);
    atomic_init(&test.client_finished, 0);

    for (unsigned int t = 0; t < ARRAY_LENGTH(test_networks); t++) {
        const TestNetworkProperties *prop = &test_networks[t];
        LOG_INFO("Test %u: %s.", t, prop->description);

        test.network_properties = prop;

        atomic_store(&test.client_connected, 0);
        atomic_store(&test.client_finished, 0);

        test.random_seed = t;
        test.dropped_frames = 0;
        test.last_packet_lost[0] = test.last_packet_lost[1] = false;
        test.buffered_bits[0] = test.buffered_bits[1] = 0;

        FakeNetworkContext *net = fake_network_create();

        socket_set_default_implementation(SOCKET_TYPE_UDP, &socket_implementation_fake_udp, net);

        fake_network_set_packet_handler(net, &test_network_packet_handler, &test);

        WhistThread server_thread, client_thread;

        server_thread = whist_create_thread(&test_udp_protocol_server_thread,
                                            "UDP Protocol Test Server Thread", &test);
        EXPECT_TRUE(server_thread);

        client_thread = whist_create_thread(&test_udp_protocol_client_thread,
                                            "UDP Protocol Test Client Thread", &test);
        EXPECT_TRUE(client_thread);

        whist_wait_thread(server_thread, NULL);
        whist_wait_thread(client_thread, NULL);

        fake_network_destroy(net);

        int drop_percent = 100 * test.dropped_frames / test.frame_count;
        LOG_INFO("Test %u: %d%% of frames dropped.", t, drop_percent);

        EXPECT_LE(drop_percent, prop->max_drop_percent);
    }
}
