#ifndef CLIPBOARD_H
#define CLIPBOARD_H

typedef enum ClipboardType
{
	CLIPBOARD_NONE,
	CLIPBOARD_TEXT,
	CLIPBOARD_IMAGE
} ClipboardType;

typedef struct ClipboardData
{
	int size;
	ClipboardType type;
	char data[800];
} ClipboardData;

ClipboardData GetClipboard();

#endif CLIPBOARD_H