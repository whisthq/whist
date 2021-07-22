/* eslint-disable no-unused-vars */
export type FractalConfig = {
    url: {
        WEBSERVER_URL: string | undefined
        FRONTEND_URL: string | undefined
        GRAPHQL_HTTP_URL: string | undefined
        GRAPHQL_WS_URL: string | undefined
        GOOGLE_REDIRECT_URI: string | undefined
    }
    keys: {
        STRIPE_PUBLIC_KEY: string | undefined
        GOOGLE_CLIENT_ID: string | undefined
        GOOGLE_ANALYTICS_TRACKING_CODES: string[]
    }
    sentryEnv: string | undefined
    clientDownloadURLs: {
        MacOS: string | undefined
        Windows: string | undefined
    }
}

export type FractalEnvironment = {
    LOCAL: FractalConfig
    DEVELOPMENT: FractalConfig
    STAGING: FractalConfig
    PRODUCTION: FractalConfig
}

export enum FractalNodeEnvironment {
    DEVELOPMENT = 'development',
    PRODUCTION = 'production',
}
