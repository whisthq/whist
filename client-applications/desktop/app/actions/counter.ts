export const LOGOUT = "LOGOUT";
export const STORE_USERNAME = "STORE_USERNAME";
export const STORE_IP = "STORE_IP";
export const STORE_IS_USER = "STORE_IS_USER";
export const TRACK_USER_ACTIVITY = "TRACK_USER_ACTIVITY";
export const LOGIN_USER = "LOGIN_USER";
export const GOOGLE_LOGIN = "GOOGLE_LOGIN";
export const LOGIN_FAILED = "LOGIN_FAILED";
export const CALCULATE_DISTANCE = "CALCULATE_DISTANCE";
export const STORE_DISTANCE = "STORE_DISTANCE";
export const SEND_FEEDBACK = "SEND_FEEDBACK";
export const RESET_FEEDBACK = "RESET_FEEDBACK";
export const SET_OS = "SET_OS";
export const ASK_FEEDBACK = "ASK_FEEDBACK";
export const CHANGE_WINDOW = "CHANGE_WINDOW";
export const LOGIN_STUDIO = "LOGIN_STUDIO";
export const PING_IPINFO = "PING_IPINFO";
export const STORE_IPINFO = "STORE_IPINFO";
export const FETCH_COMPUTERS = "FETCH_COMPUTERS";
export const STORE_COMPUTERS = "STORE_COMPUTERS";
export const FETCH_DISK = "FETCH_DISK";
export const FETCH_DISK_STATUS = "FETCH_DISK_STATUS";
export const STORE_RESOURCES = "STORE_RESOURCES";
export const ATTACH_DISK = "ATTACH_DISK";
export const FETCH_VM = "FETCH_VM";
export const STORE_JWT = "STORE_JWT";
export const CREATE_DISK = "CREATE_DISK";
export const STORE_PAYMENT_INFO = "STORE_PAYMENT_INFO";
export const STORE_PROMO_CODE = "STORE_PROMO_CODE";
export const RESTART_PC = "RESTART_PC";
export const VM_RESTARTED = "VM_RESTARTED";
export const SEND_LOGS = "SEND_LOGS";
export const CHANGE_STATUS_MESSAGE = "CHANGE_STATUS_MESSAGE";
export const UPDATE_FOUND = "UPDATE_FOUND";
export const READY_TO_CONNECT = "READY_TO_CONNECT";
export const GET_VERSION = "GET_VERSION";
export const SET_VERSION = "SET_VERSION";

export function loginUser(username: any, password: any) {
    return {
        type: LOGIN_USER,
        username,
        password
    };
}

export function googleLogin(code: any) {
    return {
        type: GOOGLE_LOGIN,
        code
    };
}

export function storeUsername(username: null) {
    return {
        type: STORE_USERNAME,
        username
    };
}

export function storeIP(ip: string) {
    return {
        type: STORE_IP,
        ip
    };
}

export function storeIsUser(isUser: any) {
    return {
        type: STORE_IS_USER,
        isUser
    };
}

export function trackUserActivity(logon: any) {
    return {
        type: TRACK_USER_ACTIVITY,
        logon
    };
}

export function loginFailed(warning: boolean) {
    return {
        type: LOGIN_FAILED,
        warning
    };
}

export function calculateDistance(public_ip: any) {
    return {
        type: CALCULATE_DISTANCE,
        public_ip
    };
}

export function storeDistance(distance: any) {
    return {
        type: STORE_DISTANCE,
        distance
    };
}

export function sendFeedback(feedback, feedback_type) {
    return {
        type: SEND_FEEDBACK,
        feedback,
        feedback_type
    };
}

export function resetFeedback(reset: boolean) {
    return {
        type: RESET_FEEDBACK,
        reset
    };
}

export function setOS(os: any) {
    return {
        type: SET_OS,
        os
    };
}

export function askFeedback(ask: boolean) {
    return {
        type: ASK_FEEDBACK,
        ask
    };
}

export function changeWindow(window: string) {
    return {
        type: CHANGE_WINDOW,
        window
    };
}

export function loginStudio(username: any, password: any) {
    return {
        type: LOGIN_STUDIO,
        username,
        password
    };
}

export function pingIPInfo(id: string) {
    return {
        type: PING_IPINFO,
        id
    };
}

export function storeIPInfo(payload: any, id: any) {
    return {
        type: STORE_IPINFO,
        payload,
        id
    };
}

export function fetchComputers() {
    return {
        type: FETCH_COMPUTERS
    };
}

export function storeComputers(payload: any) {
    return {
        type: STORE_COMPUTERS,
        payload
    };
}

export function fetchDisk(username: any) {
    return {
        type: FETCH_DISK,
        username
    };
}

export function fetchDiskStatus(status: boolean) {
    return {
        type: FETCH_DISK_STATUS,
        status
    };
}

export function storeResources(disk: string, vm: string, location: string) {
    console.log(`${disk} ${vm} ${location}`);
    return {
        type: STORE_RESOURCES,
        disk,
        vm,
        location
    };
}

export function attachDisk() {
    return {
        type: ATTACH_DISK
    };
}

export function fetchVM(id: any) {
    return {
        type: FETCH_VM,
        id
    };
}

export function storeJWT(access_token: any, refresh_token: any) {
    return {
        type: STORE_JWT,
        access_token,
        refresh_token
    };
}

export function createDisk() {
    return {
        type: CREATE_DISK
    };
}

export function storePaymentInfo(account_locked: any) {
    return {
        type: STORE_PAYMENT_INFO,
        account_locked
    };
}

export function storePromoCode(code: any) {
    return {
        type: STORE_PROMO_CODE,
        code
    };
}

export function logout() {
    return {
        type: LOGOUT
    };
}

export function restartPC() {
    return {
        type: RESTART_PC
    };
}

export function vmRestarted(status: number) {
    console.log("VM STATUS IS NOW");
    console.log(status);
    return {
        type: VM_RESTARTED
    };
}

export function sendLogs(connection_id: number, logs: any) {
    return {
        type: SEND_LOGS,
        connection_id,
        logs
    };
}

export function changeStatusMessage(status_message: string) {
    return {
        type: CHANGE_STATUS_MESSAGE,
        status_message
    };
}

export function updateFound(update: any) {
    return {
        type: UPDATE_FOUND,
        update
    };
}

export function readyToConnect(update: boolean) {
    return {
        type: READY_TO_CONNECT,
        update
    };
}

export function getVersion() {
    return {
        type: GET_VERSION
    };
}

export function setVersion(versions: any) {
    return {
        type: SET_VERSION,
        versions
    };
}
