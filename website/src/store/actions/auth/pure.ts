import * as SharedAction from "store/actions/shared"

export const UPDATE_USER = "UPDATE_USER"
export const UPDATE_AUTH_FLOW = "UPDATE_AUTH_FLOW"

export const RESET_USER = "RESET_USER"

export const updateUser = (body: {
    userID?: string
    name?: string
    accessToken?: string
    refreshToken?: string
    configToken?: string
    emailVerificationToken?: string
    emailVerified?: boolean
    usingGoogleLogin?: boolean
}) => {
    return {
        type: UPDATE_USER,
        body,
    }
}

export const updateAuthFlow = (body: {
    mode?: string
    loginWarning?: string | null
    signupWarning?: string | null
    signupSuccess?: boolean
    verificationStatus?: string | null
    forgotStatus?: string | null
    verificationEmailsSent?: number
    forgotEmailsSent?: number
    resetTokenStatus?: string | null
    resetDone?: boolean
    passwordResetEmail?: string | null
    passwordResetToken?: string | null
    passwordVerified?: string | null
    callback?: string
    token?: string
    url?: string
}) => {
    return {
        type: UPDATE_AUTH_FLOW,
        body,
    }
}

export const resetUser = () => {
    return {
        type: RESET_USER,
    }
}

export const resetState = () => {
    return {
        type: SharedAction.RESET_STATE,
    }
}
