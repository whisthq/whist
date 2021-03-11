export type User = {
    userID?: string
    name?: string
    accessToken?: string
    refreshToken?: string
    configToken?: string
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
        configToken: "",
    },
    authFlow: {
        failures: 0,
        loginToken: "",
    },
}
