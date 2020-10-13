const development = {
    url: {
        PRIMARY_SERVER: "http://localhost:7730",
        FRONTEND_URL: "http://localhost:3000",
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb",
    },
    sentry_env: "development",
}

const production = {
    url: {
        PRIMARY_SERVER: "https://main-webserver.fractalcomputers.com",
        FRONTEND_URL: "https://tryfractal.com",
    },
    stripe: {
        PUBLIC_KEY: "pk_live_XLjiiZB93KN0EjY8hwCxvKmB00whKEIj3U",
    },
    sentry_env: "production",
}

export const config =
    process.env.NODE_ENV === "development" ? development : production

export const GOOGLE_CLIENT_ID =
    "581514545734-7k820154jdfp0ov2ifk4ju3vodg0oec2.apps.googleusercontent.com"
