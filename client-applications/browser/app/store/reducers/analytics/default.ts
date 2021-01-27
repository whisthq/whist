export type Timer = {
    appOpened?: number
    appDoneLoading?: number
    createContainerRequestSent?: number
    protocolLaunched?: number
    protocolClosed?: number
}

export type AnalyticsDefault = {
    timer: Timer
}

export const DEFAULT: AnalyticsDefault = {
    timer: {
        appOpened: 0,
        appDoneLoading: 0,
        createContainerRequestSent: 0,
        protocolLaunched: 0,
        protocolClosed: 0,
    },
}
