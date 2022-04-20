export enum ContentScriptMessageType {
  POINTER_LOCK = "POINTER_LOCK",
  GEOLOCATION = "GEOLOCATION",
}

export enum NativeHostMessageType {
  ACTIVATE_TAB = "ACTIVATE_TAB",
  CREATE_NEW_TAB = "CREATE_NEW_TAB",
  DOWNLOAD_COMPLETE = "DOWNLOAD_COMPLETE",
  GEOLOCATION = "GEOLOCATION",
  NATIVE_HOST_EXIT = "NATIVE_HOST_EXIT",
  NATIVE_HOST_UPDATE = "NATIVE_HOST_UPDATE",
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
