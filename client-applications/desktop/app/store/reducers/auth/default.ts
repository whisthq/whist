export type User = {
    userID?: string
    name?: string
    accessToken?: string
    refreshToken?: string
}

export type AuthFlow = {
    failures?: number
    loginToken?: string
}

export type AuthDefault = {
    user: User
    authFlow: AuthFlow
}

export const DEFAULT: AuthDefault = {
    user: {
        // identifiers
        userID: "",
        name: "",
        accessToken: "",
        refreshToken: "",
    },
    authFlow: {
        failures: 0,
        loginToken: "",
    },
}
