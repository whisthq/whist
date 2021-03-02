export type FractalConfig = {
    url: {
        WEBSERVER_URL: string
        FRONTEND_URL: string
        GRAPHQL_HTTP_URL: string
        GRAPHQL_WS_URL: string
        TYPEFORM_URL: string
    }
    keys: {
        STRIPE_PUBLIC_KEY: string | undefined
        GOOGLE_CLIENT_ID: string | undefined
        GOOGLE_ANALYTICS_TRACKING_CODES: string[]
    }
    sentry_env: string
    client_download_urls: {
        macOS: string
        Windows: string
    }
    payment_enabled: boolean
    video_enabled: boolean
}

export type FractalEnvironment = {
    local: FractalConfig
    development: FractalConfig
    staging: FractalConfig
    production: FractalConfig
}
