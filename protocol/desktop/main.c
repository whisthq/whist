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

#pragma comment (lib, "ws2_32.lib")

#define BUFLEN 512
#define NPACK 10
#define PORT 48800
 
// This is our server's IP address. In case you're wondering, this one is an RFC 5737 address.
#define SRV_IP "34.200.170.47"
 
// A small struct to hold a UDP endpoint. We'll use this to hold each peer's endpoint.
struct client
{
    int host;
    short port;
};
 
// Just a function to kill the program when something goes wrong.
void diep(char *s)
{
    perror(s);
    exit(1);
}

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
    char *message = "Hello from the client!";
    while(1) {
        if (sendto(context.s, message, strlen(message), 0, (struct sockaddr*)(&context.si_other), slen)==-1)
            diep("sendto()");
    }
}

int CreateUDPSendContext(struct context *context, char* origin, char* destination) {
    struct client buf;
    int slen=sizeof(context->si_other), n = 0;

    if(!(strcmp(origin, "C") == 0 || strcmp(origin, "S") == 0)) {
        printf("Invalid origin parameter. Specify 'S' or 'C'.");
        return -1;
    }

    context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (context->s == INVALID_SOCKET || context->s < 0) { // Windows & Unix cases
        printf("Could not create UDP socket\n");
        return -1;
    }
 
    // The server's endpoint data
    memset((char *) &context->si_other, 0, sizeof(context->si_other));
    context->si_other.sin_family = AF_INET;
    context->si_other.sin_port = htons(PORT);
    context->si_other.sin_addr.s_addr = inet_addr(SRV_IP);
 
    // Send a simple datagram to the server to let it know of our public UDP endpoint.
    // Not only the server, but other clients will send their data through this endpoint.
    // The datagram payload is irrelevant, but if we wanted to support multiple
    // clients behind the same NAT, we'd send our won private UDP endpoint information
    // as well.


    strcat(destination, origin);
    printf("Payload is %s\n", destination);

    if (sendto(context->s, destination, strlen(destination), 0, (struct sockaddr*)(&context->si_other), slen)==-1) {
        printf("Sent failed");
    } else {
        printf("Sent message to STUN server\n");
    }

    // Right here, our NAT should have a session entry between our host and the server.
    // We can only hope our NAT maps the same public endpoint (both host and port) when we
    // send datagrams to other clients using our same private endpoint.
    int punched = 0;
    while (punched == 0)
    {
        // Receive data from the socket. Notice that we use the same socket for server and
        // peer communications. We discriminate by using the remote host endpoint data, but
        // remember that IP addresses are easily spoofed (actually, that's what the NAT is
        // doing), so remember to do some kind of validation in here.
        if (recvfrom(context->s, &buf, sizeof(buf), 0, (struct sockaddr*)(&context->si_other), &slen)==-1) {
            printf("Did not receive anything from STUN server \n");
            return -1;
        } else {
            printf("Received packet from STUN server at %s:%d\n", inet_ntoa(context->si_other.sin_addr), ntohs(context->si_other.sin_port));
            punched = 1;
        }
    }

    context->si_other.sin_addr.s_addr = buf.host;
    context->si_other.sin_port = buf.port;
    return 0;
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
    int i, f, j, k, slen=sizeof(si_other);
    int n = 0;
    SOCKET s;
    HANDLE ThreadHandle;
 
    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT); // This is not really necessary, we can also use 0 (any port)
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    struct context context = {0};
    context.s = s;
    context.si_other = si_other;

    if(CreateUDPSendContext(&context, "C", "40.117.57.45") < 0) {
        exit(1);
    }

    ThreadHandle = (HANDLE)_beginthreadex(NULL, 0, &SendStream, (void *) &context, 0, NULL);

    printf("Now paired with %s:%d\n", inet_ntoa(context.si_other.sin_addr), ntohs(context.si_other.sin_port));

    while (1)
    {
        char recv_buf[1000];
        int recv_size;

        if ((recv_size = recvfrom(context.s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&context.si_other), &slen)) < 0) {
            printf("Packet not received \n");
        } else {
            printf("Received message: %s\n", recv_buf);
        }
    }
 
    // Actually, we never reach this point...
    closesocket(s);
    return 0;
}