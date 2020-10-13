export const DEFAULT = {
    auth: {
        username: null,
        accessToken: null,
        refreshToken: null,
        loginWarning: false,
    },
    container: {
        publicIP: null,
        container_id: null,
        cluster: null,
        port32262: null,
        port32263: null,
        port32273: null,
        location: null,
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
        statusMessage: 'Boot request sent to server',
        percentLoaded: 0,
    },
}
