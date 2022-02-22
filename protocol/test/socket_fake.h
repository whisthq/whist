/**
 * Copyright 2022 Whist Technologies, Inc.
 * @file socket_fake.c
 * @brief Fake sockets implementation for socket API tests.
 */

#ifndef TEST_SOCKET_FAKE_H
#define TEST_SOCKET_FAKE_H

#include "whist/core/whist.h"
#include "whist/utils/linked_list.h"

/**
 * Type for an address on the fake network.
 */
typedef struct FakeSocketAddress {
    struct sockaddr addr;
    socklen_t len;
} FakeSocketAddress;

/**
 * Type for packets sent over the fake network.
 */
typedef struct FakePacket {
    LINKED_LIST_HEADER;
    int id;

    char *data;
    size_t len;

    timestamp_us send_time;
    timestamp_us arrival_time;

    FakeSocketAddress src_addr;
    FakeSocketAddress dst_addr;
} FakePacket;

/**
 * Type for fake network packet handler functions.
 *
 * @param  user  User context supplied when setting ths handler.
 * @param  pkt   Packet to handle.
 * @return  Whether the packet should be dropped.
 */
typedef bool (*FakeNetworkPacketHandler)(void *user, FakePacket *pkt);

/**
 * Opaque type for a fake network instance.
 */
typedef struct FakeNetworkContext FakeNetworkContext;

/**
 * Create a new fake network instance for use with fake sockets.
 *
 * @return  New fake network context.
 */
FakeNetworkContext *fake_network_create(void);

/**
 * Destroy a fake network instance.
 *
 * @param net  Fake network context to destroy.
 */
void fake_network_destroy(FakeNetworkContext *net);

/**
 * Set packet handler function for a fake network instance.
 *
 * @param net      Fake network instance to set handler for.
 * @param handler  Packet handler function.
 * @param user     User argument to packet handler function.
 */
void fake_network_set_packet_handler(FakeNetworkContext *net,
                                     FakeNetworkPacketHandler handler, void *user);

#endif /* TEST_SOCKET_FAKE_H */
