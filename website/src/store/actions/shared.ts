export const RESET_STATE = "RESET_STATE"
export const REFRESH_STATE = "REFRESH_STATE"

export const resetState = () => {
    return {
        type: RESET_STATE,
    }
}

export const refreshState = () => {
    return {
        type: REFRESH_STATE,
    }
}
