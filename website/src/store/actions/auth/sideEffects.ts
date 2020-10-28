// ask the server to log me in with a google code
export const GOOGLE_LOGIN = "GOOGLE_LOGIN"

// ask the server to log me in
export const EMAIL_LOGIN = "EMAIL_LOGIN"
export const EMAIL_SIGNUP = "EMAIL_SIGNUP"

// ask the server to validate a token I have
export const VALIDATE_VERIFY_TOKEN = "VALIDATE_SIGNUP_TOKEN"
export const VALIDATE_RESET_TOKEN = "VALIDATE_RESET_TOKEN"

export const FORGOT_PASSWORD = "FORGOT_PASSWORD"
export const RESET_PASSWORD = "RESET_PASSWORD"

export const SEND_VERIFICATION_EMAIL = "SEND_VERIFICATION_EMAIL"

export function googleLogin(code: any) {
    return {
        type: GOOGLE_LOGIN,
        code,
    }
}

export function emailLogin(email: string, password: string) {
    return {
        type: EMAIL_LOGIN,
        email,
        password,
    }
}

export function emailSignup(email: string, password: string) {
    return {
        type: EMAIL_SIGNUP,
        email,
        password,
    }
}

export function validateVerificationToken(token: any) {
    return {
        type: VALIDATE_VERIFY_TOKEN,
        token,
    }
}

export function validateResetToken(token: any) {
    return {
        type: VALIDATE_RESET_TOKEN,
        token,
    }
}

export function forgotPassword(username: string) {
    return {
        type: FORGOT_PASSWORD,
        username,
    }
}

export function resetPassword(username: string, password: string) {
    return {
        type: RESET_PASSWORD,
        username,
        password,
    }
}

export function sendVerificationEmail(email: string, token: string) {
    return {
        type: SEND_VERIFICATION_EMAIL,
        email,
        token,
    }
}
