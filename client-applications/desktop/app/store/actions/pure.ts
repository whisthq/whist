export const UPDATE_AUTH = "UPDATE_AUTH"
export const UPDATE_CONTAINER = "UPDATE_CONTAINER"
export const UPDATE_CLIENT = "UPDATE_CLIENT"
export const UPDATE_PAYMENT = "UPDATE_PAYEMNT"
export const UPDATE_LOADING = "UPDATE_LOADING"
export const UPDATE_APPS = "UPDATE_APPS"
export const UPDATE_ADMIN = "UPDATE_ADMIN"

export const RESET_STATE = "RESET_STATE"

export function updateAdmin(body: {
    webserver_url?: null | string
    task_arn?: null | string
    region?: null | string
    cluster?: null | string
}) {
    return {
        type: UPDATE_ADMIN,
        body,
    }
}

export function updateAuth(body: {
    username?: string | null
    candidateAccessToken?: string | null
    accessToken?: string | null
    refreshToken?: string | null
    loginWarning?: boolean
    loginMessage?: string | null
    name?: string | null
}) {
    return {
        type: UPDATE_AUTH,
        body,
    }
}

export function updateContainer(body: {
    publicIP: string | null
    containerID: string | null
    cluster: string | null
    port32262: string | null
    port32263: string | null
    port32273: string | null
    location: string | null
    secretKey: string | null
    desiredAppID: string | null
    currentAppID: string | null
    launches: number
    launchURL: string | null
}) {
    return {
        type: UPDATE_CONTAINER,
        body,
    }
}

export function updateClient(body: {
    clientOS?: string
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

export function updateApps(body: {
    notInstalled?: string[]
    installing?: string[]
    installed?: string[]
}) {
    return {
        type: UPDATE_APPS,
        body,
    }
}

export function resetState() {
    return {
        type: RESET_STATE,
    }
}
