// App triggers
export const appReady = "appReady"
export const windowCreated = "windowCreated"

// Update triggers
export const updateAvailable = "updateAvailable"
export const updateNotAvailable = "updateNotAvailable"
export const downloadProgress = "downloadProgress"
export const updateDownloaded = "updateDownloaded"

// IPC triggers
export const eventIPC = "eventIPC"

// Persist triggers
export const persisted = "persisted"
export const notPersisted = "notPersisted"

// Renderer triggers
export const loginAction = "loginAction"
export const signupAction = "signupAction"
export const relaunchAction = "relaunchAction"

// Tray triggers
export const signoutAction = "signoutAction"
export const quitAction = "quitAction"

// Flow triggers
export const protocolLaunchFlowSuccess = "protocolLaunchFlowSuccess"
export const protocolLaunchFlowFailure = "protocolLaunchFlowFailure"
export const containerFlowSuccess = "containerFlowSuccess"
export const containerFlowFailure = "containerFlowFailure"
export const loginFlowSuccess = "loginFlowSuccess"
export const loginFlowFailure = "loginFlowFailure"
export const signupFlowSuccess = "signupFlowSuccess"
export const signupFlowFailure = "signupFlowFailure"
export const protocolCloseFlowSuccess = "protocolCloseFlowSuccess"
export const protocolCloseFlowFailure = "protocolCloseFlowFailure"
