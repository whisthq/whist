export type User = {
    userID?: string | null
    name?: string | null 
    accessToken?: string | null
    refreshToken?: string | null 
    emailVerificationToken?: string | null
    emailVerified?: boolean
    usingGoogleLogin?: boolean
}

export type AuthFlow = {
    loginWarning?: string | null 
    signupWarning?: string | null 
    signupSuccess?: boolean
    verificationStatus?: string | null
    forgotStatus?: string | null
    verificationEmailsSent?: number
    forgotEmailsSent?: number
    resetTokenStatus?: string | null 
    passwordResetEmail?: string | null 
    passwordResetToken?: string | null
    passwordVerified?: string | null 
    callback?: string | undefined
    token?: string | null
    url?: string | null
}

export const DEFAULT = {
    user: {
        userID: null,
        name: null,
        accessToken: null, 
        refreshToken: null,
        emailVerificationToken: null,
        emailVerified: false
    },
    authFlow: {
        // error message
        loginWarning: null, // text to tell the user if they messed up logging in (primarily used as warning)
        signupWarning: null, // text to tell the user if they messed up signing up (primarily used as warning)
        signupSuccess: false,
        verificationStatus: null, // null | "success" | "failed"
        forgotStatus: null, // text/status info about what happend to the forgot email request sent (status and warning)
        // verify emails
        verificationEmailsSent: 0, // how many emails we send
        // reset password
        forgotEmailsSent: 0,
        resetTokenStatus: null, // null | "verified" | "invalid", may want to handle expired tokens in the future
        passwordResetEmail: null, // forgot password email
        passwordResetToken: null,
        passwordVerified: null, // null | "success" | "failed"
        callback: null,
        // fields for testing reset password
        token: null, //reset password token
        url: null, // should be dev.fractal.co
    },
}

export default DEFAULT
