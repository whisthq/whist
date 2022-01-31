/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file auth.ts
 * @brief This file defines all the triggers recognized by the main thread.
 */

const WhistTrigger = {
  // App triggers
  appReady: "appReady",
  electronWindowsAllClosed: "electronWindowsAllClosed",
  networkUnstable: "networkUnstable",
  reactivated: "reactivated", // When all windows are closed and the user clicks the dock icon
  userRequestedQuit: "userRequestedQuit",

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

  // Protocol triggers
  protocol: "protocol",
  protocolClosed: "protocolClosed",
  protocolStdoutData: "protocolStdoutData",
  protocolStdoutEnd: "protocolStdoutEnd",
  protocolConnection: "protocolConnection",

  // Renderer triggers
  loginAction: "loginAction",
  signupAction: "signupAction",
  relaunchAction: "relaunchAction",
  clearCacheAction: "clearCacheAction",
  startNetworkAnalysis: "startNetworkAnalysis",
  getOtherBrowserWindows: "getOtherBrowserWindows",
  importTabs: "importTabs",

  // Omnibar triggers
  showAuthWindow: "showAuthWindow",
  showSignoutWindow: "showSignoutWindow",
  showPaymentWindow: "showPaymentWindow",
  showSupportWindow: "showSupportWindow",
  showSpeedtestWindow: "showSpeedtestWindow",
  showLicenseWindow: "showLicenseWindow",
  showImportWindow: "showImportWindow",
  showRestoreTabsWindow: "showRestoreTabsWindow",
  setDefaultBrowser: "setDefaultBrowser",
  restoreLastSession: "restoreLastSession",
  closeSupportWindow: "closeSupportWindow",

  // Flow triggers
  mandelboxFlowStart: "mandelboxFlowStart",
  mandelboxFlowSuccess: "mandelboxFlowSuccess",
  mandelboxFlowFailure: "mandelboxFlowFailure",
  mandelboxFlowTimeout: "mandelboxFlowTimeout",
  authFlowSuccess: "authFlowSuccess",
  authFlowFailure: "authFlowFailure",
  authRefreshSuccess: "authRefreshSuccess",
  checkPaymentFlowSuccess: "checkPaymentFlowSuccess",
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
