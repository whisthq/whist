// UDP hole punching example, client code
// Base UDP code stolen from http://www.abc.se/~m6695/udp.html
// By Oscar Rodriguez
// This code is public domain, but you're a complete lunatic
// if you plan to use this code in any real program.
 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <windows.h>

#include "../include/fractal.h"

#pragma comment (lib, "ws2_32.lib")

#define BUFLEN 512
#define NPACK 10
#define PORT 48800
 
// This is our server's IP address. In case you're wondering, this one is an RFC 5737 address.
#define SRV_IP "34.200.170.47"
 
struct context
{
    SOCKET s;
    struct sockaddr_in si_other;
};
 
unsigned __stdcall SendStream(void *opaque) {
    struct context context = *(struct context *) opaque;
    int i, slen = sizeof(context.si_other);

    // Once again, the payload is irrelevant. Feel free to send your VoIP
    // data in here.
    char *message = "Hello from the server!";
    while(1) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.si_other), slen) < 0)
            printf("Could not send packet\n");
    }
}


int main(int argc, char* argv[])
{
    // initialize the windows socket library if this is a windows client
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Failed to initialize Winsock with error code: %d.\n", WSAGetLastError());
        return -1;
    }
    printf("Winsock initialized successfully.\n");

    struct sockaddr_in si_me, si_other;
    int slen=sizeof(si_other);
    SOCKET s;
    HANDLE ThreadHandle;
 
    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT); // This is not really necessary, we can also use 0 (any port)
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    struct context context = {0};
    context.s = s;
    context.si_other = si_other;

    if(CreateUDPSendContext(&context, "S", "") < 0) {
        exit(1);
    }

    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &SendStream, (void *) &context, 0, NULL);

    while (1)
    {
        char recv_buf[1000];
        int recv_size;

        if ((recv_size = recvfrom(context.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&context.si_me), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            printf("Received message: %s\n", recv_buf);
        }
    }
 
    // Actually, we never reach this point...
    closesocket(s);
    return 0;
}