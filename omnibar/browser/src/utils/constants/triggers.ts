export enum FractalTrigger {
  // App triggers
  appReady = "appReady",
  openUrlDefault = "openUrlDefault", // Fired when Fractal is used as a default browser for a link

  // IPC triggers
  eventIPC = "eventIPC",

  // Renderer triggers
  changeViewUrl = "changeViewUrl", // If we want to change the URL of the current view
  createView = "createView", // If we want to create a new view
  displayView = "displayView", // If we want to put a view in the foreground
  closeView = "closeView", // If we want to destroy a view

  // Window triggers
  browserWindowCreated = "browserWindowCreated",
  browserWindowClosed = "browserWindowClosed",
  browserWindowWillClose = "browserWindowWillClose",
  newWindowRedirected = "newWindowRedirected",
  windowsAllClosed = "windowsAllClosed",

  // Shortcut triggers
  shortcutFired = "shortcutFired",

  // View triggers
  viewCreated = "viewCreated",
  viewDisplayed = "viewDisplayed",
  viewDestroyed = "viewDestroyed",
  viewFailed = "viewFailed",
  viewTitleChanged = "viewTitleChanged",

  // Finder triggers
  finderDown = "finderDown",
  finderUp = "finderUp",
  finderClose = "finderClose",
  finderEnterPressed = "finderEnterPressed",

  // Onboarding triggers
  importChromeConfig = "importChromeConfig",
  doneOnboarding = "doneOnboarding",

  // Persist triggers
  persistDidChange = "persistDidChange",
}
