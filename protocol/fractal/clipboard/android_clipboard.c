/*
 * Clipboard for android, empty for now
 *
 * Copyright Fractal Computers, Inc. 2020
 **/
#include "clipboard.h"
#include <stddef.h>

bool StartTrackingClipboardUpdates();


void initClipboard() {
}

bool StartTrackingClipboardUpdates() {
  return true;
}

bool hasClipboardUpdated() {
  return false;
}

ClipboardData* GetClipboard() {
  return NULL;
}

void SetClipboard(ClipboardData* cb) {
}
