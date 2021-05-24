export default {
  // App triggers
  appReady: "appReady",
  windowCreated: "windowCreated",

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

  // Flow triggers
  protocolLaunchFlowSuccess: "protocolLaunchFlowSuccess",
  protocolLaunchFlowFailure: "protocolLaunchFlowFailure",
  containerFlowSuccess: "containerFlowSuccess",
  containerFlowFailure: "containerFlowFailure",
  loginFlowSuccess: "loginFlowSuccess",
  loginFlowFailure: "loginFlowFailure",
  signupFlowSuccess: "signupFlowSuccess",
  signupFlowFailure: "signupFlowFailure",
  protocolCloseFlowSuccess: "protocolCloseFlowSuccess",
  protocolCloseFlowFailure: "protocolCloseFlowFailure",
}
