#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <stdbool.h>

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
	char data[];
} ClipboardData;

ClipboardData* GetClipboard();

void SetClipboard( ClipboardData* cb );

void StartTrackingClipboardUpdates();

bool hasClipboardUpdated();

#endif // CLIPBOARD_H