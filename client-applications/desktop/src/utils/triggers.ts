export default {
  // App triggers
  appReady: "appReady",
  numberWindows: "numberWindows",
  windowsAllClosed: "windowsAllClosed",

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

  // Protocol triggers
  protocolClose: "protocolClose",

  // Tray triggers
  showSignoutWindow: "showSignoutWindow",
  trayQuitAction: "trayQuitAction",
  trayRegionAction: "trayRegionAction",
  trayBillingPortalAction: "trayBillingPortalAction",

  // Flow triggers
  mandelboxFlowSuccess: "mandelboxFlowSuccess",
  mandelboxFlowFailure: "mandelboxFlowFailure",
  authFlowSuccess: "authFlowSuccess",
  authFlowFailure: "authFlowFailure",
}
