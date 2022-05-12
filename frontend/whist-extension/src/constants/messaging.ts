export enum ContentScriptMessageType {
  OPEN_GOOGLE_AUTH = "OPEN_GOOGLE_AUTH",
}

export enum WorkerMessageType {
  AUTH_SUCCESS = "AUTH_SUCCESS",
  CLOSEST_AWS_REGIONS = "CLOSEST_AWS_REGIONS",
  HOST_SPINUP_SUCCESS = "HOST_SPINUP_SUCCESS",
  MANDELBOX_ASSIGN_SUCCESS = "MANDELBOX_ASSIGN_SUCCESS",
}

export interface ContentScriptMessage {
  type: ContentScriptMessageType
  value: any
}
