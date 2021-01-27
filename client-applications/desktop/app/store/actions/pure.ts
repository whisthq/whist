import { OperatingSystem, FractalApp } from "shared/types/client"
import { ExternalApp } from "store/reducers/types"

export const UPDATE_AUTH = "UPDATE_AUTH"
export const UPDATE_CONTAINER = "UPDATE_CONTAINER"
export const UPDATE_CLIENT = "UPDATE_CLIENT"
export const UPDATE_PAYMENT = "UPDATE_PAYEMNT"
export const UPDATE_LOADING = "UPDATE_LOADING"
export const UPDATE_APPS = "UPDATE_APPS"
export const UPDATE_ADMIN = "UPDATE_ADMIN"

export const RESET_STATE = "RESET_STATE"

export const updateAdmin = (body: {
    webserverUrl?: null | string
    taskArn?: null | string
    region?: null | string
    cluster?: null | string
}) => {
    return {
        type: UPDATE_ADMIN,
        body,
    }
}

export const updateAuth = (body: {
    username?: string | null
    candidateAccessToken?: string | null
    accessToken?: string | null
    refreshToken?: string | null
    loginWarning?: boolean
    loginMessage?: string | null
    name?: string | null
}) => {
    return {
        type: UPDATE_AUTH,
        body,
    }
}

export const updateContainer = (body: {
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
    pngFile: string | null
}) => {
    return {
        type: UPDATE_CONTAINER,
        body,
    }
}

export const updateClient = (body: {
    clientOS?: OperatingSystem
    region?: string
    dpi?: number
    apps?: FractalApp[]
    onboardApps?: FractalApp[]
}) => {
    return {
        type: UPDATE_CLIENT,
        body,
    }
}

export const updatePayment = (body: {
    accountLocked?: boolean
    promoCode?: string
}) => {
    return {
        type: UPDATE_PAYMENT,
        body,
    }
}

export const updateLoading = (body: {
    statusMessage?: string
    percentLoaded?: number
}) => {
    return {
        type: UPDATE_LOADING,
        body,
    }
}

export const updateApps = (body: {
    externalApps?: ExternalApp[]
    connectedApps?: string[]
    authenticated?: string | null
    disconnected?: string | null
    disconnectWarning?: string | null
}) => {
    /*
        Update information about external and connected applications in the global Redux state.

	Specify exactly one key in the action body when dispatching this action. Specifying
	multiple keys will trigger undefined behavior.

	If only the externalApps key is provided, apps.externalApps in the global Redux state gets
	the value associated with it.

	If only the connectedApps key is provided, apps.connectedApps in the global Redux state
	gets the value associated with it.

	If only the authenticated key is provided, the main reducer will ensure that
	apps.connectedApps in the global Redux state contains the associated value. The value will
	also be stored in apps.authenticated in the global Redux state.

	If only the disconnected key is provided, the main reducer will ensure that
	apps.connectedApps in the global Redux state does not contain the associated value. The
	value will also be stored in apps.disconnected in the global Redux state.

	If only the disconnectWarning key is provided, apps.disconnectWarning gets the value
	associated with it.

	Arguments:
	    body:
	        externalApps: A list of objects identical to the list of objects returned under the
		    "data" key from the GET /external_apps API endpoint.
		connectedApps: A list of strings identical to the list of strings returned under
		    the "app_names" key from the GET /connected_apps API endpoint.
		authenticated: The code name of the external application to which Fractal was just
		    granted access on behalf of a user (e.g. "dropbox" or "google_drive").
                disconnected: The code name of the external application to which Fractal's access
		    was just revoked.
		disconnectWarning: The code name of the external application to which a user just
		    tried to revoke Fractal's access.
    */
    return {
        type: UPDATE_APPS,
        body,
    }
}

export const resetState = () => {
    return {
        type: RESET_STATE,
    }
}
