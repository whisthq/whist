export interface FractalConfig {
  url: {
    TYPEFORM_URL: string
  }
  keys: {
    GOOGLE_ANALYTICS_TRACKING_CODES: string[]
  }
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
