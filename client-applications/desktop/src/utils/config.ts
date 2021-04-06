enum FractalNodeEnvironment {
    DEVELOPMENT = "development",
    PRODUCTION = "production",
}

enum FractalCIEnvironment {
    LOCAL = "local",
    DEVELOPMENT = "dev",
    STAGING = "staging",
    PRODUCTION = "prod",
}

/*
    Webserver URLs
*/
export const webservers: { [key: string]: string } = {
    local: "http://127.0.0.1:7730",
    dev: "http://dev-server.fractal.co",
    staging: "https://staging-server.fractal.co",
    production: "https://prod-server.fractal.co",
}

const url = {
    GOOGLE_REDIRECT_URI: "com.tryfractal.app:/oauth2Callback",
}

const keys = {
    GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
    AWS_ACCESS_KEY: "AKIA24A776SSHLVMSAVU",
    AWS_SECRET_KEY: "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",
    LOGZ_API_KEY: "IroqVsvNytmNricZSTLUSVtJbxNYBgxp",
}

/*
    All environment variables. All secret keys have permission restrictions.
*/
const environment = {
    LOCAL: {
        keys,
        url: {
            ...url,
            WEBSERVER_URL: webservers.local,
            FRONTEND_URL: "http://localhost:3000",
            GRAPHQL_HTTP_URL: "https://dev-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://dev-database.fractal.co/v1/graphql",
        },
        deployEnv: "dev",
        sentryEnv: "development",
        clientDownloadURLs: {
            MacOS:
                "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-dev.s3.amazonaws.com/Fractal.exe",
        },
    },
    DEVELOPMENT: {
        keys,
        url: {
            ...url,
            WEBSERVER_URL: webservers.dev,
            FRONTEND_URL: "https://dev.fractal.co",
            GRAPHQL_HTTP_URL: "https://dev-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://dev-database.fractal.co/v1/graphql",
        },
        deployEnv: "dev",
        sentryEnv: "development",
        clientDownloadURLs: {
            MacOS:
                "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-dev.s3.amazonaws.com/Fractal.exe   ",
        },
    },
    STAGING: {
        keys,
        url: {
            ...url,
            WEBSERVER_URL: webservers.staging,
            FRONTEND_URL: "https://staging.fractal.co",
            GRAPHQL_HTTP_URL: "https://staging-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://staging-database.fractal.co/v1/graphql",
        },
        deployEnv: "staging",
        sentryEnv: "staging",
        clientDownloadURLs: {
            MacOS:
                "https://fractal-chromium-macos-staging.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-staging.s3.amazonaws.com/Fractal.exe",
        },
    },
    PRODUCTION: {
        keys,
        url: {
            ...url,
            WEBSERVER_URL: webservers.production,
            FRONTEND_URL: "https://fractal.co",
            GRAPHQL_HTTP_URL: "https://prod-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://prod-database.fractal.co/v1/graphql",
        },
        deployEnv: "prod",
        sentryEnv: "production",
        clientDownloadURLs: {
            MacOS:
                "https://fractal-chromium-macos-prod.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-base.s3.amazonaws.com/Fractal.exe",
        },
    },
}

const getDevelopmentEnv = () => {
    switch (process.env.DEVELOPMENT_ENV) {
        case FractalCIEnvironment.LOCAL:
            return environment.LOCAL
        default:
            return environment.DEVELOPMENT
    }
}

const getProductionEnv = () => {
    switch (process.env.PRODUCTION_ENV) {
        case FractalCIEnvironment.DEVELOPMENT:
            return environment.DEVELOPMENT
        case FractalCIEnvironment.STAGING:
            return environment.STAGING
        case FractalCIEnvironment.PRODUCTION:
            return environment.PRODUCTION
        default:
            return environment.PRODUCTION
    }
}

export const config =
    process.env.NODE_ENV === FractalNodeEnvironment.DEVELOPMENT
        ? getDevelopmentEnv()
        : getProductionEnv()

// default export until we have multiple exports
export default config
