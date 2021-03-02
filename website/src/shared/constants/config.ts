/* eslint-disable */
import { FractalEnvironment, FractalConfig } from "shared/types/config"

const environment: FractalEnvironment = {
    local: {
        url: {
            WEBSERVER_URL: "http://127.0.0.1:7730",
            FRONTEND_URL: "http://localhost:3000",
            GRAPHQL_HTTP_URL: "https://dev-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://dev-database.fractal.co/v1/graphql",
            TYPEFORM_URL: "https://tryfractal.typeform.com/to/nRa1zGFa",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "development",
        client_download_urls: {
            macOS:
                "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-dev.s3.amazonaws.com/Fractal.exe",
        },
        payment_enabled: true,
        video_enabled: true,
    },
    development: {
        url: {
            WEBSERVER_URL: "https://dev-server.fractal.co",
            FRONTEND_URL: "https://dev.fractal.co",
            GRAPHQL_HTTP_URL: "https://dev-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://dev-database.fractal.co/v1/graphql",
            TYPEFORM_URL: "https://tryfractal.typeform.com/to/nRa1zGFa",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "development",
        client_download_urls: {
            macOS:
                "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-dev.s3.amazonaws.com/Fractal.exe",
        },
        payment_enabled: true,
        video_enabled: true,
    },
    staging: {
        url: {
            WEBSERVER_URL: "https://staging-server.fractal.co",
            FRONTEND_URL: "https://staging.fractal.co",
            GRAPHQL_HTTP_URL: "https://staging-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://staging-database.fractal.co/v1/graphql",
            TYPEFORM_URL: "https://tryfractal.typeform.com/to/nRa1zGFa",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "staging",
        client_download_urls: {
            macOS:
                "https://fractal-chromium-macos-staging.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-staging.s3.amazonaws.com/Fractal.exe",
        },
        video_enabled: false,
        payment_enabled: false,
    },
    production: {
        url: {
            WEBSERVER_URL: "https://prod-server.fractal.co",
            FRONTEND_URL: "https://fractal.co",
            GRAPHQL_HTTP_URL: "https://prod-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://prod-database.fractal.co/v1/graphql",
            TYPEFORM_URL: "https://tryfractal.typeform.com/to/RsOsBSSu",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_PROD_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "production",
        client_download_urls: {
            macOS:
                "https://fractal-chromium-macos-prod.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-base.s3.amazonaws.com/Fractal.exe",
        },
        payment_enabled: false,
        video_enabled: false,
    },
}

const LIVE_ENV = process.env.REACT_APP_ENVIRONMENT
    ? process.env.REACT_APP_ENVIRONMENT.toString()
    : "development"
// export const config: any = environment.local
export const config: FractalConfig = (() => {
    if (process.env.NODE_ENV !== "development")
        return environment[LIVE_ENV as keyof FractalEnvironment]

    if (process.env.REACT_APP_USE_DOCKER) return environment.local

    return environment.development
})()
