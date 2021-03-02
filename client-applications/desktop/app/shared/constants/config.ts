import {
    FractalEnvironment,
    FractalConfig,
    FractalNodeEnvironment,
    FractalCIEnvironment,
} from "../types/config"

/*
    Webserver URLs
*/
export const webservers: { [key: string]: string } = {
    local: "http://127.0.0.1:7730",
    dev: "http://dev-server.fractal.co/",
    staging: "https://staging-server.fractal.co",
    production: "https://prod-server.fractal.co",
}

/* 
    All environment variables. All secret keys have permission restrictions.
*/
const environment: FractalEnvironment = {
    LOCAL: {
        url: {
            WEBSERVER_URL: webservers.local,
            FRONTEND_URL: "http://localhost:3000",
            GRAPHQL_HTTP_URL: "https://dev-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://dev-database.fractal.co/v1/graphql",
            GOOGLE_REDIRECT_URI: "com.tryfractal.app:/oauth2Callback",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
            AWS_ACCESS_KEY: "AKIA24A776SSHLVMSAVU",
            AWS_SECRET_KEY: "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",
            LOGZ_API_KEY: "IroqVsvNytmNricZSTLUSVtJbxNYBgxp",
        },
        deployEnv: "dev",
        sentryEnv: "development",
        clientDownloadURLs: {
            MacOS:
                "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-dev.s3.amazonaws.com/Fractal.exe",
        },
    },
    DEVELOPMENT: {
        url: {
            WEBSERVER_URL: webservers.dev,
            FRONTEND_URL: "https://dev.fractal.co",
            GRAPHQL_HTTP_URL: "https://dev-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://dev-database.fractal.co/v1/graphql",
            GOOGLE_REDIRECT_URI: "com.tryfractal.app:/oauth2Callback",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
            AWS_ACCESS_KEY: "AKIA24A776SSHLVMSAVU",
            AWS_SECRET_KEY: "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",
            LOGZ_API_KEY: "IroqVsvNytmNricZSTLUSVtJbxNYBgxp",
        },
        deployEnv: "dev",
        sentryEnv: "development",
        clientDownloadURLs: {
            MacOS:
                "https://fractal-chromium-macos-dev.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-dev.s3.amazonaws.com/Fractal.exe   ",
        },
    },
    STAGING: {
        url: {
            WEBSERVER_URL: webservers.staging,
            FRONTEND_URL: "https://staging.fractal.co",
            GRAPHQL_HTTP_URL: "https://staging-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://staging-database.fractal.co/v1/graphql",
            GOOGLE_REDIRECT_URI: "com.tryfractal.app:/oauth2Callback",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
            AWS_ACCESS_KEY: "AKIA24A776SSHLVMSAVU",
            AWS_SECRET_KEY: "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",
            LOGZ_API_KEY: "IroqVsvNytmNricZSTLUSVtJbxNYBgxp",
        },
        deployEnv: "staging",
        sentryEnv: "staging",
        clientDownloadURLs: {
            MacOS:
                "https://fractal-chromium-macos-staging.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-staging.s3.amazonaws.com/Fractal.exe",
        },
    },
    PRODUCTION: {
        url: {
            WEBSERVER_URL: webservers.production,
            FRONTEND_URL: "https://fractal.co",
            GRAPHQL_HTTP_URL: "https://prod-database.fractal.co/v1/graphql",
            GRAPHQL_WS_URL: "wss://prod-database.fractal.co/v1/graphql",
            GOOGLE_REDIRECT_URI: "com.tryfractal.app:/oauth2Callback",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_PROD_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
            AWS_ACCESS_KEY: "AKIA24A776SSHLVMSAVU",
            AWS_SECRET_KEY: "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",
            LOGZ_API_KEY: "IroqVsvNytmNricZSTLUSVtJbxNYBgxp",
        },
        deployEnv: "prod",
        sentryEnv: "production",
        clientDownloadURLs: {
            MacOS:
                "https://fractal-chromium-macos-prod.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-chromium-windows-base.s3.amazonaws.com/Fractal.exe",
        },
    },
}

const getDevelopmentEnv = () => {
    switch (process.env.DEVELOPMENT_ENV) {
        case FractalCIEnvironment.LOCAL:
            return environment.LOCAL
        default:
            return environment.DEVELOPMENT
    }
}

const getProductionEnv = () => {
    switch (process.env.PRODUCTION_ENV) {
        case FractalCIEnvironment.DEVELOPMENT:
            return environment.DEVELOPMENT
        case FractalCIEnvironment.STAGING:
            return environment.STAGING
        case FractalCIEnvironment.PRODUCTION:
            return environment.PRODUCTION
        default:
            return environment.PRODUCTION
    }
}

export const config: FractalConfig =
    process.env.NODE_ENV === FractalNodeEnvironment.DEVELOPMENT
        ? getDevelopmentEnv()
        : getProductionEnv()

// default export until we have multiple exports
export default config
