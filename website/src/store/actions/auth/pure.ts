export const UPDATE_USER = "UPDATE_USER"
export const UPDATE_AUTH_FLOW = "UPDATE_AUTH_FLOW"

export const RESET_USER = "RESET_USER"

export function updateUser(body: {
    user_id?: string
    name?: string
    accessToken?: string
    refreshToken?: string
    emailVerificationToken?: string
    emailVerified?: boolean
}) {
    //console.log("update user action")
    return {
        type: UPDATE_USER,
        body,
    }
}

export function updateAuthFlow(body: {
    mode?: string
    loginWarning?: string
    signupWarning?: string
    signupSuccess?: boolean
    forgotStatus?: string
    verificationEmailsSent?: number
    verificationAttemptsExecuted?: number
    forgotEmailsSent?: number
    resetTokenStatus?: string | null
    resetDone?: boolean
}) {
    return {
        type: UPDATE_AUTH_FLOW,
        body,
    }
}

export function resetUser() {
    return {
        type: RESET_USER
    }
}