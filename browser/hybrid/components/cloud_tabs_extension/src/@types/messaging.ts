export enum ContentScriptMessageType {
  INJECT_WHIST_TAG,
}

export enum PopupMessageType {
  STREAM_TAB = "POPUP_STREAM_TAB", // Fired by the popup when the user clicks the stream tab toggle
  FETCH_POPUP_DATA = "POPUP_FETCH_DATA", // Fired by the popup to request data from the background script
  SEND_POPUP_DATA = "POPUP_SEND_DATA", // Fired by the background script to send updated data to the popup
  SAVE_URL = "POPUP_SAVE_URL", // Fired by the popup when the user clicks the "always stream this URL" checkbox
  OPEN_LOGIN_PAGE = "POPUP_OPEN_LOGIN_PAGE", // Fired by the popup when the user clicks the login button
  OPEN_INTERCOM = "POPUP_OPEN_INTERCOM", // Fired by the popup when the user clicks the help button
  INVITE_CODE = "POPUP_INVITE_CODE", // Fired by the popup when the user enters an invite code
}

export enum HelpScreenMessageType {
  HELP_SCREEN_OPENED = "HELP_SCREEN_OPENED", // Fired by the help page when it is opened
  HELP_SCREEN_USER_EMAIL = "HELP_SCREEN_USER_EMAIL", // Fired by the background script to send the user's email to the help page
}

export interface ContentScriptMessage {
  type: ContentScriptMessageType
  value: any
}

export interface PopupMessage {
  type: PopupMessageType
  value: any
}
