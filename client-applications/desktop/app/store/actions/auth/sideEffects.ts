export const VALIDATE_ACCESS_TOKEN = "VALIDATE_ACCESS_TOKEN"

export const validateAccessToken = (accessToken: string) => {
    return {
        type: VALIDATE_ACCESS_TOKEN,
        accessToken,
    }
}
