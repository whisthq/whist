export enum ContentScriptMessageType {
  POINTER_LOCK = "POINTER_LOCK",
  GESTURE_DETECTED = "GESTURE_DETECTED",
  DRAW_NAVIGATION_ARROW = "DRAW_NAVIGATION_ARROW",
}

export enum NativeHostMessageType {
  DOWNLOAD_COMPLETE = "DOWNLOAD_COMPLETE",
  NATIVE_HOST_UPDATE = "NATIVE_HOST_UPDATE",
  NATIVE_HOST_EXIT = "NATIVE_HOST_EXIT",
  POINTER_LOCK = "POINTER_LOCK",
  CREATE_NEW_TAB = "CREATE_NEW_TAB",
  ACTIVATE_TAB = "ACTIVATE_TAB",
}

export interface ContentScriptMessage {
  type: ContentScriptMessageType
  value: any
}

export interface NativeHostMessage {
  type: NativeHostMessageType
  value: any
}
