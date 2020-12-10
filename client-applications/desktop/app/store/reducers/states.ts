export const DEFAULT = {
    auth: {
        username: null,
        candidateAccessToken: null,
        accessToken: null,
        refreshToken: null,
        loginWarning: false,
        loginMessage: null,
        name: null,
    },
    container: {
        publicIP: null,
        containerID: null,
        cluster: null,
        port32262: null,
        port32263: null,
        port32273: null,
        location: null,
        secretKey: null,
        currentAppID: null,
        desiredAppID: null,
        launches: 0,
        launchURL: null,
        pngFile: null,
    },
    client: {
        clientOS: null,
        region: null,
        dpi: null,
        apps: [],
        onboardApps: [],
    },
    payment: {
        accountLocked: false,
        promoCode: null,
    },
    loading: {
        statusMessage: "Powering up your app",
        percentLoaded: 0,
    },
    apps: {
        external: [], // all external applications (example entry: {id: "google_drive", display_name" "Google Drive", ...})
        connected: [], // all external application ids that the user has connected their account to
    },
    admin: {
        webserverUrl: "dev",
        taskArn: "fractal-browsers-chrome",
        region: "us-east-1",
        cluster: null,
    },
}

// default export until we have multiple exports
export default DEFAULT
