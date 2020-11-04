const environment: any = {
    local: {
        url: {
            WEBSERVER_URL: "http://127.0.0.1:7730",
            FRONTEND_URL: "http://localhost:3000",
            GRAPHQL_HTTP_URL:
                "https://staging-database.tryfractal.com/v1/graphql",
            GRAPHQL_WS_URL: "wss://staging-database.tryfractal.com/v1/graphql",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            HASURA_ACCESS_KEY: process.env.REACT_APP_HASURA_STAGING_ACCESS_KEY,
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
            WEBSERVER_URL: "https://dev-webserver.herokuapp.com",
            FRONTEND_URL: "http://localhost:3000",
            GRAPHQL_HTTP_URL:
                "https://staging-database.tryfractal.com/v1/graphql",
            GRAPHQL_WS_URL: "wss://staging-database.tryfractal.com/v1/graphql",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            HASURA_ACCESS_KEY: process.env.REACT_APP_HASURA_STAGING_ACCESS_KEY,
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
            WEBSERVER_URL: "https://staging-webserver.tryfractal.com",
            FRONTEND_URL: "https://tryfractal-staging.netlify.app",
            GRAPHQL_HTTP_URL:
                "https://staging-database.tryfractal.com/v1/graphql",
            GRAPHQL_WS_URL: "wss://staging-database.tryfractal.com/v1/graphql",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            HASURA_ACCESS_KEY: process.env.REACT_APP_HASURA_STAGING_ACCESS_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "development",
        client_download_urls: {
            MacOS:
                "https://fractal-mac-application-release.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-windows-application-release.s3.amazonaws.com/Fractal.exe",
        },
    },
    production: {
        url: {
            WEBSERVER_URL: "https://main-webserver.tryfractal.com",
            FRONTEND_URL: "https://tryfractal.com",
            GRAPHQL_HTTP_URL: "https://prod-database.tryfractal.com/v1/graphql",
            GRAPHQL_WS_URL: "wss://prod-database.tryfractal.com/v1/graphql",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_PROD_PUBLIC_KEY,
            HASURA_ACCESS_KEY: process.env.REACT_APP_HASURA_PROD_ACCESS_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
            GOOGLE_ANALYTICS_TRACKING_CODES: ["UA-180615646-1"],
        },
        sentry_env: "production",
        client_download_urls: {
            MacOS:
                "https://fractal-mac-application-release.s3.amazonaws.com/Fractal.dmg",
            Windows:
                "https://fractal-windows-application-release.s3.amazonaws.com/Fractal.exe",
        },
    },
}

export const config: any =
    process.env.NODE_ENV === "development"
        ? environment.staging
        : environment.staging
