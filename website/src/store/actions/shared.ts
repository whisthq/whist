export const RESET_STATE = "RESET_STATE"
export const REFRESH_STATE = "REFRESH_STATE"

export function resetState() {
    return {
        type: RESET_STATE,
    }
}

export function refreshState() {
    return {
        type: REFRESH_STATE,
    }
}
