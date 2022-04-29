/* eslint-disable no-unused-vars */
export type WhistConfig = {
  url: {
    SCALING_SERVICE_URL: string | undefined
    FRONTEND_URL: string | undefined
    GOOGLE_REDIRECT_URI: string | undefined
  }
  keys: {
    STRIPE_PUBLIC_KEY: string | undefined
    GOOGLE_CLIENT_ID: string | undefined
    GOOGLE_ANALYTICS_TRACKING_CODES: string[]
  }
}

export type WhistEnvironment = {
  LOCAL: WhistConfig
  DEVELOPMENT: WhistConfig
  STAGING: WhistConfig
  PRODUCTION: WhistConfig
}

export enum WhistNodeEnvironment {
  DEVELOPMENT = "development",
  PRODUCTION = "production",
}
