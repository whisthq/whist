/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file auth.ts
 * @brief This file defines all the triggers recognized by the main thread.
 */

const WhistTrigger = {
  // App triggers
  appReady: "appReady",
  windowInfo: "windowInfo",
  windowsAllClosed: "windowsAllClosed",
  networkUnstable: "networkUnstable",
  reactivated: "reactivated", // When all windows are closed and the user clicks the dock icon

  // Auth triggers
  authInfo: "authInfo",

  // Payment triggers
  stripeAuthRefresh: "stripeAuthRefresh",
  stripePaymentError: "stripePaymentError",

  // Update triggers
  updateAvailable: "updateAvailable",
  updateNotAvailable: "updateNotAvailable",
  updateDownloaded: "updateDownloaded",
  updateError: "updateError",
  updateChecking: "updateChecking",
  downloadProgress: "downloadProgress",

  // Importer triggers
  cookiesImported: "cookiesImported",

  // IPC triggers
  eventIPC: "eventIPC",
  emitIPC: "emitIPC",

  // Persist triggers
  persisted: "persisted",
  notPersisted: "notPersisted",
  beginImport: "beginImport",
  storeDidChange: "storeDidChange",

  // Renderer triggers
  loginAction: "loginAction",
  signupAction: "signupAction",
  relaunchAction: "relaunchAction",
  clearCacheAction: "clearCacheAction",
  startNetworkAnalysis: "startNetworkAnalysis",

  // Omnibar triggers
  showSignoutWindow: "showSignoutWindow",
  showPaymentWindow: "showPaymentWindow",
  showSupportWindow: "showSupportWindow",
  showSpeedtestWindow: "showSpeedtestWindow",
  showLicenseWindow: "showLicenseWindow",
  showImportWindow: "showImportWindow",
  setDefaultBrowser: "setDefaultBrowser",
  restoreLastSession: "restoreLastSession",

  // Flow triggers
  mandelboxFlowStart: "mandelboxFlowStart",
  mandelboxFlowSuccess: "mandelboxFlowSuccess",
  mandelboxFlowFailure: "mandelboxFlowFailure",
  authFlowSuccess: "authFlowSuccess",
  authFlowFailure: "authFlowFailure",
  authRefreshSuccess: "authRefreshSuccess",
  checkPaymentFlowFailure: "checkPaymentFlowFailure",
  awsPingCached: "awsPingCached",
  awsPingRefresh: "awsPingRefresh",

  // System triggers (power, Internet)
  powerResume: "powerResume",
  powerSuspend: "powerSuspend",
  networkAnalysisEvent: "networkAnalysisEvent",

  // Protocol triggers
  protocolError: "protocolError",
}

export { WhistTrigger }
