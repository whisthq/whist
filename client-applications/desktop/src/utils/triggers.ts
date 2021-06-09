export default {
  // App triggers
  appReady: "appReady",
  windowCreated: "windowCreated",
  willQuit: "willQuit",

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

  // Protocol triggers
  childProcessSpawn: "childProcessSpawn",
  childProcessClose: "childProcessClose",

  // Renderer triggers
  loginAction: "loginAction",
  signupAction: "signupAction",
  relaunchAction: "relaunchAction",
  clearCacheAction: "clearCacheAction",

  // Tray triggers
  showSignoutWindow: "showSignoutWindow",
  trayQuitAction: "trayQuitAction",
  trayRegionAction: "trayRegionAction",

  // Flow triggers
  mandelboxFlowSuccess: "mandelboxFlowSuccess",
  mandelboxFlowFailure: "mandelboxFlowFailure",
  authFlowSuccess: "authFlowSuccess",
  authFlowFailure: "authFlowFailure",
}
