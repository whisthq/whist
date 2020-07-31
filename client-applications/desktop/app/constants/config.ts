/* eslint-disable no-unused-vars */
const production = {
    url: {
        PRIMARY_SERVER: "https://main-webserver.fractalcomputers.com"
    },
    stripe: {
        PUBLIC_KEY: "pk_live_XLjiiZB93KN0EjY8hwCxvKmB00whKEIj3U"
    },
    azure: {
        RESOURCE_GROUP: "Fractal"
    }
};

const staging = {
    url: {
        PRIMARY_SERVER: "https://cube-celery-staging.herokuapp.com"
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb"
    },
    azure: {
        RESOURCE_GROUP: "Fractal"
    }
};

const development = {
    url: {
        PRIMARY_SERVER: "http://localhost:7730"
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb"
    },
    azure: {
        RESOURCE_GROUP: "FractalStaging"
    }
};

export const config =
    process.env.NODE_ENV === "development" ? production : production;

export const GOOGLE_CLIENT_ID =
    "581514545734-gml44hupfv9shlj68703s67crnbgjuoe.apps.googleusercontent.com";
