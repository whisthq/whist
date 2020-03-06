/*
 * This file contains functions to send keyboard/mouse input.

 Protocol version: 1.0
 Last modification: 2/10/2020

 By: Ming Ying

 Copyright Fractal Computers, Inc. 2019
*/
#include "input.h" // header file for this file

#if defined(_WIN32)
// @brief Windows keycodes for replaying SDL user inputs on server
// @details index is SDL keycode, value is Windows keycode
const int windows_keycodes[NUM_KEYCODES] = {
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
	VK_BACK, // 42 -> Backspace
	0x09, // 43 -> Tab
	0x20, // 44 -> Space
	0xBD, // 45 -> Minus
	0xBB, // 46 -> Equal
	0xDB, // 47 -> Left Bracket
	0xDD, // 48 -> Right Bracket
	0xE2, // 49 -> Backslash
	NULL, // 50 -> no SDL keycode at index 50
	VK_OEM_1, // 51 -> Semicolon
	VK_OEM_7, // 52 -> Apostrophe
	VK_OEM_3, // 53 -> Backtick
	VK_OEM_COMMA, // 54 -> Comma
	VK_OEM_PERIOD, // 55 -> Period
	VK_OEM_2, // 56 -> Forward Slash
	VK_CAPITAL, // 57 -> Capslock
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
	VK_SNAPSHOT, // 70 -> Print Screen
	VK_SCROLL, // 71 -> Scroll Lock
	VK_PAUSE, // 72 -> Pause
	VK_INSERT, // 73 -> Insert
	VK_HOME, // 74 -> Home
	VK_PRIOR, // 75 -> Pageup
	VK_DELETE, // 76 -> Delete
	VK_END, // 77 -> End
	VK_NEXT, // 78 -> Pagedown
	0x27, // 79 -> Right
	0x25, // 80 -> Left
	0x28, // 81 -> Down
	0x26, // 82 -> Up
	0x90, // 83 -> Numlock
	VK_DIVIDE, // 84 -> Numeric Keypad Divide
	VK_MULTIPLY, // 85 -> Numeric Keypad Multiply
	0x6B, // 86 -> Numeric Keypad Minus
	VK_ADD, // 87 -> Numeric Keypad Plus
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
	VK_DECIMAL, // 99 -> Numeric Keypad Period
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
	NULL, // 118 -> Menu
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
	VK_LMENU, // 226 -> Left Alt
	VK_LWIN, // 227 -> Left GUI (Windows Key)
	VK_RCONTROL, // 228 -> Right Ctrl
	VK_RSHIFT, // 229 -> Right Shift
	VK_RMENU, // 230 -> Right Alt
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


int GetWindowsKeyCode(int sdl_keycode) {
	return windows_keycodes[sdl_keycode];
}

void updateKeyboardState( struct FractalClientMessage* fmsg )
{
	if( fmsg->type != MESSAGE_KEYBOARD_STATE )
	{
		mprintf( "updateKeyboardState requires fmsg.type to be MESSAGE_KEYBOARD_STATE\n" );
		return;
	}

	INPUT ip;
	ip.type = INPUT_KEYBOARD;
	ip.ki.wVk = 0;
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;

	for( int sdl_keycode = 0; sdl_keycode < fmsg->num_keycodes; sdl_keycode++ )
	{
		int windows_keycode = GetWindowsKeyCode( sdl_keycode );

		if( windows_keycode )
		{
			ip.ki.wScan = MapVirtualKeyA( windows_keycode, MAPVK_VK_TO_VSC_EX );
			ip.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
			if( ip.ki.wScan >> 8 == 0xE0 )
			{
				ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
				ip.ki.wScan &= 0xFF;
			}

			if( !fmsg->keyboard_state[sdl_keycode] && GetAsyncKeyState( windows_keycode ) )
			{
				SendInput( 1, &ip, sizeof( INPUT ) );
			}
		}
	}

	for( int sdl_keycode = 0; sdl_keycode < fmsg->num_keycodes; sdl_keycode++ )
	{
		int windows_keycode = GetWindowsKeyCode( sdl_keycode );

		if( windows_keycode )
		{
			ip.ki.wScan = MapVirtualKeyA( windows_keycode, MAPVK_VK_TO_VSC );
			ip.ki.dwFlags = KEYEVENTF_SCANCODE;
			if( ip.ki.wScan >> 8 == 0xE0 )
			{
				ip.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
				ip.ki.wScan &= 0xFF;
			}

			if( fmsg->keyboard_state[sdl_keycode] && !GetAsyncKeyState( windows_keycode ) )
			{
				SendInput( 1, &ip, sizeof( INPUT ) );
			}
		}
	}
}

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
			//Event[i].ki.wVk = windows_keycodes[fmsg[i].keyboard.code];
			Event[i].type = INPUT_KEYBOARD;
			Event[i].ki.time = 0; // system supplies timestamp

			Event[i].ki.dwFlags = KEYEVENTF_SCANCODE;
			Event[i].ki.wVk = 0;
			Event[i].ki.wScan = MapVirtualKeyA( windows_keycodes[fmsg[i].keyboard.code], MAPVK_VK_TO_VSC_EX );

			if( Event[i].ki.wScan >> 8 == 0xE0 )
			{
				Event[i].ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
				Event[i].ki.wScan &= 0xFF;
			}

			if (!fmsg[i].keyboard.pressed) {
				Event[i].ki.dwFlags |= KEYEVENTF_KEYUP;
			}
			else {
				Event[i].ki.dwFlags |= 0;
			}

			break;
			// mouse motion event
		case MESSAGE_MOUSE_MOTION:
			Event[i].type = INPUT_MOUSE;
			if(fmsg[i].mouseMotion.relative) {
				Event[i].mi.dx = fmsg[i].mouseMotion.x * 0.9;
				Event[i].mi.dy = fmsg[i].mouseMotion.y * 0.9;
				Event[i].mi.dwFlags = MOUSEEVENTF_MOVE;
			} else {
				Event[i].mi.dx = fmsg[i].mouseMotion.x * (double) 65536 / 1000000;
				Event[i].mi.dy = fmsg[i].mouseMotion.y * (double) 65536 / 1000000;
				Event[i].mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
			}
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

FractalStatus EnterWinString(enum FractalKeycode* keycodes, int len) {
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
	SendInput(index, Event, sizeof(INPUT)); 

	return FRACTAL_OK;
}

void LoadCursors(FractalCursorTypes *types) {
  types->CursorAppStarting = LoadCursor(NULL, IDC_APPSTARTING);
  types->CursorArrow       = LoadCursor(NULL, IDC_ARROW);
  types->CursorCross       = LoadCursor(NULL, IDC_CROSS);
  types->CursorHand        = LoadCursor(NULL, IDC_HAND);
  types->CursorHelp        = LoadCursor(NULL, IDC_HELP);
  types->CursorIBeam       = LoadCursor(NULL, IDC_IBEAM);
  types->CursorIcon        = LoadCursor(NULL, IDC_ICON);
  types->CursorNo          = LoadCursor(NULL, IDC_NO);
  types->CursorSize        = LoadCursor(NULL, IDC_SIZE);
  types->CursorSizeAll     = LoadCursor(NULL, IDC_SIZEALL);
  types->CursorSizeNESW    = LoadCursor(NULL, IDC_SIZENESW);
  types->CursorSizeNS      = LoadCursor(NULL, IDC_SIZENS);
  types->CursorSizeNWSE    = LoadCursor(NULL, IDC_SIZENWSE);
  types->CursorSizeWE      = LoadCursor(NULL, IDC_SIZEWE);
  types->CursorUpArrow     = LoadCursor(NULL, IDC_UPARROW);
  types->CursorWait        = LoadCursor(NULL, IDC_WAIT);
}

FractalCursorImage GetCursorImage(FractalCursorTypes *types, PCURSORINFO pci) {
	HCURSOR cursor = pci->hCursor;
	FractalCursorImage image = {0};

	if(cursor == types->CursorArrow) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_ARROW;
	} else if(cursor == types->CursorCross) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_CROSSHAIR;
	} else if(cursor == types->CursorHand) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_HAND;
	} else if(cursor == types->CursorIBeam) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_IBEAM;
	} else if(cursor == types->CursorNo) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_NO;
	} else if(cursor == types->CursorSizeAll) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_SIZEALL;
	} else if(cursor == types->CursorSizeNESW) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_SIZENESW;
	} else if(cursor == types->CursorSizeNS) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_SIZENS;
	} else if(cursor == types->CursorSizeNWSE) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_SIZENWSE;
	} else if(cursor == types->CursorSizeWE) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_SIZEWE;
	} else if(cursor == types->CursorWait) {
		image.cursor_id   = SDL_SYSTEM_CURSOR_WAITARROW;
	} else {
		image.cursor_id   = SDL_SYSTEM_CURSOR_ARROW;
	}

	return image;
}

FractalCursorImage GetCurrentCursor(FractalCursorTypes *types) {
    CURSORINFO *pci = (CURSORINFO *) malloc(sizeof(CURSORINFO));
    pci->cbSize = sizeof(CURSORINFO);
    GetCursorInfo(pci);

    FractalCursorImage image = {0};
    image = GetCursorImage(types, pci);

    image.cursor_state = pci->flags;

    return image;
}