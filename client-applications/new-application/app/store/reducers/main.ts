import * as MainAction from 'store/actions/main'

const DEFAULT = {
    username: '',
    public_ip: '',
    warning: false,
    os: '',
    container_id: '',
    cluster: '',
    port_32262: '',
    port_32263: '',
    port_32273: '',
    location: '',
    access_token:
        'eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpYXQiOjE2MDEwNzM1MjksIm5iZiI6MTYwMTA3MzUyOSwianRpIjoiMTc3MjBiNmMtMTNjOS00OWE0LWI1ZDItZTZhMjU2Y2Y5OTQ0IiwiaWRlbnRpdHkiOiJmcmFjdGFsLWFkbWluQGdtYWlsLmNvbSIsImZyZXNoIjpmYWxzZSwidHlwZSI6ImFjY2VzcyJ9.Fghwlh8_BaSiz1iVAxprbhRW2B_0aeydDZSHjN2ecqs',
    refresh_token:
        'eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9.eyJpYXQiOjE2MDEwNzM1MjksIm5iZiI6MTYwMTA3MzUyOSwianRpIjoiOWFmMmFiMmItMDk0OC00YTc3LTg1MjMtYTdhYTJmZGE3NmJhIiwiaWRlbnRpdHkiOiJmcmFjdGFsLWFkbWluQGdtYWlsLmNvbSIsInR5cGUiOiJyZWZyZXNoIn0.Ov_fWVh5020bjyO9YoA9bbftWJNvBzvTg00LUEf4rw4',
    attach_attempts: 0,
    account_locked: false,
    promo_code: '',
    status_message: 'Boot request sent to server',
    percent_loaded: 0,
}

export default function MainReducer(
    state = DEFAULT,
    action: {
        type: any
        username: any
        ip: any
        warning: any
        os: any
        container_id: any
        cluster: any
        port_32262: any
        port_32263: any
        port_32273: any
        location: any
        access_token: any
        refresh_token: any
        account_locked: any
        code: any
        status_message: any
        percent_loaded: any
    }
) {
    switch (action.type) {
        case MainAction.LOGOUT:
            return DEFAULT
        case MainAction.STORE_USERNAME:
            return {
                ...state,
                username: action.username,
            }
        case MainAction.STORE_IP:
            return {
                ...state,
                public_ip: action.ip,
                attach_attempts: state.attach_attempts + 1,
                ready_to_connect: true,
            }
        case MainAction.LOGIN_FAILED:
            return {
                ...state,
                warning: action.warning,
            }
        case MainAction.SET_OS:
            return {
                ...state,
                os: action.os,
            }
        case MainAction.STORE_RESOURCES:
            return {
                ...state,
                container_id: action.container_id,
                cluster: action.cluster,
                port_32262: action.port_32262,
                port_32263: action.port_32263,
                port_32273: action.port_32273,
                location: action.location,
            }
        case MainAction.STORE_JWT:
            return {
                ...state,
                access_token: action.access_token,
                refresh_token: action.refresh_token,
            }
        case MainAction.STORE_PAYMENT_INFO:
            return {
                ...state,
                account_locked: action.account_locked,
            }
        case MainAction.STORE_PROMO_CODE:
            return {
                ...state,
                promo_code: action.code,
            }
        case MainAction.CHANGE_STATUS_MESSAGE:
            return {
                ...state,
                status_message: action.status_message,
            }
        case MainAction.CHANGE_PERCENT_LOADED:
            return {
                ...state,
                percent_loaded: action.percent_loaded,
            }
        default:
            return state
    }
}
