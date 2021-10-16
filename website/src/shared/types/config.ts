export interface FractalConfig {
  keys: {
    GOOGLE_ANALYTICS_TRACKING_CODES: string[]
  }
  sentry_env: string
  client_download_urls: {
    macOS_x64: string
    macOS_arm64: string
    Windows: string
  }
}

export interface FractalEnvironment {
  development: FractalConfig
  staging: FractalConfig
  production: FractalConfig
}
