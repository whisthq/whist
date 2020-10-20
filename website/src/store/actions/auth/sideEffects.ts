export const GOOGLE_LOGIN = "GOOGLE_LOGIN"
export const EMAIL_LOGIN = "EMAIL_LOGIN"

export function googleLogin(code: any) {
    return {
        type: GOOGLE_LOGIN,
        code,
    }
}

export function emailLogin(email: string, password: string) {
    console.log("action dispatched")
    return {
        type: EMAIL_LOGIN,
        email,
        password,
    }
}
