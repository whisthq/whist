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
		u_long mode = 0;
#if defined(_WIN32)
		ioctlsocket(s, FIONBIO, &mode);
#else
		ioctl(s, FIONBIO, &mode);
#endif
	} else if (timeout_ms == 0) {
		u_long mode = 1;
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
		read_timeout.tv_sec = 0;
		read_timeout.tv_usec = timeout_ms * 1000;
#endif

		if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *) &read_timeout, sizeof(read_timeout)) < 0) {
			mprintf("Failed to set timeout\n");
			return;
		}
	}
}

int CreateTCPContext( struct SocketContext* context, char* origin, char* destination, int port, int recvfrom_timeout_ms, int stun_timeout_ms )
{
	context->is_tcp = true;

	// Function parameter checking
	if( !(strcmp( origin, "C" ) == 0 || strcmp( origin, "S" ) == 0) )
	{
		mprintf( "Invalid origin parameter. Specify 'S' or 'C'." );
		return -1;
	}

	// Create UDP socket
	context->s = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if( context->s <= 0 )
	{ // Windows & Unix cases
		mprintf( "Could not create UDP socket %d\n", GetLastNetworkError() );
		return -1;
	}

	if( strcmp( origin, "C" ) == 0 )
	{
		// Client connection protocol
		context->is_server = false;

		context->addr.sin_family = AF_INET;
		context->addr.sin_addr.s_addr = inet_addr( destination );
		context->addr.sin_port = htons( (u_short) port );

		mprintf( "Connecting to server...\n" );

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

		struct sockaddr_in origin_addr;
		origin_addr.sin_family = AF_INET;
		origin_addr.sin_addr.s_addr = htonl( INADDR_ANY );
		origin_addr.sin_port = htons( (u_short) port );

		// Reuse addr
		int opt = 1;
		if( setsockopt( context->s, SOL_SOCKET, SO_REUSEADDR,
						(const char *) &opt, sizeof( opt ) ) < 0 )
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

		// Set listen queue
		set_timeout( context->s, stun_timeout_ms );
		if( listen( context->s, 3 ) < 0 )
		{
			mprintf( "Could not listen(2)! %d\n", GetLastNetworkError() );
			closesocket( context->s );
			return -1;
		}

		fd_set fd;
		FD_ZERO( &fd );
		FD_SET( context->s, &fd );

		struct timeval tv;
		tv.tv_sec = stun_timeout_ms / 1000;
		tv.tv_usec = (stun_timeout_ms % 1000) * 1000;

		if( select( 0, &fd, &fd, NULL, stun_timeout_ms > 0 ? &tv : NULL ) < 0 )
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

		mprintf( "Client received at %s:%d!\n", inet_ntoa( context->addr.sin_addr ), ntohs( context->addr.sin_port ) );

		set_timeout( context->s, recvfrom_timeout_ms );
	}

	return 0;
}

typedef struct
{
	int ip;
	short private_port;
	short public_port;
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

#define STUN_IP "54.159.130.32"
#define STUN_PORT 48800

#define USING_STUN true

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
		stun_request.entry.public_port = htons( (u_short)port );

		mprintf( "Sending request to STUN...\n" );
		if( sendto( context->s, &stun_request, sizeof( stun_request ), 0, &stun_addr, sizeof( stun_addr ) ) < 0 )
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

		if( recv_size != entry )
		{
			mprintf( "STUN Response of incorrect size!\n" );
			closesocket( context->s );
			return -1;
		} else
		{
			mprintf( "Received STUN response!\n" );
			context->addr.sin_family = AF_INET;
			context->addr.sin_addr.s_addr = entry.ip;
			context->addr.sin_port = entry.private_port;
		}
#endif

		mprintf("Connecting to server...\n");

		// Send request
		if (sendp(context, NULL, 0) < 0) {
			mprintf("Could not send message to server %d\n", GetLastNetworkError());
			closesocket(context->s);
			return -1;
		}

		// Receive server's acknowledgement of connection
		set_timeout(context->s, stun_timeout_ms);
		if (recvp(context, NULL, 0) < 0) {
			mprintf("Did not receive response from server! %d\n", GetLastNetworkError());
			closesocket(context->s);
			return -1;
		}

		mprintf("Connected on %s:%d!\n", destination, port);

		set_timeout(context->s, recvfrom_timeout_ms);
	}
	else {
		// Server connection protocol
		context->is_server = true;

		struct sockaddr_in origin_addr;
		origin_addr.sin_family = AF_INET;
		origin_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		origin_addr.sin_port = htons((u_short) port);

		if (bind(context->s, (struct sockaddr*)(&origin_addr), sizeof(origin_addr)) < 0) {
			mprintf("Failed to bind to port! %d\n", GetLastNetworkError());
			closesocket(context->s);
			return -1;
		}

		mprintf("Waiting for client to connect to %s:%d...\n", "localhost", port);

		// Receive client's connection attempt
		set_timeout(context->s, stun_timeout_ms);
		socklen_t slen = sizeof(context->addr);
		if (recvfrom(context->s, NULL, 0, 0, (struct sockaddr*)(&context->addr), &slen) < 0) {
			mprintf("Did not receive response from client! %d\n", GetLastNetworkError());
			closesocket(context->s);
			return -1;
		}

		// Send acknowledgement
		if (sendp(context, NULL, 0) < 0) {
			mprintf("Could not send ack to client! %d\n", GetLastNetworkError());
			closesocket(context->s);
			return -1;
		}

		mprintf("Client received at %s:%d!\n", inet_ntoa(context->addr.sin_addr), ntohs(context->addr.sin_port));

		set_timeout(context->s, recvfrom_timeout_ms);
	}

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
			if( err == ETIMEDOUT )
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
volatile static SDL_sem* multithreadedprintf_semaphore;
volatile static SDL_mutex* multithreadedprintf_mutex;

// Multithreaded printf queue
#define MPRINTF_QUEUE_SIZE 1000
#define MPRINTF_BUF_SIZE 1000
typedef struct mprintf_queue_item {
	bool log;
	char buf[MPRINTF_BUF_SIZE];
} mprintf_queue_item;
volatile static mprintf_queue_item mprintf_queue[MPRINTF_QUEUE_SIZE];
volatile static mprintf_queue_item mprintf_queue_cache[MPRINTF_QUEUE_SIZE];
volatile static int mprintf_queue_index = 0;
volatile static int mprintf_queue_size = 0;

// Multithreaded printf global variables
SDL_Thread* mprintf_thread = NULL;
volatile static bool run_multithreaded_printf;
void MultiThreadedPrintf(void* opaque);
void real_mprintf(bool log, const char* fmtStr, va_list args);
clock mprintf_timer;
FILE* mprintf_log_file = NULL;

void initMultiThreadedPrintf(bool use_logging) {
	if (use_logging) {
		mprintf_log_file = fopen("C:\\Program Files\\Fractal\\log.txt", "ab");
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

void MultiThreadedPrintf(void* opaque) {
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
				DeleteFileA((LPCSTR) L"C:\\Program Files\\Fractal\\log_prev.txt" );
				MoveFile( L"C:\\Program Files\\Fractal\\log.txt", L"C:\\Program Files\\Fractal\\log_prev.txt" );
				DeleteFileA((LPCSTR) L"C:\\Program Files\\Fractal\\log.txt" );
#endif
				mprintf_log_file = fopen( "C:\\Program Files\\Fractal\\log.txt", "ab" );
			}
		}
	}
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