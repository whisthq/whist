// ask the server to log me in with a google code
export const GOOGLE_LOGIN = "GOOGLE_LOGIN"
export const GOOGLE_SIGNUP = "GOOGLE_SIGNUP"

// ask the server to log me in
export const EMAIL_LOGIN = "EMAIL_LOGIN"
export const EMAIL_SIGNUP = "EMAIL_SIGNUP"

// ask the server to validate a token I have
export const VALIDATE_VERIFY_TOKEN = "VALIDATE_SIGNUP_TOKEN"
export const VALIDATE_RESET_TOKEN = "VALIDATE_RESET_TOKEN"

export const RESET_PASSWORD = "RESET_PASSWORD"
export const UPDATE_PASSWORD = "UPDATE_PASSWORD"

export const SEND_VERIFICATION_EMAIL = "SEND_VERIFICATION_EMAIL"

export const FETCH_PAYMENT_INFO = "FETCH_PAYMENT_INFO"

export const googleLogin = (code: any, rememberMe?: boolean) => {
    return {
        type: GOOGLE_LOGIN,
        code,
        rememberMe,
    }
}

export const emailLogin = (
    email: string,
    password: string,
    rememberMe?: boolean
) => {
    return {
        type: EMAIL_LOGIN,
        email,
        password,
        rememberMe,
    }
}

export const emailSignup = (
    email: string,
    name: string,
    password: string,
    rememberMe?: boolean
) => {
    return {
        type: EMAIL_SIGNUP,
        email,
        name,
        password,
        rememberMe,
    }
}

export const googleSignup = (code: string, rememberMe?: boolean) => {
    return {
        type: GOOGLE_SIGNUP,
        code,
        rememberMe,
    }
}

export const validateVerificationToken = (token: string) => {
    return {
        type: VALIDATE_VERIFY_TOKEN,
        token,
    }
}

export const validateResetToken = (token: string) => {
    return {
        type: VALIDATE_RESET_TOKEN,
        token,
    }
}

export const resetPassword = (
    password: string
) => {
    return {
        type: RESET_PASSWORD,
        password
    }
}

export const sendVerificationEmail = (
    email?: string,
    name?: string,
    token?: string
) => {
    return {
        type: SEND_VERIFICATION_EMAIL,
        email,
        name,
        token,
    }
}

export const updatePassword = (
    currentPassword: string,
    newPassword: string
) => {
    return {
        type: UPDATE_PASSWORD,
        currentPassword,
        newPassword,
    }
}

export const fetchPaymentInfo = (email: string) => {
    return {
        type: FETCH_PAYMENT_INFO,
        email,
    }
}
