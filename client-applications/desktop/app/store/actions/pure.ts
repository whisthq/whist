export const UPDATE_AUTH = "UPDATE_AUTH"
export const UPDATE_CONTAINER = "UPDATE_CONTAINER"
export const UPDATE_CLIENT = "UPDATE_CLIENT"
export const UPDATE_PAYMENT = "UPDATE_PAYEMNT"
export const UPDATE_LOADING = "UPDATE_LOADING"

export const RESET_STATE = "RESET_STATE"

export function updateAuth(body: {
    username?: string
    accessToken?: string
    refreshToken?: string
    loginWarning?: boolean
    loginMessage?: string
    name?: string
}) {
    return {
        type: UPDATE_AUTH,
        body,
    }
}

export function updateContainer(body: {
    publicIP: string
    containerID: string
    cluster: string
    port32262: string
    port32263: string
    port32273: string
    location: string
    secretKey: string
    desiredAppID: string
    currentAppID: string
    launches: number
    launchURL: string
}) {
    return {
        type: UPDATE_CONTAINER,
        body,
    }
}

export function updateClient(body: {
    os?: string
    region?: string
    dpi?: number
}) {
    return {
        type: UPDATE_CLIENT,
        body,
    }
}

export function updatePayment(body: {
    accountLocked?: boolean
    promoCode?: string
}) {
    return {
        type: UPDATE_PAYMENT,
        body,
    }
}

export function updateLoading(body: {
    statusMessage: string
    percentLoaded?: number
}) {
    return {
        type: UPDATE_LOADING,
        body,
    }
}

export function resetState() {
    return {
        type: RESET_STATE,
    }
}
