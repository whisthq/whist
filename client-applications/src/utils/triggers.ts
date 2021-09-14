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
  networkUnstable: "networkUnstable",

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

  // IPC triggers
  eventIPC: "eventIPC",

  // Persist triggers
  persisted: "persisted",
  notPersisted: "notPersisted",
  onboarded: "onboarded",

  // Renderer triggers
  loginAction: "loginAction",
  signupAction: "signupAction",
  relaunchAction: "relaunchAction",
  clearCacheAction: "clearCacheAction",
  exitTypeformSubmitted: "exitTypeformSubmitted",
  onboardingTypeformSubmitted: "onboardingTypeformSubmitted",
  installUpdate: "installUpdate",

  // Tray triggers
  showSignoutWindow: "showSignoutWindow",
  trayRegionAction: "trayRegionAction",
  trayFeedbackAction: "trayFeedbackAction",
  trayBugAction: "trayBugAction",
  trayAutolaunchAction: "trayAutolaunchAction",
  showPaymentWindow: "showPaymentWindow",

  // Flow triggers
  mandelboxFlowSuccess: "mandelboxFlowSuccess",
  mandelboxFlowFailure: "mandelboxFlowFailure",
  authFlowSuccess: "authFlowSuccess",
  authFlowFailure: "authFlowFailure",
  authRefreshSuccess: "authRefreshSuccess",
  checkPaymentFlowSuccess: "checkPaymentFlowSuccess",
  checkPaymentFlowFailure: "checkPaymentFlowFailure",
  configFlowSuccess: "configFlowSuccess",
  monitoringFlowSuccess: "monitoringFlowSuccess",

  // Powermonitor triggers
  powerResume: "powerResume",
  powerSuspend: "powerSuspend",
}
