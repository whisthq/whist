export const GOOGLE_LOGIN = "GOOGLE_LOGIN"
export const SET_EMAIL = "SET_EMAIL"
export const LOGOUT = "LOGOUT"

export const googleLogin = (email: string, points: number) => {
    return {
        type: GOOGLE_LOGIN,
        email,
        points,
    }
}

export const logout = () => {
    return {
        type: LOGOUT,
    }
}
