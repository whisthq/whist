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
  clearCacheAction: "clearCacheAction",

  // Tray triggers
  showSignoutWindow: "showSignoutWindow",
  trayQuitAction: "trayQuitAction",
  trayRegionAction: "trayRegionAction",
  trayBillingPortalAction: "trayBillingPortalAction",

  // Flow triggers
  protocolLaunchFlowSuccess: "protocolLaunchFlowSuccess",
  mandelboxFlowSuccess: "mandelboxFlowSuccess",
  mandelboxFlowFailure: "mandelboxFlowFailure",
  protocolCloseFlowSuccess: "protocolCloseFlowSuccess",
  protocolCloseFlowFailure: "protocolCloseFlowFailure",
  authFlowSuccess: "authFlowSuccess",
  authFlowFailure: "authFlowFailure",
}
