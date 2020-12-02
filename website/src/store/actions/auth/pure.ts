import * as SharedAction from "store/actions/shared"

export const UPDATE_USER = "UPDATE_USER"
export const UPDATE_AUTH_FLOW = "UPDATE_AUTH_FLOW"

export const RESET_USER = "RESET_USER"

export function updateUser(body: {
    userID?: string
    name?: string
    accessToken?: string
    refreshToken?: string
    emailVerificationToken?: string
    emailVerified?: boolean
    canLogin?: boolean
    waitlistToken?: string
    usingGoogleLogin?: boolean
    cardBrand?: string | null
    cardLastFour?: string | null
    postalCode?: string
    plan?: string | null
}) {
    return {
        type: UPDATE_USER,
        body,
    }
}

export function updateAuthFlow(body: {
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
}) {
    return {
        type: UPDATE_AUTH_FLOW,
        body,
    }
}

export function resetUser() {
    return {
        type: RESET_USER,
    }
}

export function resetState() {
    return {
        type: SharedAction.RESET_STATE,
    }
}
