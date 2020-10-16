export const GOOGLE_LOGIN = "GOOGLE_LOGIN"

export function googleLogin(user_id: string, points: number) {
    return {
        type: GOOGLE_LOGIN,
        user_id,
        points,
    }
}
