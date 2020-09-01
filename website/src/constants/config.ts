const production = {
    url: {
        PRIMARY_SERVER: "https://main-webserver.fractalcomputers.com",
    },
    stripe: {
        PUBLIC_KEY: "pk_live_XLjiiZB93KN0EjY8hwCxvKmB00whKEIj3U",
    },
    azure: {
        RESOURCE_GROUP: "Fractal",
    },
    new_server: true,
    sentry_env: "production",
};

const staging = {
    url: {
        PRIMARY_SERVER: "https://cube-celery-staging.herokuapp.com",
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb",
    },
    azure: {
        RESOURCE_GROUP: "Fractal",
    },
    new_server: true,
    sentry_env: "development",
};

const development = {
    url: {
        PRIMARY_SERVER: "http://localhost:7730",
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb",
    },
    azure: {
        RESOURCE_GROUP: "FractalStaging",
    },
    new_server: true,
    sentry_env: "development",
};


// TODO: Change config to development/production after changes in staging are deployed to the other vm webservers
export const config =
    process.env.NODE_ENV === "development" ? development : staging;

export const GOOGLE_CLIENT_ID =
    "581514545734-7k820154jdfp0ov2ifk4ju3vodg0oec2.apps.googleusercontent.com";
