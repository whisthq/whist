/* eslint-disable no-unused-vars */
const production = {
    url: {
        PRIMARY_SERVER: "https://cube-celery-vm.herokuapp.com",
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

const staging2 = {
    url: {
        PRIMARY_SERVER: "https://cube-celery-staging2.herokuapp.com",
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb",
    },
    azure: {
        RESOURCE_GROUP: "FractalStaging",
    },
};

const staging4 = {
    url: {
        PRIMARY_SERVER: "https://cube-celery-staging4.herokuapp.com",
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb",
    },
    azure: {
        RESOURCE_GROUP: "FractalStaging",
    },
};

const development = {
    url: {
        PRIMARY_SERVER: "http://127.0.0.1:5000",
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb",
    },
    azure: {
        RESOURCE_GROUP: "FractalStaging",
    },
};

// TODO: change back when deployed to other vm-webservers
export const config =
    process.env.NODE_ENV === "development" ? staging : production;
