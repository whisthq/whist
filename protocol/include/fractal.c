// unportable Windows warnings, need to be at the very top
#if defined(_WIN32)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdint.h>
#include <stdbool.h>

#include "fractal.h" // header file for this protocol, includes winsock
#include "aes.h"

// windows socklen
#if defined(_WIN32)
	#define socklen_t int
#endif

void runcmd( const char* cmdline )
{
#ifdef _WIN32
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof( si ) );
	si.cb = sizeof( si );
	ZeroMemory( &pi, sizeof( pi ) );

	char cmd_buf[1000];

	if( strlen( (const char*)cmdline ) + 1 > sizeof( cmd_buf ) )
	{
		mprintf( "runcmd cmdline too long!\n" );
		return;
	}

	memcpy( cmd_buf, cmdline, strlen( (const char*)cmdline ) + 1 );

	if( CreateProcessA( NULL, (LPSTR)cmd_buf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi ) )
	{
		WaitForSingleObject( pi.hProcess, INFINITE );
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}
#else
	//TODO: rewrite the entire function for Unix
#endif
}

int GetFmsgSize(struct FractalClientMessage* fmsg) {
	if( fmsg->type == MESSAGE_KEYBOARD_STATE )
	{
		return sizeof( *fmsg );
	} else if( fmsg->type == CMESSAGE_CLIPBOARD ) {
		return sizeof( *fmsg ) + fmsg->clipboard.size;
	} else {
		return sizeof(fmsg->type) + 40;
	}
}

int GetLastNetworkError() {
#if defined(_WIN32)
  return WSAGetLastError();
#else
  return errno;
#endif
}

int get_native_screen_width() {
  static int width = -1;
  if (width == -1) {
    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);
    width = DM.w;
  }
  return width;
}

int get_native_screen_height() {
  static int height = -1;
  if (height == -1) {
    SDL_DisplayMode DM;
    SDL_GetCurrentDisplayMode(0, &DM);
    height = DM.h;
  }
  return height;
}

void set_timeout(SOCKET s, int timeout_ms) {
	if (timeout_ms < 0) {
		unsigned long mode = 0;
#if defined(_WIN32)
		ioctlsocket(s, FIONBIO, &mode);
#else
		ioctl(s, FIONBIO, &mode);
#endif
	} else if (timeout_ms == 0) {
		unsigned long mode = 1;
#if defined(_WIN32)
		ioctlsocket(s, FIONBIO, &mode);
#else
		ioctl(s, FIONBIO, &mode);
#endif
	} else {
#if defined(_WIN32)
		int read_timeout = timeout_ms;
#else
		struct timeval read_timeout;
		read_timeout.tv_sec = timeout_ms / 1000;
		read_timeout.tv_usec = (timeout_ms % 1000) * 1000;
#endif
		if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *) &read_timeout, sizeof(read_timeout)) < 0) {
			mprintf("Failed to set timeout\n");
			return;
		}
	}
}

typedef struct
{
	unsigned int ip;
	unsigned short private_port;
	unsigned short public_port;
} stun_entry_t;

typedef enum stun_request_type
{
	ASK_INFO,
	POST_INFO
} stun_request_type_t;

typedef struct
{
	stun_request_type_t type;
	stun_entry_t entry;
} stun_request_t;

#define STUN_IP "52.22.246.213"
#define STUN_PORT 48800

int CreateTCPContext( struct SocketContext* context, char* origin, char* destination, int port, int recvfrom_timeout_ms, int stun_timeout_ms )
{
	context->is_tcp = true;
	
	// Init stun_addr
	struct sockaddr_in stun_addr;
	stun_addr.sin_family = AF_INET;
	stun_addr.sin_addr.s_addr = inet_addr( STUN_IP );
	stun_addr.sin_port = htons( STUN_PORT );
	int opt;

	// Function parameter checking
	if( !(strcmp( origin, "C" ) == 0 || strcmp( origin, "S" ) == 0) )
	{
		mprintf( "Invalid origin parameter. Specify 'S' or 'C'." );
		return -1;
	}

	// Create TCP socket
	context->s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( context->s <= 0 )
	{ // Windows & Unix cases
		mprintf( "Could not create UDP socket %d\n", GetLastNetworkError() );
		return -1;
	}

	// Tell the STUN to use TCP
#if USING_STUN
	SOCKET udp_s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	sendto( udp_s, NULL, 0, 0, (struct sockaddr*)&stun_addr, sizeof( stun_addr ) );
	closesocket( udp_s );
#endif

	if( strcmp( origin, "C" ) == 0 )
	{
		// Client connection protocol
		context->is_server = false;

#if USING_STUN
		// Reuse addr
		opt = 1;
		if( setsockopt( context->s, SOL_SOCKET, SO_REUSEADDR,
			(const char*)&opt, sizeof( opt ) ) < 0 )
		{
			mprintf( "Could not setsockopt SO_REUSEADDR\n" );
			return -1;
		}

		struct sockaddr_in origin_addr;

		// Connect to TCP server
		if( connect( context->s, (struct sockaddr*)(&stun_addr), sizeof( stun_addr ) ) < 0 )
		{
			mprintf( "Could not connect over TCP to server %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		int slen = sizeof( origin_addr );
		if( getsockname( context->s, (struct sockaddr*)&origin_addr, &slen ) < 0 )
		{
			mprintf( "Could not get sock name\n" );
			closesocket( context->s );
			return -1;
		}

		// Make STUN request
		stun_request_t stun_request;
		stun_request.type = ASK_INFO;
		stun_request.entry.ip = inet_addr( destination );
		stun_request.entry.public_port = htons( (unsigned short)port );

		if( sendp( context, &stun_request, sizeof( stun_request ) ) < 0 )
		{
			mprintf( "Could not send STUN request to connected STUN server!\n" );
			closesocket( context->s );
			return -1;
		}

		// Receive STUN response
		clock t;
		StartTimer( &t );

		int recv_size = 0;
		stun_entry_t entry;

		while( recv_size < (int) sizeof( entry ) && GetTimer( t ) < stun_timeout_ms )
		{
			int single_recv_size;
			if( (single_recv_size = recvp( context, ((char*)&entry) + recv_size, max(0, (int) sizeof( entry ) - recv_size) )) < 0 )
			{
				mprintf( "Did not receive STUN response %d\n", GetLastNetworkError() );
				closesocket( context->s );
				return -1;
			}
			recv_size += single_recv_size;
		}

		if( recv_size != sizeof( entry ) )
		{
			mprintf( "TCP STUN Response packet of wrong size! %d\n", recv_size );
			closesocket( context->s );
			return -1;
		}

		// Print STUN response
		struct in_addr a;
		a.s_addr = entry.ip;
		mprintf( "TCP STUN responded that the TCP server is located at %s:%d\n", inet_ntoa( a ), ntohs( entry.private_port ) );

		closesocket( context->s );

		context->s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if( context->s <= 0 )
		{ // Windows & Unix cases
			mprintf( "Could not create UDP socket %d\n", GetLastNetworkError() );
			return -1;
		}

		// Reuse addr
		opt = 1;
		if( setsockopt( context->s, SOL_SOCKET, SO_REUSEADDR,
			(const char*)&opt, sizeof( opt ) ) < 0 )
		{
			mprintf( "Could not setsockopt SO_REUSEADDR\n" );
			return -1;
		}

		// Bind to port
		if( bind( context->s, (struct sockaddr*)(&origin_addr), sizeof( origin_addr ) ) < 0 )
		{
			mprintf( "Failed to bind to port! %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		context->addr.sin_family = AF_INET;
		context->addr.sin_addr.s_addr = entry.ip;
		context->addr.sin_port = entry.private_port;
#else
		context->addr.sin_family = AF_INET;
		context->addr.sin_addr.s_addr = inet_addr( destination );
		context->addr.sin_port = htons( (unsigned short) port );
#endif

		mprintf( "Connecting to server...\n" );

		SDL_Delay( 200 );

		// Connect to TCP server
		if( connect( context->s, (struct sockaddr*)(&context->addr), sizeof( context->addr ) ) < 0 )
		{
			mprintf( "Could not connect over TCP to server %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		mprintf( "Connected on %s:%d!\n", destination, port );

		set_timeout( context->s, recvfrom_timeout_ms );
	} else
	{
		// Server connection protocol
		context->is_server = true;

		// Reuse addr
		opt = 1;
		if( setsockopt( context->s, SOL_SOCKET, SO_REUSEADDR,
			(const char*)&opt, sizeof( opt ) ) < 0 )
		{
			mprintf( "Could not setsockopt SO_REUSEADDR\n" );
			return -1;
		}

		struct sockaddr_in origin_addr;

#if USING_STUN
		// Connect over TCP to STUN
		mprintf( "Connecting to STUN TCP...\n" );
		if( connect( context->s, (struct sockaddr*)(&stun_addr), sizeof( stun_addr ) ) < 0 )
		{
			mprintf( "Could not connect over TCP to server %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		int slen = sizeof(origin_addr);
		if( getsockname( context->s, (struct sockaddr*)&origin_addr, &slen ) < 0 )
		{
			mprintf( "Could not get sock name\n" );
			closesocket( context->s );
			return -1;
		}

		// Send STUN request
		stun_request_t stun_request;
		stun_request.type = POST_INFO;
		stun_request.entry.public_port = htons( (unsigned short)port );

		if( sendp( context, &stun_request, sizeof( stun_request ) ) < 0 )
		{
			mprintf( "Could not send STUN request to connected STUN server!\n" );
			closesocket( context->s );
			return -1;
		}

		// Receive STUN response
		clock t;
		StartTimer( &t );

		int recv_size = 0;
		stun_entry_t entry;

		while( recv_size < (int) sizeof(entry) && GetTimer(t) < stun_timeout_ms )
		{
			int single_recv_size;
			if( (single_recv_size = recvp( context, ((char*)&entry) + recv_size, max( 0, (int) sizeof( entry ) - recv_size ) )) < 0 )
			{
				mprintf( "Did not receive STUN response %d\n", GetLastNetworkError() );
				closesocket( context->s );
				return -1;
			}
			recv_size += single_recv_size;
		}

		if( recv_size != sizeof( entry ) )
		{
			mprintf( "TCP STUN Response packet of wrong size! %d\n", recv_size );
			closesocket( context->s );
			return -1;
		}

		// Print STUN response
		struct sockaddr_in client_addr;
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = entry.ip;
		client_addr.sin_port = entry.private_port;
		mprintf( "TCP STUN notified of desired request from %s:%d\n", inet_ntoa( client_addr.sin_addr ), ntohs( client_addr.sin_port ) );

		/*
		if( connect( context->s, (struct sockaddr*)(&client_addr), sizeof( client_addr ) ) < 0 )
		{
			mprintf( "Could not connect! (Expected)\n" );
		}
		*/

		closesocket( context->s );

		// Create TCP socket
		context->s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
		if( context->s <= 0 )
		{ // Windows & Unix cases
			mprintf( "Could not create UDP socket %d\n", GetLastNetworkError() );
			return -1;
		}

		opt = 1;
		if( setsockopt( context->s, SOL_SOCKET, SO_REUSEADDR,
			(const char*)&opt, sizeof( opt ) ) < 0 )
		{
			mprintf( "Could not setsockopt SO_REUSEADDR\n" );
			return -1;
		}

		// Bind to port
		if( bind( context->s, (struct sockaddr*)(&origin_addr), sizeof( origin_addr ) ) < 0 )
		{
			mprintf( "Failed to bind to port! %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		mprintf( "WAIT\n" );

		if( connect( context->s, (struct sockaddr*)&client_addr, sizeof( client_addr ) ) < 0 )
		{
			mprintf( "Failed to connect! %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		context->addr = client_addr;
#else
		origin_addr.sin_family = AF_INET;
		origin_addr.sin_addr.s_addr = htonl( INADDR_ANY );
		origin_addr.sin_port = htons( (unsigned short)port );

		// Bind to port
		if( bind( context->s, (struct sockaddr*)(&origin_addr), sizeof( origin_addr ) ) < 0 )
		{
			mprintf( "Failed to bind to port! %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		// Set listen queue
		set_timeout( context->s, stun_timeout_ms );
		if( listen( context->s, 3 ) < 0 )
		{
			mprintf( "Could not listen(2)! %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		fd_set fd_read, fd_write;
		FD_ZERO( &fd_read );
		FD_ZERO( &fd_write );
		FD_SET( context->s, &fd_read );
		FD_SET( context->s, &fd_write );

		struct timeval tv;
		tv.tv_sec = stun_timeout_ms / 1000;
		tv.tv_usec = (stun_timeout_ms % 1000) * 1000;

		if( select( 0, &fd_read, &fd_write, NULL, stun_timeout_ms > 0 ? &tv : NULL ) < 0 )
		{
			mprintf( "Could not select!\n" );
			closesocket( context->s );
			return -1;
		}

		// Accept connection from client
		socklen_t slen = sizeof( context->addr );
		SOCKET new_socket;
		if( (new_socket = accept( context->s, (struct sockaddr*)(&context->addr), &slen)) < 0 )
		{
			mprintf( "Did not receive response from client! %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		closesocket( context->s );
		context->s = new_socket;
#endif

		mprintf( "Client received at %s:%d!\n", inet_ntoa( context->addr.sin_addr ), ntohs( context->addr.sin_port ) );

		set_timeout( context->s, recvfrom_timeout_ms );
	}

	return 0;
}

int CreateUDPContext(struct SocketContext* context, char* origin, char* destination, int port, int recvfrom_timeout_ms, int stun_timeout_ms) {
	context->is_tcp = false;

	// Function parameter checking
	if (!(strcmp(origin, "C") == 0 || strcmp(origin, "S") == 0)) {
		mprintf("Invalid origin parameter. Specify 'S' or 'C'.");
		return -1;
	}

	// Create UDP socket
	context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (context->s <= 0) { // Windows & Unix cases
		mprintf("Could not create UDP socket %d\n", GetLastNetworkError());
		return -1;
	}
	set_timeout( context->s, stun_timeout_ms );

	if (strcmp(origin, "C") == 0) {
		// Client connection protocol
		context->is_server = false;

#if USING_STUN
		struct sockaddr_in stun_addr;
		stun_addr.sin_family = AF_INET;
		stun_addr.sin_addr.s_addr = inet_addr( STUN_IP );
		stun_addr.sin_port = htons( STUN_PORT );

		stun_request_t stun_request;
		stun_request.type = ASK_INFO;
		stun_request.entry.ip = inet_addr( destination );
		stun_request.entry.public_port = htons( (unsigned short)port );

		mprintf( "Sending info request to STUN...\n" );
		if( sendto( context->s, &stun_request, sizeof( stun_request ), 0, (struct sockaddr *)&stun_addr, sizeof( stun_addr ) ) < 0 )
		{
			mprintf( "Could not send message to STUN %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		stun_entry_t entry;
		int recv_size;
		if( (recv_size = recvp( context, &entry, sizeof( entry ) )) < 0 )
		{
			mprintf( "Could not receive message from STUN %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		if( recv_size != sizeof(entry) )
		{
			mprintf( "STUN Response of incorrect size!\n" );
			closesocket( context->s );
			return -1;
		} else if (entry.ip != stun_request.entry.ip || entry.public_port != stun_request.entry.public_port)
		{
			mprintf( "STUN Response IP and/or Public Port is incorrect!" );
			closesocket( context->s );
			return -1;
		} else
		{
			mprintf( "Received STUN response! Public %d is mapped to private %d\n", ntohs( (unsigned short)entry.public_port ), ntohs( (unsigned short)entry.private_port ) );
			context->addr.sin_family = AF_INET;
			context->addr.sin_addr.s_addr = entry.ip;
			context->addr.sin_port = entry.private_port;
		}
#else
		context->addr.sin_family = AF_INET;
		context->addr.sin_addr.s_addr = inet_addr( destination );
		context->addr.sin_port = htons( (unsigned short)port );
#endif

		mprintf("Connecting to server...\n");

		SDL_Delay( 150 );

		// Open up the port
		if (sendp(context, NULL, 0) < 0) {
			mprintf("Could not send message to server %d\n", GetLastNetworkError());
			closesocket(context->s);
			return -1;
		}

		SDL_Delay( 150 );

		// Send acknowledgement
		if( sendp( context, NULL, 0 ) < 0 )
		{
			mprintf( "Could not send message to server %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		// Receive server's acknowledgement of connection
		socklen_t slen = sizeof(context->addr);
		if (recvfrom(context->s, NULL, 0, 0, (struct sockaddr *) &context->addr, &slen) < 0 && recvfrom( context->s, NULL, 0, 0, (struct sockaddr *)&context->addr, &slen ) < 0 ) {
			mprintf("Did not receive response from server! %d\n", GetLastNetworkError());
			closesocket(context->s);
			return -1;
		}

		mprintf( "Connected to server on %s:%d! (Private %d)\n", inet_ntoa(context->addr.sin_addr), port, ntohs(context->addr.sin_port) );
	}
	else {
		// Server connection protocol
		context->is_server = true;

#if USING_STUN
		// Tell the STUN to log our requested virtual port
		struct sockaddr_in stun_addr;
		stun_addr.sin_family = AF_INET;
		stun_addr.sin_addr.s_addr = inet_addr( STUN_IP );
		stun_addr.sin_port = htons( STUN_PORT );

		stun_request_t stun_request;
		stun_request.type = POST_INFO;
		stun_request.entry.public_port = htons( (unsigned short)port );

		mprintf( "Sending stun entry to STUN...\n" );
		if( sendto( context->s, &stun_request, sizeof( stun_request ), 0, (struct sockaddr *) &stun_addr, sizeof( stun_addr ) ) < 0 )
		{
			mprintf( "Could not send message to STUN %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}
#else
		// Bind the server port to the advertized public port
		struct sockaddr_in origin_addr;
		origin_addr.sin_family = AF_INET;
		origin_addr.sin_addr.s_addr = htonl( INADDR_ANY );
		origin_addr.sin_port = htons( (unsigned short)port );

		if (bind(context->s, (struct sockaddr*)(&origin_addr), sizeof(origin_addr)) < 0) {
			mprintf("Failed to bind to port! %d\n", GetLastNetworkError());
			closesocket(context->s);
			return -1;
		}
#endif

		mprintf("Waiting for client to connect to %s:%d...\n", "localhost", port);

		// Receive client's connection attempt
#if USING_STUN
		// Update the STUN every 100ms
		set_timeout(context->s, 100);

		// But keep track of time to compare against stun_timeout_ms
		clock recv_timer;
		StartTimer( &recv_timer );
#endif

		socklen_t slen = sizeof(context->addr);
		stun_entry_t entry;
		int recv_size;
		while ((recv_size = recvfrom(context->s, &entry, sizeof( entry ), 0, (struct sockaddr*)(&context->addr), &slen)) < 0) {
#if USING_STUN
			// If we haven't spent too much time waiting, and our previous 100ms poll failed, then send another STUN update
			if( GetTimer( recv_timer ) * 1000 < stun_timeout_ms && (GetLastNetworkError() == ETIMEDOUT || GetLastNetworkError() == EAGAIN) )
			{
				if( sendto( context->s, &stun_request, sizeof( stun_request ), 0, (struct sockaddr *) &stun_addr, sizeof( stun_addr ) ) < 0 )
				{
					mprintf( "Could not send message to STUN %d\n", GetLastNetworkError() );
					closesocket( context->s );
					return -1;
				}
				continue;
			}
#endif
			mprintf("Did not receive response from client! %d\n", GetLastNetworkError());
			closesocket(context->s);
			return -1;
		}

		set_timeout( context->s, 350 );

#if USING_STUN
		if( recv_size != sizeof( entry ) )
		{
			mprintf( "STUN response was not the size of an entry!\n" );
			closesocket( context->s );
			return -1;
		}

		// Setup addr to open up port
		context->addr.sin_family = AF_INET;
		context->addr.sin_addr.s_addr = entry.ip;
		context->addr.sin_port = entry.private_port;

		mprintf( "Received STUN response, client connection desired from %s:%d\n", inet_ntoa( context->addr.sin_addr ), ntohs( context->addr.sin_port ) );

		SDL_Delay( 150 );

		// Open up port to receive message
		if( sendp( context, NULL, 0 ) < 0 )
		{
			mprintf( "Could not open up port!\n" );
			closesocket( context->s );
			return -1;
		}

		SDL_Delay( 150 );

		// Send acknowledgement
		if( sendp( context, NULL, 0 ) < 0 )
		{
			mprintf( "Could not open up port!\n" );
			closesocket( context->s );
			return -1;
		}

		// Wait for client to connect
		if( recvfrom( context->s, NULL, 0, 0, (struct sockaddr*)(&context->addr), &slen ) < 0 && recvfrom( context->s, NULL, 0, 0, (struct sockaddr*)(&context->addr), &slen ) < 0 )
		{
			mprintf( "Did not receive client confirmation!\n" );
			closesocket( context->s );
			return -1;
		}

		// Check that confirmation matches STUN's claimed client
		if( context->addr.sin_addr.s_addr != entry.ip || context->addr.sin_port != entry.private_port )
		{
			mprintf( "Connection did not match STUN's claimed client, got %s:%d instead\n", inet_ntoa( context->addr.sin_addr ), ntohs( context->addr.sin_port ) );
			context->addr.sin_addr.s_addr = entry.ip;
			context->addr.sin_port = entry.private_port;
			mprintf( "Should have been %s:%d!\n", inet_ntoa( context->addr.sin_addr ), ntohs( context->addr.sin_port ) );
			closesocket( context->s );
			return -1;
		}
#else
		// Send acknowledgement of connection
		sendp( context, NULL, 0 );
#endif

		mprintf("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr), ntohs(context->addr.sin_port));
	}

	set_timeout( context->s, recvfrom_timeout_ms );

	return 0;
}

int recvp(struct SocketContext* context, void* buf, int len) {
	return recv( context->s, buf, len, 0 );
}

int sendp(struct SocketContext* context, void* buf, int len) {
	return sendto( context->s, buf, len, 0, (struct sockaddr*)(&context->addr), sizeof( context->addr ) );
}

#define LARGEST_TCP_PACKET 10000000
static int reading_packet_len = 0;
static char reading_packet_buffer[LARGEST_TCP_PACKET];

static char decrypted_packet_buffer[LARGEST_TCP_PACKET];

void ClearReadingTCP()
{
	reading_packet_len = 0;
}

void* TryReadingTCPPacket( struct SocketContext* context )
{
	if( !context->is_tcp )
	{
		mprintf( "TryReadingTCPPacket received a context that is NOT TCP!\n" );
		return NULL;
	}

#define TCP_SEGMENT_SIZE 1024

	int len;

	do
	{
		// Try to fill up the buffer, in chunks of TCP_SEGMENT_SIZE, but don't overflow LARGEST_TCP_PACKET
		len = recvp( context, reading_packet_buffer + reading_packet_len, min(TCP_SEGMENT_SIZE, LARGEST_TCP_PACKET - reading_packet_len) );

		if( len < 0 )
		{
			int err = GetLastNetworkError();
			if( err == ETIMEDOUT || err == EAGAIN )
			{
			} else
			{
				//mprintf( "Error %d\n", err );
			}
		} else
		{
			reading_packet_len += len;
		}

		// If the previous recvp was maxed out, then try pulling some more from recvp
	} while( len == TCP_SEGMENT_SIZE );

	if( (unsigned long) reading_packet_len > sizeof( int ) )
	{
		// The amount of data bytes read (actual len), and the amount of bytes we're looking for (target len), respectively
		int actual_len = reading_packet_len - sizeof( int );
		int target_len = *((int*)reading_packet_buffer);

		// If the target len is valid, and actual len > target len, then we're good to go
		if( target_len >= 0 && target_len <= LARGEST_TCP_PACKET && actual_len >= target_len )
		{
			// Decrypt it
			int decrypted_len = decrypt_packet_n( (struct RTPPacket *) (reading_packet_buffer + sizeof( int )), target_len, (struct RTPPacket *) decrypted_packet_buffer, LARGEST_TCP_PACKET, (unsigned char *) PRIVATE_KEY );

			// Move the rest of the read bytes to the beginning of the buffer to continue
			int start_next_bytes = sizeof(int) + target_len;
			for( unsigned long i = start_next_bytes; i < sizeof(int) + actual_len; i++ )
			{
				reading_packet_buffer[i - start_next_bytes] = reading_packet_buffer[i];
			}
			reading_packet_len = actual_len - target_len;

			if( decrypted_len < 0 )
			{
				mprintf( "Could not decrypt TCP message\n" );
				return NULL;
			} else
			{
				// Return the decrypted packet
				return decrypted_packet_buffer;
			}

		}
	}

	return NULL;
}

// Multithreaded printf Semaphores and Mutexes
static volatile SDL_sem* multithreadedprintf_semaphore;
static volatile SDL_mutex* multithreadedprintf_mutex;

// Multithreaded printf queue
#define MPRINTF_QUEUE_SIZE 1000
#define MPRINTF_BUF_SIZE 1000
typedef struct mprintf_queue_item {
	bool log;
	char buf[MPRINTF_BUF_SIZE];
} mprintf_queue_item;
static volatile mprintf_queue_item mprintf_queue[MPRINTF_QUEUE_SIZE];
static volatile mprintf_queue_item mprintf_queue_cache[MPRINTF_QUEUE_SIZE];
static volatile int mprintf_queue_index = 0;
static volatile int mprintf_queue_size = 0;

// Multithreaded printf global variables
SDL_Thread* mprintf_thread = NULL;
static volatile bool run_multithreaded_printf;
int MultiThreadedPrintf(void* opaque);
void real_mprintf(bool log, const char* fmtStr, va_list args);
clock mprintf_timer;
FILE* mprintf_log_file = NULL;

void initMultiThreadedPrintf(bool use_logging) {
	if (use_logging) {
		mprintf_log_file = fopen("log.txt", "ab");
	}

	run_multithreaded_printf = true;
	multithreadedprintf_mutex = SDL_CreateMutex();
	multithreadedprintf_semaphore = SDL_CreateSemaphore(0);
	mprintf_thread = SDL_CreateThread((SDL_ThreadFunction) MultiThreadedPrintf, "MultiThreadedPrintf", NULL);
	StartTimer(&mprintf_timer);
}

void destroyMultiThreadedPrintf() {
	// Wait for any remaining printfs to execute
	SDL_Delay(50);

	run_multithreaded_printf = false;
	SDL_SemPost((SDL_sem *) multithreadedprintf_semaphore);

	SDL_WaitThread(mprintf_thread, NULL);
	mprintf_thread = NULL;

	if (mprintf_log_file) {
		fclose(mprintf_log_file);
	}
	mprintf_log_file = NULL;
}

int MultiThreadedPrintf(void* opaque) {
	opaque;

	while (true) {
		// Wait until signaled by printf to begin running
		SDL_SemWait((SDL_sem *) multithreadedprintf_semaphore);

		if (!run_multithreaded_printf) {
			break;
		}

		int cache_size = 0;

		// Clear the queue into the cache,
		// And then let go of the mutex so that printf can continue accumulating
		SDL_LockMutex((SDL_mutex *) multithreadedprintf_mutex);
		cache_size = mprintf_queue_size;
		for (int i = 0; i < mprintf_queue_size; i++) {
			mprintf_queue_cache[i].log = mprintf_queue[mprintf_queue_index].log;
			strcpy((char *) mprintf_queue_cache[i].buf, (const char *) mprintf_queue[mprintf_queue_index].buf);
			mprintf_queue_index++;
			mprintf_queue_index %= MPRINTF_QUEUE_SIZE;
			if (i != 0) {
				SDL_SemWait((SDL_sem *) multithreadedprintf_semaphore);
			}
		}
		mprintf_queue_size = 0;
		SDL_UnlockMutex((SDL_mutex *) multithreadedprintf_mutex);

		// Print all of the data into the cache
        //int last_printf = -1;
		for (int i = 0; i < cache_size; i++) {
			if (mprintf_log_file && mprintf_queue_cache[i].log) {
				fprintf(mprintf_log_file, "%s", mprintf_queue_cache[i].buf);
			}
			//if (i + 6 < cache_size) {
			//	printf("%s%s%s%s%s%s%s", mprintf_queue_cache[i].buf, mprintf_queue_cache[i+1].buf, mprintf_queue_cache[i+2].buf, mprintf_queue_cache[i+3].buf, mprintf_queue_cache[i+4].buf,  mprintf_queue_cache[i+5].buf,  mprintf_queue_cache[i+6].buf);
			//    last_printf = i + 6;
			//} else if (i > last_printf) {
				printf("%s", mprintf_queue_cache[i].buf);
			//}
		}
		if( mprintf_log_file )
		{
			fflush( mprintf_log_file );
		}

		// If the log file is large enough, cache it
		if( mprintf_log_file ) {
			fseek( mprintf_log_file, 0L, SEEK_END );
			int sz = ftell( mprintf_log_file );

			// If it's larger than 5MB, start a new file and store the old one
			if( sz > 5 * 1024 * 1024 )
			{
				fclose( mprintf_log_file );
#if defined(_WIN32)
				DeleteFileW( L"log_prev.txt" );
				MoveFileW( L"log.txt", L"log_prev.txt" );
				DeleteFileW( L"log.txt" );
#endif
				mprintf_log_file = fopen( "log.txt", "ab" );
			}
		}
	}
	return 0;
}

void mprintf(const char* fmtStr, ...) {
	va_list args;
	va_start(args, fmtStr);

	real_mprintf(WRITE_MPRINTF_TO_LOG, fmtStr, args);
}

void lprintf(const char* fmtStr, ...) {
	va_list args;
	va_start(args, fmtStr);
	real_mprintf(true, fmtStr, args);
}

void real_mprintf(bool log, const char* fmtStr, va_list args) {
	if (mprintf_thread == NULL) {
		printf("initMultiThreadedPrintf has not been called!\n");
		return;
	}

	SDL_LockMutex((SDL_mutex *) multithreadedprintf_mutex);
	int index = (mprintf_queue_index + mprintf_queue_size) % MPRINTF_QUEUE_SIZE;
	char* buf = NULL;
	if (mprintf_queue_size < MPRINTF_QUEUE_SIZE - 2) {
		mprintf_queue[index].log = log;
		buf = (char *) mprintf_queue[index].buf;
		snprintf(buf, MPRINTF_BUF_SIZE, "%15.4f: ", GetTimer(mprintf_timer));
		int len = (int) strlen(buf);
		vsnprintf(buf + len, MPRINTF_BUF_SIZE - len, fmtStr, args);
		mprintf_queue_size++;
	}
	else if (mprintf_queue_size == MPRINTF_QUEUE_SIZE - 2) {
		mprintf_queue[index].log = log;
		buf = (char *) mprintf_queue[index].buf;
		strcpy(buf, "Buffer maxed out!!!\n");
		mprintf_queue_size++;
	}

	if (buf != NULL) {
		buf[MPRINTF_BUF_SIZE - 5] = '.';
		buf[MPRINTF_BUF_SIZE - 4] = '.';
		buf[MPRINTF_BUF_SIZE - 3] = '.';
		buf[MPRINTF_BUF_SIZE - 2] = '\n';
		buf[MPRINTF_BUF_SIZE - 1] = '\0';
		SDL_SemPost((SDL_sem *) multithreadedprintf_semaphore);
	}
	SDL_UnlockMutex((SDL_mutex *) multithreadedprintf_mutex);

	va_end(args);
}

#if defined(_WIN32)
LARGE_INTEGER frequency;
bool set_frequency = false;
#endif

void StartTimer(clock* timer) {
	#if defined(_WIN32)
		if (!set_frequency) {
			QueryPerformanceFrequency(&frequency);
			set_frequency = true;
		}
	QueryPerformanceCounter(timer);
	#else
		// start timer
		gettimeofday(timer, NULL);
	#endif
}

double GetTimer(clock timer) {
	#if defined(_WIN32)
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);
		double ret = (double) (end.QuadPart - timer.QuadPart) / frequency.QuadPart;
	#else
		// stop timer
		struct timeval t2;
		gettimeofday(&t2, NULL);

		// compute and print the elapsed time in millisec
		double elapsedTime = (t2.tv_sec - timer.tv_sec) * 1000.0;    // sec to ms
		elapsedTime += (t2.tv_usec - timer.tv_usec) / 1000.0;        // us to ms

		//printf("elapsed time in ms is: %f\n", elapsedTime);


		// standard var to return and convert to seconds since it gets converted to ms in function call
		double ret = elapsedTime / 1000.0;
	#endif
	return ret;
}

uint32_t Hash(void* buf, size_t len)
{
	char* key = buf;

	uint64_t pre_hash = 123456789;
	while (len > 8) {
		pre_hash += *(uint64_t*)(key);
		pre_hash += (pre_hash << 32);
		pre_hash += (pre_hash >> 32);
		pre_hash += (pre_hash << 10);
		pre_hash ^= (pre_hash >> 6);
		pre_hash ^= (pre_hash << 48);
		pre_hash ^= 123456789;
		len -= 8;
		key += 8;
	}

	uint32_t hash = (uint32_t) ((pre_hash << 32) ^ pre_hash);
	for (size_t i = 0; i < len; ++i)
	{
		hash += key[i];
		hash += (hash << 10);
		hash ^= (hash >> 6);
	}
	hash += (hash << 3);
	hash ^= (hash >> 11);
	hash += (hash << 15);
	return hash;
}


#if defined(_WIN32)
#pragma warning(default: 4100)
#endif
