/*
 * This file contains the implementations of helper functions regarding sockets
 * on Linux Ubuntu, specifically UDP sockets, that are used by the hole punching
 * server to communicate with the peers.
 *
 * Hole Punching Server version: 1.0
 *
 * Last modification: 12/24/2019
 *
 * By: Philippe Noël
 *
 * Copyright Fractal Computers, Inc. 2019
**/

#include "socket.h" // header file for this file

// number of "reliable" connection attemps made before giving up, at 5% packet
// loss this is 3.125 * 10^-7 probability of success, as good as it gets
#define MAX_N_ATTEMPTS 10

#define ACK_BUFFLEN 3 // buffer for the acknowledgement packets
#define MSG_BUFFLEN 20 // buffer for the IP message packets

// @brief ensures reliable UDP sending over a socket by listening for an ack
// @details requires complementary reliable_udp_recvfrom on the receiving end
int reliable_udp_sendto(int socket_fd, unsigned char *message, int message_len, struct sockaddr_in dest_addr, socklen_t addr_size) {
  // buffer to receive the acknowledgement packet
  char ack_buff[ACK_BUFFLEN];
  int msg_sent_size, ack_recv_size, attempts = 0; // packets sizes and # of connection attempts

  // define struct to handle timeout (only on receive calls)
  #if defined(_WIN32)
    DWORD tv = 2 * 1000; // timeout in seconds * 1000 milliseconds per second
  #else
    struct timeval tv;
    tv.tv_sec = 2; // 2 seconds timeout
    tv.tv_usec = 0;
  #endif

  // set timeout on our socket
  if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv)) < 0) {
    printf("Could not set timeout on socket.\n");
    return -2;
  }

  printf("Attempting to send reliable UDP packet.\n"); // for transparency

  // we send the UDP packet and wait for an ack, if it isn't received we retry
  // up to MAX_N_ATTEMPTS times, after which we give up
  while (attempts < MAX_N_ATTEMPTS) {
    // reset memory to avoid packet clumping
    memset(&ack_buff, 0, ACK_BUFFLEN);

    // attempt to send the packet
    if ((msg_sent_size = sendto(socket_fd, message, message_len, 0, (struct sockaddr *) &dest_addr, addr_size)) < 0) {
      printf("Unable to send reliable message to the destination.\n");
      return -3;
    }

    // message sent, listen for the receipt acknowledgement
    if ((ack_recv_size = recvfrom(socket_fd, ack_buff, ACK_BUFFLEN, 0, (struct sockaddr *) &dest_addr, &addr_size)) < 0) {
      printf("Ack receipt timeout reached on attempt #%d. Resending message.\n", attempts);

      // increment attempts count
      attempts += 1;
    }
    // received ack to confirm the packet was received
    else {
      printf("Received Ack, sending successful.\n");
      break; // done sending
    }
  }

  // if we have less than the max number of attempts, that means the sending
  // succeeded, so we return a success
  if (attempts < MAX_N_ATTEMPTS) {
    // return the size of the message sent, as per the usual socket send/recv functions
    return msg_sent_size;
  }
  // if it is equal, it failed so we return an error
  else {
    return -1; // error code
  }
}


// @brief ensures reliable UDP receiving over a socket by listening for an ack
// @details requires complementary reliable_udp_sendto on the sending end
int reliable_udp_recvfrom(int socket_fd, char *msg_buff, int msg_bufflen, struct sockaddr_in dest_addr, socklen_t addr_size) {
  // vars to know packet sizes, number of connection attempts and whether an ack was sent
  int msg_recv_size, tmp_recv_size, attempts = 0;
  int ack_sent = 0, failed = 0; // failed is dummy bool to check for failed recvfrom call to avoid long if statements
  int timeout = 0; // to check in if statement

  // define struct to handle timeout (only on receive calls)
  #if defined(_WIN32)
    DWORD tv = 0; // timeout in seconds * 1000 milliseconds per second, 0 at first
  #else
    struct timeval tv;
    tv.tv_sec = 0; // 0 seconds timeout at first
    tv.tv_usec = 0;
  #endif

  // set timeout
  if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv)) < 0) {
    printf("Could not re-set timeout on socket.\n");
    return -2;
  }

  // we recv the UDP packet and send an ack, if we don't receive the  message we
  // wait for another attempt, or if our ack isn't received we try again up to
  // MAX_N_ATTEMPTS times, after which we give up
  while (attempts < MAX_N_ATTEMPTS) {
    // listen for the message
    tmp_recv_size = recvfrom(socket_fd, msg_buff, msg_bufflen, 0, (struct sockaddr *) &dest_addr, &addr_size);

    // if nothing is received before timeout (errno checks) or the socket fails
    // while without a timeout
    #if defined(_WIN32)
    if (tmp_recv_size < 0 || WSAGetLastError() == WSAEWOULDBLOCK) {
      failed = 1; // recvfrom call failed, need to enter fail if statement
    }
    #else
      if (tmp_recv_size < 0 || errno == EAGAGAIN || errno == EWOULDBLOCK) {
        failed = 1; // recvfrom call failed, need to enter fail if statement
      }
    #endif

    // recvfrom called failed
    if (failed) {
      // if it failed and we had previously sent an ack, that's because the ack
      // was received and so the server didn't send anything back, so we can
      // safely break
      // there's also the possibility ack wasn't received and the new message
      // send wasn't received, so we need an error checking in the code using
      // this function to exit the application if that happens (unlikely)
      // in this case the message wasnt received, so put something on the client
      // to exit the application because of network error
      if (ack_sent) {
        // success, let's reset the socket file descriptor to wait infinitely
        #if defined(_WIN32)
          tv = 0;
        #else
          tv.tv_sec = 0;
        #endif

        // set timeout
        if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv)) < 0) {
          printf("Could not re-set timeout on socket.\n");
          return -2;
        }
        // return success
        break; // assume ack was received since it timed out
      }
      // no ack, we just didn't receive the message
      else {
        // if timeout is 0, then it just failed so we exit, if timeout is nonzero
        // then it just timed out and we retry
        if (timeout == 0) {
          // it just failed on a non timeout socket
          printf("Error receiving on no timeout socket.\n");
          return -4;
        }
        else {
          // else we just keep retrying
          printf("Timeout reached on attempt #%d. Waiting for message resending.\n", attempts);
          // reset memory and increment attempts count
          memset(msg_buff, 0, msg_bufflen);
          attempts += 1;
        }
      }
    }
    // message received, we send an ack
    else {
      // store message received size
      msg_recv_size = tmp_recv_size;

      // now that the message is received, we need to re-set a timeout on the socket so that if no
      // further message attempt happens, it successfully exists
      // 4 seconds (double the sendto timeout) to wait to see if the connection was received
      // if nothing is received after those 4 seconds, we assume the sendto call received our
      // ack and so stopped, while if it didn't receive it it would have re-sent a packe and
      // we would have received something within those 4 second
      #if defined(_WIN32)
        tv = 4 * 1000; // timeout in seconds * 1000 milliseconds per second
      #else
        tv.tv_sec = 4; // 4 seconds timeout
      #endif

      timeout = 4; // set timeout for checking in if statement

      // set timeout
      if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv)) < 0) {
        printf("Could not re-set timeout on socket.\n");
        return -2;
      }

      // send message reception acknowledgement
      if (sendto(socket_fd, "ACK", strlen("ACK"), 0, (struct sockaddr *) &dest_addr, addr_size) < 0) {
        printf("Unable to send ack to the destination.\n");
        return -3;
      }
      // ack sent
      ack_sent = 1;
      return msg_recv_size;
    }
  }

  // if we have less than the max number of attempts, that means the sending
  // succeeded, so we return a success
  return -1;
}