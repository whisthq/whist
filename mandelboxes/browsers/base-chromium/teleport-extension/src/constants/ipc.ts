export enum ContentScriptMessageType {
  POINTER_LOCK = "POINTER_LOCK",
  HISTORY_GO_BACK = "HISTORY_GO_BACK",
  HISTORY_GO_FORWARD = "HISTORY_GO_FORWARD",
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
