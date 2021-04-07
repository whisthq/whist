/* eslint-disable */
import React from "react"
import type {
    FractalEnvironment,
    FractalConfig,
} from "@app/shared/types/config"

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
            STRIPE_PUBLIC_KEY: import.meta.env
                .REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: import.meta.env.REACT_APP_GOOGLE_CLIENT_ID,
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
            STRIPE_PUBLIC_KEY: import.meta.env
                .REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: import.meta.env.REACT_APP_GOOGLE_CLIENT_ID,
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
            STRIPE_PUBLIC_KEY: import.meta.env
                .REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: import.meta.env.REACT_APP_GOOGLE_CLIENT_ID,
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
            STRIPE_PUBLIC_KEY: import.meta.env.REACT_APP_STRIPE_PROD_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: import.meta.env.REACT_APP_GOOGLE_CLIENT_ID,
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

const VALID_ENVS = new Set(["local", "development", "staging", "production"])

const LIVE_ENV = (() => {
    if (!import.meta.env.REACT_APP_ENVIRONMENT) return "development"
    if (!VALID_ENVS.has(import.meta.env.REACT_APP_ENVIRONMENT.toString()))
        throw new Error(
            "Environment variable REACT_APP_ENVIRONMENT must be one of: " +
                Array.from(VALID_ENVS).join(", ") +
                ". Received: " +
                import.meta.env.REACT_APP_ENVIRONMENT
        )
    return import.meta.env.REACT_APP_ENVIRONMENT.toString()
})()

export const config: FractalConfig = (() => {
    if (import.meta.env.NODE_ENV !== "development")
        return environment[LIVE_ENV as keyof FractalEnvironment]

    if (import.meta.env.REACT_APP_USE_DOCKER) return environment.local

    return environment.development
})()
