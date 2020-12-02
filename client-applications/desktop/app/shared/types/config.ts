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
    sentry_env: string
    client_download_urls: {
        MacOS: string
        Windows: string
    }
}

export type FractalEnvironment = {
    local: FractalConfig
    development: FractalConfig
    staging: FractalConfig
    production: FractalConfig
}
