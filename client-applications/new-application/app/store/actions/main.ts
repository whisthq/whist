export const LOGOUT = 'LOGOUT'
export const STORE_USERNAME = 'STORE_USERNAME'
export const STORE_IP = 'STORE_IP'
export const LOGIN_USER = 'LOGIN_USER'
export const GOOGLE_LOGIN = 'GOOGLE_LOGIN'
export const LOGIN_FAILED = 'LOGIN_FAILED'
export const SET_OS = 'SET_OS'
export const STORE_RESOURCES = 'STORE_RESOURCES'
export const FETCH_CONTAINER = 'FETCH_CONTAINER'
export const STORE_JWT = 'STORE_JWT'
export const STORE_PAYMENT_INFO = 'STORE_PAYMENT_INFO'
export const STORE_PROMO_CODE = 'STORE_PROMO_CODE'
export const CHANGE_STATUS_MESSAGE = 'CHANGE_STATUS_MESSAGE'
export const CHANGE_PERCENT_LOADED = 'CHANGE_PERCENT_LOADED'
export const DELETE_CONTAINER = 'DELETE_CONTAINER'
export const STORE_DIMENSIONS = 'STORE_DIMENSIONS'

export function loginUser(username: any, password: any) {
    return {
        type: LOGIN_USER,
        username,
        password,
    }
}

export function googleLogin(code: any) {
    return {
        type: GOOGLE_LOGIN,
        code,
    }
}

export function storeUsername(username: null) {
    return {
        type: STORE_USERNAME,
        username,
    }
}

export function storeIP(ip: string) {
    return {
        type: STORE_IP,
        ip,
    }
}

export function loginFailed(warning: boolean) {
    return {
        type: LOGIN_FAILED,
        warning,
    }
}

export function setOS(os: any) {
    return {
        type: SET_OS,
        os,
    }
}

export function storeResources(
    container_id: string,
    cluster: string,
    port_32262: string,
    port_32263: string,
    port_32273: string,
    location: string
) {
    return {
        type: STORE_RESOURCES,
        container_id,
        cluster,
        port_32262,
        port_32263,
        port_32273,
        location,
    }
}

export function fetchContainer() {
    return {
        type: FETCH_CONTAINER,
    }
}

export function storeJWT(access_token: any, refresh_token: any) {
    return {
        type: STORE_JWT,
        access_token,
        refresh_token,
    }
}

export function storePaymentInfo(account_locked: any) {
    return {
        type: STORE_PAYMENT_INFO,
        account_locked,
    }
}

export function storePromoCode(code: any) {
    return {
        type: STORE_PROMO_CODE,
        code,
    }
}

export function logout() {
    return {
        type: LOGOUT,
    }
}

export function changeStatusMessage(status_message: string) {
    return {
        type: CHANGE_STATUS_MESSAGE,
        status_message,
    }
}

export function changePercentLoaded(percent_loaded: number) {
    return {
        type: CHANGE_PERCENT_LOADED,
        percent_loaded,
    }
}

export function deleteContainer(username: string, container_id: string) {
    return {
        type: DELETE_CONTAINER,
        username,
        container_id,
    }
}
