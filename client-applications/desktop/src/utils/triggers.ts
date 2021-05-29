export default {
  // App triggers
  appReady: "appReady",
  windowCreated: "windowCreated",

  // Auth triggers
  authInfo: "authInfo",

  // Update triggers
  updateAvailable: "updateAvailable",
  updateNotAvailable: "updateNotAvailable",
  downloadProgress: "downloadProgress",
  updateDownloaded: "updateDownloaded",

  // IPC triggers
  eventIPC: "eventIPC",

  // Persist triggers
  persisted: "persisted",
  notPersisted: "notPersisted",

  // Renderer triggers
  loginAction: "loginAction",
  signupAction: "signupAction",
  relaunchAction: "relaunchAction",

  // Tray triggers
  signoutAction: "signoutAction",
  quitAction: "quitAction",
  regionAction: "regionAction",

  // Flow triggers
  protocolLaunchFlowSuccess: "protocolLaunchFlowSuccess",
  protocolLaunchFlowFailure: "protocolLaunchFlowFailure",
  mandelboxFlowSuccess: "mandelboxFlowSuccess",
  mandelboxFlowFailure: "mandelboxFlowFailure",
  protocolCloseFlowSuccess: "protocolCloseFlowSuccess",
  protocolCloseFlowFailure: "protocolCloseFlowFailure",
  authFlowSuccess: "authFlowSuccess",
  authFlowFailure: "authFlowFailure",
}
