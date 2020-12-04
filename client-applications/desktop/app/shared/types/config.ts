export type FractalConfig = {
    url: {
        WEBSERVER_URL: string
        FRONTEND_URL: string
        GRAPHQL_HTTP_URL: string
        GRAPHQL_WS_URL: string
        GOOGLE_REDIRECT_URL: string
    }
    keys: {
        STRIPE_PUBLIC_KEY: string
        GOOGLE_CLIENT_ID: string
        GOOGLE_ANALYTICS_TRACKING_CODES: string
    }
    sentryEnv: string
    clientDownloadURLs: {
        MacOS: string
        Windows: string
    }
}

export type FractalEnvironment = {
    LOCAL: FractalConfig
    DEVELOPMENT: FractalConfig
    STAGING: FractalConfig
    PRODUCTION: FractalConfig
}

export enum FractalNodeEnvironment {
    DEVELOPMENT = "development",
    PRODUCTION = "production",
}
