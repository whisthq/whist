export enum ContentScriptMessageType {
  INJECT_WHIST_TAG,
}

export enum PopupMessageType {
  STREAM_TAB, // Fired by the popup when the user clicks the stream tab toggle
  FETCH_POPUP_DATA, // Fired by the popup to request data from the background script
  SEND_POPUP_DATA, // Fired by the background script to send updated data to the popup
  SAVE_URL, // Fired by the popup when the user clicks the "always stream this URL" checkbox
  OPEN_LOGIN_PAGE, // Fired by the popup when the user clicks the login button
  OPEN_INTERCOM, // Fired by the popup when the user clicks the help button
  INVITE_CODE, // Fired by the popup when the user enters an invite code
}

export interface ContentScriptMessage {
  type: ContentScriptMessageType
  value: any
}

export interface PopupMessage {
  type: PopupMessageType
  value: any
}
