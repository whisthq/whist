/* eslint-disable no-unused-vars */
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
};

const development = {
    url: {
        PRIMARY_SERVER: "http://localhost:7730",
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb",
    },
    azure: {
        RESOURCE_GROUP: "Fractal",
    },
};

// TODO: change back when deployed to other vm-webservers
export const config =
    process.env.NODE_ENV === "development" ? staging : production;
