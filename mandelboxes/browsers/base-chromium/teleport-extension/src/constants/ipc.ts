export enum ContentScriptMessageType {
  POINTER_LOCK = "POINTER_LOCK",
}

export enum NativeHostMessageType {
  DOWNLOAD_COMPLETE = "DOWNLOAD_COMPLETE",
  NATIVE_HOST_UPDATE = "NATIVE_HOST_UPDATE",
  NATIVE_HOST_EXIT = "NATIVE_HOST_EXIT",
  POINTER_LOCK = "POINTER_LOCK",
}

export interface ContentScriptMessage {
  type: ContentScriptMessageType
  value: any
}

export interface NativeHostMessage {
  type: NativeHostMessageType
  value: any
}
