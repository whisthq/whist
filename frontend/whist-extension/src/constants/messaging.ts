export enum ContentScriptMessageType {
  AUTH_SUCCESS = "AUTH_SUCCESS",
  CLOSEST_AWS_REGIONS = "CLOSEST_AWS_REGIONS",
  HOST_SPINUP_SUCCESS = "HOST_SPINUP_SUCCESS",
  MANDELBOX_ASSIGN_SUCCESS = "MANDELBOX_ASSIGN_SUCCESS",
  OPEN_GOOGLE_AUTH = "OPEN_GOOGLE_AUTH",
}

export interface ContentScriptMessage {
  type: ContentScriptMessageType
  value: any
}
