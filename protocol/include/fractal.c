/*
 * This file contains the implementation of the functions used as part of the
 * streaming protocol.

 Protocol version: 1.0
 Last modification: 12/14/2019

 By: Philippe Noël

 Copyright Fractal Computers, Inc. 2019
*/

#include <stdint.h>
#include <stdbool.h>

#include "fractal.h" // header file for this protocol, includes winsock

// windows warnings
#if defined(_WIN32)
#pragma warning(disable: 4201)
#endif


/*** FRACTAL FUNCTIONS START ***/

/*
/// @brief destroy the server sockets and threads, and WSA for windows
/// @details if full=true, destroys everything, else only current connection
FractalStatus ServerDestroy(SOCKET sockets[], HANDLE threads[], bool full) {
	int i; // counter
	int num_threads = strlen(threads); // max number of threads we have
	int num_sockets = strlen(sockets); // max number of sockets we have

	// first we close the threads if they ever got started
	for (i = 0; i < num_threads; i++) {
		// if a thread was initialized, close it
		if (threads[i] != 0) {
			CloseHandle(threads[i]);
		}
	}

	// if full == true, we destroy everything to close the app, else we only
	// destroy the current connection, but leave listensocket on to listen for
	// future connections without needing to manually restart the app
	if (full) {
		// then we close Windows socket library
		WSACleanup();
		i = 0; // set index to 0 to destroy all sockets
	}
	// keep the listensocket and windows socket library active
	else {
		i = 1; // set index to 1 to skip the listen socket when destroying
	}

	// then we can close the sockets that were opened
	for (i; i < num_sockets; i++) {
		// if a socket was opened, closed it
		if (sockets[i] != 0) {
			closesocket(sockets[i]);
		}
	}

	// done
	return FRACTAL_OK;
}

/// @brief initialize the listen socket (TCP path)
/// @details initializes windows socket, creates and binds our listen socket
SOCKET ServerInit(SOCKET listensocket, FractalConfig config) {
	// socket variables
	int bind_attempts = 0; 	// init counter to attempt multiple port bindings
	WSADATA wsa; // windows socket library
	struct sockaddr_in serverRECV; // serverRECV port parameters

	// initialize Winsock (sockets library)
	mprintf("Initialising Winsock...\n");
	if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
		mprintf("Failed. Error Code : %d.\n", WSAGetLastError());
	}
	mprintf("Winsock Initialised.\n");

	// Creating our TCP (receiving) socket (need it first to initiate connection)
	// AF_INET = IPv4
	// SOCK_STREAM = TCP Socket
	// 0 = protocol automatically detected
	if ((listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		mprintf("Could not create listen TCP socket: %d.\n" , WSAGetLastError());
		WSACleanup(); // close Windows socket library
	}
	mprintf("Listen TCP Socket created.\n");

	// prepare the sockaddr_in structure for the listening socket
	serverRECV.sin_family = AF_INET; // IPv4
	serverRECV.sin_addr.s_addr = INADDR_ANY; // any IP
	serverRECV.sin_port = htons(config.serverPortRECV); // initial default port

	// bind our socket to this port. If it fails, increment port by one and retry
	while (bind(listensocket, (struct sockaddr *) &serverRECV, sizeof(serverRECV)) == SOCKET_ERROR) {
		// at most 50 attempts, after that we give up
		if (bind_attempts == 50) {
			mprintf("Cannot find an open port, abort.\n");
			closesocket(listensocket); // close open socket
			WSACleanup(); // close Windows socket library
		}
		// display failed attempt
		mprintf("Bind attempt #%i failed with error code: %d.\n", bind_attempts, WSAGetLastError());

		// increment port number and retry
		bind_attempts += 1;
		serverRECV.sin_port = htons(config.serverPortRECV + bind_attempts); // initial default port 48888
	}
	// successfully binded, we're good to go
	mprintf("Bind done on port: %d.\n", ntohs(serverRECV.sin_port));

	// this passive socket is always open to listen for an incoming connection
	listen(listensocket, 1); // 1 maximum concurrent connection
	mprintf("Waiting for an incoming connection...\n");

	// done initializing, waiting for a connection
	return listensocket;
}
*/

int CreateUDPContext(struct SocketContext* context, char* origin, char* destination, int recvfrom_timeout_ms, int stun_timeout_ms) {
	SOCKET s;
	struct sockaddr_in addr;
	struct FractalDestination buf;
	unsigned int slen = sizeof(context->addr);

	// Function parameter checking
	if (!(strcmp(origin, "C") == 0 || strcmp(origin, "S") == 0)) {
		mprintf("Invalid origin parameter. Specify 'S' or 'C'.");
		return -1;
	}
	// Create UDP socket
	context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (context->s < 0) { // Windows & Unix cases
		mprintf("Could not create UDP socket\n");
		return -1;
	}

	memset((char*)&context->addr, 0, sizeof(context->addr));
	context->addr.sin_family = AF_INET;
	context->addr.sin_port = htons(PORT);

	// Point address to STUN server
	context->addr.sin_addr.s_addr = inet_addr(STUN_SERVER_IP);

	// Create payload to send to STUN server
#define DEST_BUF_LEN 20
	char dest[DEST_BUF_LEN];
	if (strlen(destination) + strlen(origin) + 1 > DEST_BUF_LEN) {
		mprintf("Strings too long!\n");
		return -1;
	}
	strcpy(dest, destination);
	strcat(dest, origin);

	if (stun_timeout_ms == 0) {
		mprintf("STUN timeout can't be zero!\n");
		return -1;
	}

	// Set timeout, will refresh STUN this often
	const int stun_server_retry_ms = 50;
#if defined(_WIN32)
	int read_timeout = stun_server_retry_ms;
#else
	struct timeval read_timeout;
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = stun_server_retry_ms * 1000;
#endif


	if (setsockopt(context->s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout)) < 0) {
		mprintf("Failed to set timeout\n");
		return -1;
	}

	// Connect to STUN server
	mprintf("Connecting to STUN server...\n");
	clock attempt_time;
	StartTimer(&attempt_time);
	do {
		if (stun_timeout_ms > 0 && GetTimer(attempt_time)*1000.0 > stun_timeout_ms) {
			mprintf("STUN server failed to respond!\n");
			return -1;
		}
		// Refresh STUN
		if (sendto(context->s, dest, strlen(dest), 0, (struct sockaddr*)(&context->addr), slen) < 0) {
			mprintf("Failed to message STUN server, trying again...\n");
		}
		// Wait for response from STUN server
	} while (recvfrom(context->s, &buf, sizeof(buf), 0, (struct sockaddr*)(&context->addr), &slen) < 0);

	// Set destination address to the client that the STUN server has paired us with
	context->addr.sin_addr.s_addr = buf.host;
	context->addr.sin_port = buf.port;

	mprintf("Received packet from STUN server, connecting to %s:%d\n", inet_ntoa(context->addr.sin_addr), ntohs(context->addr.sin_port));
	SDL_Delay(100);
	if (sendto(context->s, NULL, 0, 0, (struct sockaddr*)(&context->addr), sizeof(context->addr)) < 0) {
		mprintf("Could not open connection\n");
	}
	SDL_Delay(200);

	// Set timeout, default 5 seconds
	if (recvfrom_timeout_ms < 0) {
		recvfrom_timeout_ms = 5000;
	}

	if (recvfrom_timeout_ms == 0) {
		u_long mode = 1;
		#if defined(_WIN32)
			ioctlsocket(context->s, FIONBIO, &mode);
		#else
			ioctl(context->s, FIONBIO, &mode);
		#endif
	}
	else {
#if defined(_WIN32)
		read_timeout = recvfrom_timeout_ms;
#else
		read_timeout.tv_sec = 0;
		read_timeout.tv_usec = recvfrom_timeout_ms * 1000;
#endif
		setsockopt(context->s, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
	}

	// Great success!
	return 0;
}

int recvp(struct SocketContext* context, void* buf, int len) {
	return recvfrom(context->s, buf, len, 0, NULL, NULL);
}

int sendp(struct SocketContext* context, void* buf, int len) {
	return sendto(context->s, buf, len, 0, (struct sockaddr*)(&context->addr), sizeof(context->addr));
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
		mprintf_log_file = fopen("C:\\log.txt", "a+");
	}

	run_multithreaded_printf = true;
	multithreadedprintf_mutex = SDL_CreateMutex();
	multithreadedprintf_semaphore = SDL_CreateSemaphore(0);
	mprintf_thread = SDL_CreateThread(MultiThreadedPrintf, "MultiThreadedPrintf", NULL);
	StartTimer(&mprintf_timer);
}

void destroyMultiThreadedPrintf() {
	// Wait for any remaining printfs to execute
	SDL_Delay(50);

	run_multithreaded_printf = false;
	SDL_SemPost(multithreadedprintf_semaphore);

	SDL_WaitThread(mprintf_thread, NULL);
	mprintf_thread = NULL;

	if (mprintf_log_file) {
		fclose(mprintf_log_file);
	}
	mprintf_log_file = NULL;
}

void MultiThreadedPrintf(void* opaque) {
	int produced_in_advance = 0;
	while (true) {
		SDL_SemWait(multithreadedprintf_semaphore);

		if (!run_multithreaded_printf) {
			break;
		}

		int cache_size = 0;

		SDL_LockMutex(multithreadedprintf_mutex);
		cache_size = mprintf_queue_size;
		for (int i = 0; i < mprintf_queue_size; i++) {
			mprintf_queue_cache[i].log = mprintf_queue[mprintf_queue_index].log;
			strcpy(mprintf_queue_cache[i].buf, mprintf_queue[mprintf_queue_index].buf);
			mprintf_queue_index++;
			mprintf_queue_index %= MPRINTF_QUEUE_SIZE;
			if (i != 0) {
				SDL_SemWait(multithreadedprintf_semaphore);
			}
		}
		mprintf_queue_size = 0;
		SDL_UnlockMutex(multithreadedprintf_mutex);

		for (int i = 0; i < cache_size; i++) {
			if (mprintf_log_file && mprintf_queue_cache[i].log) {
				fprintf(mprintf_log_file, "%s", mprintf_queue_cache[i].buf);
				fflush(mprintf_log_file);
			}
			printf("%s", mprintf_queue_cache[i].buf);
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
		mprintf("initMultiThreadedPrintf has not been called!\n");
		return;
	}

	SDL_LockMutex(multithreadedprintf_mutex);
	int index = (mprintf_queue_index + mprintf_queue_size) % MPRINTF_QUEUE_SIZE;
	char* buf = NULL;
	if (mprintf_queue_size < MPRINTF_QUEUE_SIZE - 2) {
		mprintf_queue[index].log = log;
		buf = mprintf_queue[index].buf;
		snprintf(buf, MPRINTF_BUF_SIZE, "%15.4f: ", GetTimer(mprintf_timer));
		int len = strlen(buf);
		vsnprintf(buf + len, MPRINTF_BUF_SIZE - len, fmtStr, args);
		mprintf_queue_size++;
	}
	else if (mprintf_queue_size == MPRINTF_QUEUE_SIZE - 2) {
		mprintf_queue[index].log = log;
		buf = mprintf_queue[index].buf;
		strcpy(buf, "Buffer maxed out!!!\n");
		mprintf_queue_size++;
	}

	if (buf != NULL) {
		buf[MPRINTF_BUF_SIZE - 5] = '.';
		buf[MPRINTF_BUF_SIZE - 4] = '.';
		buf[MPRINTF_BUF_SIZE - 3] = '.';
		buf[MPRINTF_BUF_SIZE - 2] = '\n';
		buf[MPRINTF_BUF_SIZE - 1] = '\0';
		SDL_SemPost(multithreadedprintf_semaphore);
	}
	SDL_UnlockMutex(multithreadedprintf_mutex);

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
	#elif __APPLE__
		// start timer
		gettimeofday(timer, NULL);
	#endif
}

double GetTimer(clock timer) {
	#if defined(_WIN32)
		LARGE_INTEGER end;
		QueryPerformanceCounter(&end);
		double ret = (double) (end.QuadPart - timer.QuadPart) / frequency.QuadPart;
	#elif __APPLE__
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

	uint32_t hash = (pre_hash << 32) ^ pre_hash;
	for (int i = 0; i < len; ++i)
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


void getClientResolution(unsigned int *width, unsigned int *height) {
	#if defined (_WIN32)
		width = (int) GetSystemMetrics(SM_CXSCREEN);
		height = (int) GetSystemMetrics(SM_CYSCREEN);
	#else // apple, prob need a different one for linux TODO
		auto mainDisplayId = CGMainDisplayID();
		width = CGDisplayPixelsWide(mainDisplayId);
		height = CGDisplayPixelsHigh(mainDisplayId);
	#endif
}

/*** FRACTAL FUNCTIONS END ***/

// renable Windows warning
#if defined(_WIN32)
#pragma warning(default: 4201)
#endif
