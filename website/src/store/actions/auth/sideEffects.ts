export const GOOGLE_LOGIN = "GOOGLE_LOGIN"
export const EMAIL_LOGIN = "EMAIL_LOGIN"
export const EMAIL_SIGNUP = "EMAIL_SIGNUP"

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
