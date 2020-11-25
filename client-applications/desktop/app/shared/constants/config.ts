// these are basically used to keep these links in one place
// since we are going to be using them in logic for admin integration testing
// app where you can choose where to go to
export const webservers: { [key: string]: string } = {
    local: "http://127.0.0.1:7730",
    dev: "https://dev-webserver.herokuapp.com",
    staging: "https://staging-webserver.tryfractal.com",
    prod: "https://main-webserver.herokuapp.com",
}

const environment: any = {
    local: {
        url: {
            WEBSERVER_URL: webservers.local,
            FRONTEND_URL: "http://localhost:3000",
            GRAPHQL_HTTP_URL: "https://dev-database.tryfractal.com/v1/graphql",
            GRAPHQL_WS_URL: "wss://dev-database.tryfractal.com/v1/graphql",
            GOOGLE_REDIRECT_URI: "com.tryfractal.app:/oauth2Callback",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "development",
        client_download_urls: {
            MacOS:
                "https://fractal-mac-application-testing.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-windows-application-testing.s3.amazonaws.com/Fractal.exe",
        },
    },
    development: {
        url: {
            WEBSERVER_URL: webservers.dev,
            // these below we might also want to change...
            // mainly graphql
            FRONTEND_URL: "https://dev.tryfractal.com",
            GRAPHQL_HTTP_URL: "https://dev-database.tryfractal.com/v1/graphql",
            GRAPHQL_WS_URL: "wss://dev-database.tryfractal.com/v1/graphql",
            GOOGLE_REDIRECT_URI: "com.tryfractal.app:/oauth2Callback",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "development",
        client_download_urls: {
            MacOS:
                "https://fractal-mac-application-testing.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-windows-application-testing.s3.amazonaws.com/Fractal.exe   ",
        },
    },
    staging: {
        url: {
            WEBSERVER_URL: webservers.staging,
            FRONTEND_URL: "https://staging.tryfractal.com",
            GRAPHQL_HTTP_URL:
                "https://staging-database.tryfractal.com/v1/graphql",
            GRAPHQL_WS_URL: "wss://staging-database.tryfractal.com/v1/graphql",
            GOOGLE_REDIRECT_URI: "com.tryfractal.app:/oauth2Callback",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "development",
        client_download_urls: {
            MacOS:
                "https://fractal-mac-application-release.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-windows-application-base.s3.amazonaws.com/Fractal.exe",
        },
    },
    production: {
        url: {
            WEBSERVER_URL: webservers.prod,
            FRONTEND_URL: "https://tryfractal.com",
            GRAPHQL_HTTP_URL: "https://prod-database.tryfractal.com/v1/graphql",
            GRAPHQL_WS_URL: "wss://prod-database.tryfractal.com/v1/graphql",
            GOOGLE_REDIRECT_URI: "com.tryfractal.app:/oauth2Callback",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_PROD_PUBLIC_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "production",
        client_download_urls: {
            MacOS:
                "https://fractal-mac-application-release.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-windows-application-base.s3.amazonaws.com/Fractal.exe",
        },
    },
}

export const config: any =
    process.env.NODE_ENV === "development"
        ? environment.development
        : environment.production
