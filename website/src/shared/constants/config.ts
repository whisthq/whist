/* eslint-disable */
import React from "react"
import type {
    FractalEnvironment,
    FractalConfig,
} from "@app/shared/types/config"

const environment: FractalEnvironment = {
    development: {
        keys: {
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "development",
        client_download_urls: {
            macOS: "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-dev.s3.amazonaws.com/Fractal.exe",
        },
    },
    staging: {
        keys: {
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "staging",
        client_download_urls: {
            macOS: "https://fractal-chromium-macos-staging.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-staging.s3.amazonaws.com/Fractal.exe",
        },
    },
    production: {
        keys: {
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "production",
        client_download_urls: {
            macOS: "https://fractal-chromium-macos-prod.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-base.s3.amazonaws.com/Fractal.exe",
        },
    },
}

export const config: FractalConfig = (() => {
    if (import.meta.env.MODE !== "development")
        return environment[
            import.meta.env.FRACTAL_ENVIRONMENT as keyof FractalEnvironment
        ]

    return environment.development
})()
