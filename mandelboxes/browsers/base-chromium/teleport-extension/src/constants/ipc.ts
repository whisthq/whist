export enum ContentScriptMessageType {
  POINTER_LOCK = "POINTER_LOCK",
}

export enum NativeHostMessageType {
  DOWNLOAD_COMPLETE = "DOWNLOAD_COMPLETE",
  GEOLOCATION = "GEOLOCATION",
  NATIVE_HOST_EXIT = "NATIVE_HOST_EXIT",
  NATIVE_HOST_UPDATE = "NATIVE_HOST_UPDATE",
  POINTER_LOCK = "POINTER_LOCK",
  KEEP_ALIVE = "KEEP_ALIVE"
}

export interface ContentScriptMessage {
  type: ContentScriptMessageType
  value: any
}

export interface NativeHostMessage {
  type: NativeHostMessageType
  value: any
}
