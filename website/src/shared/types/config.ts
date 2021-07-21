export interface FractalConfig {
    url: {
        TYPEFORM_FORM_ID: string
    }
    keys: {
        GOOGLE_ANALYTICS_TRACKING_CODES: string[]
    }
    sentry_env: string
    client_download_urls: {
        macOS: string
        Windows: string
    }
}

export interface FractalEnvironment {
    development: FractalConfig
    staging: FractalConfig
    production: FractalConfig
}
