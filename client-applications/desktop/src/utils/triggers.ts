/**
 * Copyright Fractal Computers, Inc. 2021
 * @file auth.ts
 * @brief This file defines all the triggers recognized by the main thread.
 */

export default {
  // App triggers
  appReady: "appReady",
  windowInfo: "windowInfo",
  windowsAllClosed: "windowsAllClosed",

  // Auth triggers
  authInfo: "authInfo",

  // Update triggers
  updateAvailable: "updateAvailable",
  updateNotAvailable: "updateNotAvailable",
  downloadProgress: "downloadProgress",
  updateDownloaded: "updateDownloaded",
  updateError: "updateError",
  updateChecking: "updateChecking",

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
  trayRegionAction: "trayRegionAction",
  trayFeedbackAction: "trayFeedbackAction",
  showPaymentWindow: "showPaymentWindow",

  // Flow triggers
  mandelboxFlowSuccess: "mandelboxFlowSuccess",
  mandelboxFlowFailure: "mandelboxFlowFailure",
  authFlowSuccess: "authFlowSuccess",
  authFlowFailure: "authFlowFailure",
  authRefreshSuccess: "authRefreshSuccess",
}
