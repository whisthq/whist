export const DEFAULT = {
    auth: {
        username: null,
        accessToken: null,
        refreshToken: null,
        loginWarning: false,
        loginMessage: null,
        name: null,
    },
    container: {
        publicIP: null,
        container_id: null,
        cluster: null,
        port32262: null,
        port32263: null,
        port32273: null,
        location: null,
        secretKey: null,
    },
    client: {
        os: null,
        region: null,
    },
    payment: {
        accountLocked: false,
        promoCode: null,
    },
    loading: {
        statusMessage: "Powering up your app",
        percentLoaded: 0,
    },
}
