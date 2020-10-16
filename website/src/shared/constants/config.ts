const environment: any = {
    development: {
        url: {
            WEBSERVER_URL: "http://localhost:7300",
            FRONTEND_URL: "http://localhost:3000",
            GRAPHQL_HTTP_URL:
                "https://staging-database.tryfractal.com/v1/graphql",
            GRAPHQL_WS_URL: "wss://staging-database.tryfractal.com/v1/graphql",
        },
        keys: {
            STRIPE_PUBLIC_KEY: process.env.REACT_APP_STRIPE_STAGING_PUBLIC_KEY,
            HASURA_ACCESS_KEY: process.env.REACT_APP_HASURA_STAGING_ACCESS_KEY,
            GOOGLE_CLIENT_ID: process.env.REACT_APP_GOOGLE_CLIENT_ID,
        },
        sentry_env: "development",
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
        },
        sentry_env: "development",
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
        },
        sentry_env: "production",
    },
}

export const config: any =
    process.env.NODE_ENV === "development"
        ? environment.staging
        : environment.staging
