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

/*** DEFINITIONS START ***/

#if defined(_WIN32)
  // @brief Windows keycodes for replaying SDL user inputs on server
  // @details index is SDL keycode, value is Windows keycode
const int windows_keycodes[265] = {
	NULL, // SDL keycodes start at index 4
	NULL, // SDL keycodes start at index 4
	NULL, // SDL keycodes start at index 4
	NULL, // SDL keycodes start at index 4
	0x41, // 4 -> A
	0x42, // 5 -> B
	0x43, // 6 -> C
	0x44, // 7 -> D
	0x45, // 8 -> E
	0x46, // 9 -> F
	0x47, // 10 -> G
	0x48, // 11 -> H
	0x49, // 12 -> I
	0x4A, // 13 -> J
	0x4B, // 14 -> K
	0x4C, // 15 -> L
	0x4D, // 16 -> M
	0x4E, // 17 -> N
	0x4F, // 18 -> O
	0x50, // 19 -> P
	0x51, // 20 -> Q
	0x52, // 21 -> R
	0x53, // 22 -> S
	0x54, // 23 -> T
	0x55, // 24 -> U
	0x56, // 25 -> V
	0x57, // 26 -> W
	0x58, // 27 -> X
	0x59, // 28 -> Y
	0x5A, // 29 -> Z
	0x31, // 30 -> 1
	0x32, // 31 -> 2
	0x33, // 32 -> 3
	0x34, // 33 -> 4
	0x35, // 34 -> 5
	0x36, // 35 -> 6
	0x37, // 36 -> 7
	0x38, // 37 -> 8
	0x39, // 38 -> 9
	0x30, // 39 -> 0
	0x0D, // 40 -> Enter
	0x1B, // 41 -> Escape
	0x08, // 42 -> Backspace
	0x09, // 43 -> Tab
	0x20, // 44 -> Space
	0xBD, // 45 -> Minus
	0xBB, // 46 -> Equal
	0xDB, // 47 -> Left Bracket
	0xDD, // 48 -> Right Bracket
	0xE2, // 49 -> Backslash
	NULL, // 50 -> no SDL keycode at index 50
	0xBA, // 51 -> Semicolon
	0xDE, // 52 -> Apostrophe
	0xDC, // 53 -> Backtick
	0xBC, // 54 -> Comma
	0xBE, // 55 -> Period
	0xBF, // 56 -> Slash
	0x14, // 57 -> Capslock
	0x70, // 58 -> F1
	0x71, // 59 -> F2
	0x72, // 60 -> F3
	0x73, // 61 -> F4
	0x74, // 62 -> F5
	0x75, // 63 -> F6
	0x76, // 64 -> F7
	0x77, // 65 -> F8
	0x78, // 66 -> F9
	0x79, // 67 -> F10
	0x7A, // 68 -> F11
	0x7B, // 69 -> F12
	0x2C, // 70 -> Print Screen
	0x91, // 71 -> Scroll Lock
	0xB3, // 72 -> Pause
	0x2D, // 73 -> Insert
	0x24, // 74 -> Home
	0x21, // 75 -> Pageup
	0x2E, // 76 -> Delete
	0x23, // 77 -> End
	0x22, // 78 -> Pagedown
	0x27, // 79 -> Right
	0x25, // 80 -> Left
	0x28, // 81 -> Down
	0x26, // 82 -> Up
	0x90, // 83 -> Numlock
	0x6F, // 84 -> Numeric Keypad Divide
	0x6A, // 85 -> Numeric Keypad Multiply
	0x6B, // 86 -> Numeric Keypad Minus
	0x2E, // 87 -> Numeric Keypad Plus
	0x0D, // 88 -> Numeric Keypad Enter
	0x61, // 89 -> Numeric Keypad 1
	0x62, // 90 -> Numeric Keypad 2
	0x63, // 91 -> Numeric Keypad 3
	0x64, // 92 -> Numeric Keypad 4
	0x65, // 93 -> Numeric Keypad 5
	0x66, // 94 -> Numeric Keypad 6
	0x67, // 95 -> Numeric Keypad 7
	0x68, // 96 -> Numeric Keypad 8
	0x69, // 97 -> Numeric Keypad 9
	0x60, // 98 -> Numeric Keypad 0
	0xBE, // 99 -> Numeric Keypad Period
	NULL, // 100 -> no SDL keycode at index 100
	0x5D, // 101 -> Application
	NULL, // 102 -> no SDL keycode at index 102
	NULL, // 103 -> no SDL keycode at index 103
	0x7C, // 104 -> F13
	0x7D, // 105 -> F14
	0x7E, // 106 -> F15
	0x7F, // 107 -> F16
	0x80, // 108 -> F17
	0x81, // 109 -> F18
	0x82, // 110 -> F19
	NULL, // 111 -> no SDL keycode at index 111
	NULL, // 112 -> no SDL keycode at index 112
	NULL, // 113 -> no SDL keycode at index 113
	NULL, // 114 -> no SDL keycode at index 114
	NULL, // 115 -> no SDL keycode at index 115
	NULL, // 116 -> no SDL keycode at index 116
	NULL, // 117 -> no SDL keycode at index 117
	VK_MENU, // 118 -> Menu
	NULL, // 119 -> no SDL keycode at index 119
	NULL, // 120 -> no SDL keycode at index 120
	NULL, // 121 -> no SDL keycode at index 121
	NULL, // 122 -> no SDL keycode at index 122
	NULL, // 123 -> no SDL keycode at index 123
	NULL, // 124 -> no SDL keycode at index 124
	NULL, // 125 -> no SDL keycode at index 125
	NULL, // 126 -> no SDL keycode at index 126
	VK_VOLUME_MUTE, // 127 -> Mute
	VK_VOLUME_UP, // 128 -> Volume Up
	VK_VOLUME_DOWN, // 129 -> Volume Down
	NULL, // 130 -> no SDL keycode at index 130
	NULL, // 131 -> no SDL keycode at index 131
	NULL, // 132 -> no SDL keycode at index 132
	NULL, // 133 -> no SDL keycode at index 133
	NULL, // 134 -> no SDL keycode at index 134
	NULL, // 135 -> no SDL keycode at index 135
	NULL, // 136 -> no SDL keycode at index 136
	NULL, // 137 -> no SDL keycode at index 137
	NULL, // 138 -> no SDL keycode at index 138
	NULL, // 139 -> no SDL keycode at index 139
	NULL, // 140 -> no SDL keycode at index 140
	NULL, // 141 -> no SDL keycode at index 141
	NULL, // 142 -> no SDL keycode at index 142
	NULL, // 143 -> no SDL keycode at index 143
	NULL, // 144 -> no SDL keycode at index 144
	NULL, // 145 -> no SDL keycode at index 145
	NULL, // 146 -> no SDL keycode at index 146
	NULL, // 147 -> no SDL keycode at index 147
	NULL, // 148 -> no SDL keycode at index 148
	NULL, // 149 -> no SDL keycode at index 149
	NULL, // 150 -> no SDL keycode at index 150
	NULL, // 151 -> no SDL keycode at index 151
	NULL, // 152 -> no SDL keycode at index 152
	NULL, // 153 -> no SDL keycode at index 153
	NULL, // 154 -> no SDL keycode at index 154
	NULL, // 155 -> no SDL keycode at index 155
	NULL, // 156 -> no SDL keycode at index 156
	NULL, // 157 -> no SDL keycode at index 157
	NULL, // 158 -> no SDL keycode at index 158
	NULL, // 159 -> no SDL keycode at index 159
	NULL, // 160 -> no SDL keycode at index 160
	NULL, // 161 -> no SDL keycode at index 161
	NULL, // 162 -> no SDL keycode at index 162
	NULL, // 163 -> no SDL keycode at index 163
	NULL, // 164 -> no SDL keycode at index 164
	NULL, // 165 -> no SDL keycode at index 165
	NULL, // 166 -> no SDL keycode at index 166
	NULL, // 167 -> no SDL keycode at index 167
	NULL, // 168 -> no SDL keycode at index 168
	NULL, // 169 -> no SDL keycode at index 169
	NULL, // 170 -> no SDL keycode at index 170
	NULL, // 171 -> no SDL keycode at index 171
	NULL, // 172 -> no SDL keycode at index 172
	NULL, // 173 -> no SDL keycode at index 173
	NULL, // 174 -> no SDL keycode at index 174
	NULL, // 175 -> no SDL keycode at index 175
	NULL, // 176 -> no SDL keycode at index 176
	NULL, // 177 -> no SDL keycode at index 177
	NULL, // 178 -> no SDL keycode at index 178
	NULL, // 179 -> no SDL keycode at index 179
	NULL, // 180 -> no SDL keycode at index 180
	NULL, // 181 -> no SDL keycode at index 181
	NULL, // 182 -> no SDL keycode at index 182
	NULL, // 183 -> no SDL keycode at index 183
	NULL, // 184 -> no SDL keycode at index 184
	NULL, // 185 -> no SDL keycode at index 185
	NULL, // 186 -> no SDL keycode at index 186
	NULL, // 187 -> no SDL keycode at index 187
	NULL, // 188 -> no SDL keycode at index 188
	NULL, // 189 -> no SDL keycode at index 189
	NULL, // 190 -> no SDL keycode at index 190
	NULL, // 191 -> no SDL keycode at index 191
	NULL, // 192 -> no SDL keycode at index 192
	NULL, // 193 -> no SDL keycode at index 193
	NULL, // 194 -> no SDL keycode at index 194
	NULL, // 195 -> no SDL keycode at index 195
	NULL, // 196 -> no SDL keycode at index 196
	NULL, // 197 -> no SDL keycode at index 197
	NULL, // 198 -> no SDL keycode at index 198
	NULL, // 199 -> no SDL keycode at index 199
	NULL, // 200 -> no SDL keycode at index 200
	NULL, // 201 -> no SDL keycode at index 201
	NULL, // 202 -> no SDL keycode at index 202
	NULL, // 203 -> no SDL keycode at index 203
	NULL, // 204 -> no SDL keycode at index 204
	NULL, // 205 -> no SDL keycode at index 205
	NULL, // 206 -> no SDL keycode at index 206
	NULL, // 207 -> no SDL keycode at index 207
	NULL, // 208 -> no SDL keycode at index 208
	NULL, // 209 -> no SDL keycode at index 209
	NULL, // 210 -> no SDL keycode at index 210
	NULL, // 211 -> no SDL keycode at index 212
	NULL, // 213 -> no SDL keycode at index 213
	NULL, // 214 -> no SDL keycode at index 214
	NULL, // 215 -> no SDL keycode at index 215
	NULL, // 216 -> no SDL keycode at index 216
	NULL, // 217 -> no SDL keycode at index 217
	NULL, // 218 -> no SDL keycode at index 218
	NULL, // 219 -> no SDL keycode at index 219
	NULL, // 220 -> no SDL keycode at index 220
	NULL, // 221 -> no SDL keycode at index 221
	NULL, // 222 -> no SDL keycode at index 222
	NULL, // 223 -> no SDL keycode at index 223
	NULL,
	VK_LCONTROL, // 224 -> Left Ctrl
	VK_LSHIFT, // 225 -> Left Shift
	VK_MENU, // 226 -> Left Alt
	VK_LWIN, // 227 -> Left GUI (Windows Key)
	VK_RCONTROL, // 228 -> Right Ctrl
	VK_RSHIFT, // 229 -> Right Shift
	VK_MENU, // 230 -> Right Alt
	VK_RWIN, // 231 -> Right GUI (Windows Key)
	NULL, // 232 -> no SDL keycode at index 232
	NULL, // 233 -> no SDL keycode at index 233
	NULL, // 234 -> no SDL keycode at index 234
	NULL, // 235 -> no SDL keycode at index 235
	NULL, // 236 -> no SDL keycode at index 236
	NULL, // 237 -> no SDL keycode at index 237
	NULL, // 238 -> no SDL keycode at index 238
	NULL, // 239 -> no SDL keycode at index 239
	NULL, // 240 -> no SDL keycode at index 240
	NULL, // 241 -> no SDL keycode at index 241
	NULL, // 242 -> no SDL keycode at index 242
	NULL, // 243 -> no SDL keycode at index 243
	NULL, // 244 -> no SDL keycode at index 244
	NULL, // 245 -> no SDL keycode at index 245
	NULL, // 246 -> no SDL keycode at index 246
	NULL, // 247 -> no SDL keycode at index 247
	NULL, // 248 -> no SDL keycode at index 248
	NULL, // 249 -> no SDL keycode at index 249
	NULL, // 250 -> no SDL keycode at index 250
	NULL, // 251 -> no SDL keycode at index 251
	NULL, // 252 -> no SDL keycode at index 252
	NULL, // 253 -> no SDL keycode at index 253
	NULL, // 254 -> no SDL keycode at index 254
	NULL, // 255 -> no SDL keycode at index 255
	NULL, // 256 -> no SDL keycode at index 256
	NULL, // 257 -> no SDL keycode at index 257
	VK_MEDIA_NEXT_TRACK, // 258 -> Audio/Media Next
	VK_MEDIA_PREV_TRACK, // 259 -> Audio/Media Prev
	VK_MEDIA_STOP, // 260 -> Audio/Media Stop
	VK_MEDIA_PLAY_PAUSE, // 261 -> Audio/Media Play
	VK_VOLUME_MUTE, // 262 -> Audio/Media Mute
	VK_LAUNCH_MEDIA_SELECT, // 263 -> Media Select
	0x7FFFFFFF };
#endif

/*** DEFINITIONS END ***/

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
	printf("Initialising Winsock...\n");
	if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
		printf("Failed. Error Code : %d.\n", WSAGetLastError());
	}
	printf("Winsock Initialised.\n");

	// Creating our TCP (receiving) socket (need it first to initiate connection)
	// AF_INET = IPv4
	// SOCK_STREAM = TCP Socket
	// 0 = protocol automatically detected
	if ((listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		printf("Could not create listen TCP socket: %d.\n" , WSAGetLastError());
		WSACleanup(); // close Windows socket library
	}
	printf("Listen TCP Socket created.\n");

	// prepare the sockaddr_in structure for the listening socket
	serverRECV.sin_family = AF_INET; // IPv4
	serverRECV.sin_addr.s_addr = INADDR_ANY; // any IP
	serverRECV.sin_port = htons(config.serverPortRECV); // initial default port

	// bind our socket to this port. If it fails, increment port by one and retry
	while (bind(listensocket, (struct sockaddr *) &serverRECV, sizeof(serverRECV)) == SOCKET_ERROR) {
		// at most 50 attempts, after that we give up
		if (bind_attempts == 50) {
			printf("Cannot find an open port, abort.\n");
			closesocket(listensocket); // close open socket
			WSACleanup(); // close Windows socket library
		}
		// display failed attempt
		printf("Bind attempt #%i failed with error code: %d.\n", bind_attempts, WSAGetLastError());

		// increment port number and retry
		bind_attempts += 1;
		serverRECV.sin_port = htons(config.serverPortRECV + bind_attempts); // initial default port 48888
	}
	// successfully binded, we're good to go
	printf("Bind done on port: %d.\n", ntohs(serverRECV.sin_port));

	// this passive socket is always open to listen for an incoming connection
	listen(listensocket, 1); // 1 maximum concurrent connection
	printf("Waiting for an incoming connection...\n");

	// done initializing, waiting for a connection
	return listensocket;
}
*/

#if defined(_WIN32)
/// @brief replays a user action taken on the client and sent to the server
/// @details parses the FractalClientMessage struct and send input to Windows OS
FractalStatus ReplayUserInput(struct FractalClientMessage fmsg[6], int len) {
	// get screen width and height for mouse cursor
	int sWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
	int sHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
	int i;
	INPUT Event[6];

	len = min(len, 6);

	for (i = 0; i < len; i++) {
		// switch to fill in the Windows event depending on the FractalClientMessage type
		switch (fmsg[i].type) {
			// Windows event for keyboard action
		case MESSAGE_KEYBOARD:
			Event[i].ki.wVk = windows_keycodes[fmsg[i].keyboard.code];
			Event[i].type = INPUT_KEYBOARD;
			Event[i].ki.wScan = 0;
			Event[i].ki.time = 0; // system supplies timestamp
			if (!fmsg[i].keyboard.pressed) {
				Event[i].ki.dwFlags = KEYEVENTF_KEYUP;
			}
			else {
				Event[i].ki.dwFlags = 0;
			}
			break;
			// mouse motion event
		case MESSAGE_MOUSE_MOTION:
			Event[i].type = INPUT_MOUSE;
			Event[i].mi.dx = fmsg[i].mouseMotion.x * ((float)65536 / sWidth);
			Event[i].mi.dy = fmsg[i].mouseMotion.y * ((float)65536 / sHeight);
			Event[i].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
			break;
			// mouse button event
		case MESSAGE_MOUSE_BUTTON:
			Event[i].type = INPUT_MOUSE;
			Event[i].mi.dx = 0;
			Event[i].mi.dy = 0;
			// switch to parse button type
			switch (fmsg[i].mouseButton.button) {
				// leftclick
			case 1:
				if (fmsg[i].mouseButton.pressed) {
					Event[i].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				}
				else {
					Event[i].mi.dwFlags = MOUSEEVENTF_LEFTUP;
				}
				break; // inner switch
			// right click
			case 3:
				if (fmsg[i].mouseButton.pressed) {
					Event[i].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
				}
				else {
					Event[i].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
				}
				break; // inner switch
			}
			break; // outer switch
			  // mouse wheel event
		case MESSAGE_MOUSE_WHEEL:
			Event[i].type = INPUT_MOUSE;
			Event[i].mi.dwFlags = MOUSEEVENTF_WHEEL;
			Event[i].mi.dx = 0;
			Event[i].mi.dy = 0;
			Event[i].mi.mouseData = fmsg[i].mouseWheel.y * 100;
			break;
			// TODO: add clipboard
		}
	}
	// send FMSG mapped to Windows event to Windows and return
	int num_events_sent = SendInput(len, Event, sizeof(INPUT)); // 1 structure to send

	if (len != num_events_sent) {
		mprintf("SendInput did not send all events! %d/%d\n", num_events_sent, len);
	}

	return FRACTAL_OK;
}

FractalStatus EnterWinString(enum FractalKeycode keycodes[100], int len) {
  // get screen width and height for mouse cursor
  int i, index = 0;
  enum FractalKeycode keycode;
  INPUT Event[200];

  for(i = 0; i < len; i++) {
	keycode = keycodes[i];
	Event[index].ki.wVk = windows_keycodes[keycode];
	Event[index].type = INPUT_KEYBOARD;
	Event[index].ki.wScan = 0;
	Event[index].ki.time = 0; // system supplies timestamp
	Event[index].ki.dwFlags = 0;

	index++;

	Event[index].ki.wVk = windows_keycodes[keycode];
	Event[index].type = INPUT_KEYBOARD;
	Event[index].ki.wScan = 0;
	Event[index].ki.time = 0; // system supplies timestamp
	Event[index].ki.dwFlags = KEYEVENTF_KEYUP;

	index++;
  }
  // send FMSG mapped to Windows event to Windows and return
  SendInput(index, Event, sizeof(INPUT)); // 1 structure to send
 
	return FRACTAL_OK;
}
#endif

int CreateUDPContext(struct SocketContext* context, char* origin, char* destination, int recvfrom_timeout_ms, int stun_timeout_ms) {
	SOCKET s;
	struct sockaddr_in addr;
	struct FractalDestination buf;
	unsigned int slen = sizeof(context->addr);

	// Function parameter checking
	if (!(strcmp(origin, "C") == 0 || strcmp(origin, "S") == 0)) {
		printf("Invalid origin parameter. Specify 'S' or 'C'.");
		return -1;
	}
	// Create UDP socket
	context->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (context->s < 0) { // Windows & Unix cases
		printf("Could not create UDP socket\n");
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
		printf("Strings too long!\n");
		return -1;
	}
	strcpy(dest, destination);
	strcat(dest, origin);

	if (stun_timeout_ms == 0) {
		printf("STUN timeout can't be zero!\n");
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
		printf("Failed to set timeout\n");
		return -1;
	}

	// Connect to STUN server
	printf("Connecting to STUN server...\n");
	clock attempt_time;
	StartTimer(&attempt_time);
	do {
		if (stun_timeout_ms > 0 && GetTimer(attempt_time)*1000.0 > stun_timeout_ms) {
			printf("STUN server failed to respond!\n");
			return -1;
		}
		// Refresh STUN
		if (sendto(context->s, dest, strlen(dest), 0, (struct sockaddr*)(&context->addr), slen) < 0) {
			printf("Failed to message STUN server, trying again...\n");
		}
		// Wait for response from STUN server
	} while (recvfrom(context->s, &buf, sizeof(buf), 0, (struct sockaddr*)(&context->addr), &slen) < 0);

	printf("Received packet from STUN server at %s:%d\n", inet_ntoa(context->addr.sin_addr), ntohs(context->addr.sin_port));

	// Set destination address to the client that the STUN server has paired us with
	context->addr.sin_addr.s_addr = buf.host;
	context->addr.sin_port = buf.port;

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

int SendAck(struct SocketContext* context, int reps) {
	bool success = false;
	for (int rep = 0; rep < reps; rep++) {
		if (sendto(context->s, NULL, 0, 0, (struct sockaddr*)(&context->addr), sizeof(context->addr)) >= 0) {
			success = true;
		}
	}

	return success ? 0 : -1;
}

int ReceiveAck(struct SocketContext* context) {
	char recv_buf[1000];
	unsigned int slen = sizeof(context->addr);

	if (recvfrom(context->s, &recv_buf, sizeof(recv_buf), 0, (struct sockaddr*)(&context->addr), &slen) < 0) {
		return -1;
	}
	else {
		return 0;
	}
}

// Multithreaded printf Semaphores and Mutexes
volatile static SDL_sem* multithreadedprintf_semaphore;
volatile static SDL_mutex* multithreadedprintf_mutex;

// Multithreaded printf queue
#define MPRINTF_QUEUE_SIZE 1000
#define MPRINTF_BUF_SIZE 1000
volatile static char mprintf_queue[MPRINTF_QUEUE_SIZE][MPRINTF_BUF_SIZE];
volatile static char mprintf_queue_cache[MPRINTF_QUEUE_SIZE][MPRINTF_BUF_SIZE];
volatile static int mprintf_queue_index = 0;
volatile static int mprintf_queue_size = 0;

// Multithreaded printf global variables
SDL_Thread* mprintf_thread = NULL;
volatile static bool run_multithreaded_printf;
void MultiThreadedPrintf(void* opaque);
FILE* mprintf_log_file = NULL;

void initMultiThreadedPrintf(bool use_logging) {
	if (use_logging) {
		mprintf_log_file = fopen("C:\\log.txt", "a+");
	}

	run_multithreaded_printf = true;
	multithreadedprintf_mutex = SDL_CreateMutex();
	multithreadedprintf_semaphore = SDL_CreateSemaphore(0);
	mprintf_thread = SDL_CreateThread(MultiThreadedPrintf, "MultiThreadedPrintf", NULL);

	if (mprintf_log_file) {
		fclose(mprintf_log_file);
	}
	mprintf_log_file = NULL;
}

void destroyMultiThreadedPrintf() {
	run_multithreaded_printf = false;
	for (int i = 0; i < 200; i++) {
		SDL_SemPost(multithreadedprintf_semaphore);
	}
	SDL_WaitThread(mprintf_thread, NULL);
	mprintf_thread = NULL;
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
			strcpy(mprintf_queue_cache[i], mprintf_queue[mprintf_queue_index]);
			mprintf_queue_index++;
			mprintf_queue_index %= MPRINTF_QUEUE_SIZE;
			if (i != 0) {
				SDL_SemWait(multithreadedprintf_semaphore);
			}
		}
		mprintf_queue_size = 0;
		SDL_UnlockMutex(multithreadedprintf_mutex);

		for (int i = 0; i < cache_size; i++) {
			if (mprintf_log_file) {
				fprintf(mprintf_log_file, "%s", mprintf_queue_cache[i]);
				fflush(mprintf_log_file);
			}
			printf("%s", mprintf_queue_cache[i]);
		}
	}
}

void mprintf(const char* fmtStr, ...) {
	if (mprintf_thread == NULL) {
		printf("initMultiThreadedPrintf has not been called!\n");
		return;
	}

	va_list args;
	va_start(args, fmtStr);

	SDL_LockMutex(multithreadedprintf_mutex);
	int index = (mprintf_queue_index + mprintf_queue_size) % MPRINTF_QUEUE_SIZE;
	char* buf = NULL;
	if (mprintf_queue_size < MPRINTF_QUEUE_SIZE - 2) {
		buf = &mprintf_queue[index];
		vsnprintf(buf, MPRINTF_BUF_SIZE, fmtStr, args);
		mprintf_queue_size++;
	}
	else if (mprintf_queue_size == MPRINTF_QUEUE_SIZE - 2) {
		buf = &mprintf_queue[index];
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




uint32_t Hash(void* key, size_t len)
{
	char* buf = key;
	uint32_t hash, i;
	for (hash = i = 0; i < len; ++i)
	{
		hash += buf[i];
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
