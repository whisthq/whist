export enum ContentScriptMessageType {
  OPEN_GOOGLE_AUTH = "OPEN_GOOGLE_AUTH",
  AUTH_SUCCESS = "AUTH_SUCCESS",
}

export interface ContentScriptMessage {
  type: ContentScriptMessageType
  value: any
}
