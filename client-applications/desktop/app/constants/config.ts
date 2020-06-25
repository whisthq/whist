/* eslint-disable no-unused-vars */
const production = {
    url: {
        PRIMARY_SERVER: "https://cube-celery-vm.herokuapp.com"
    },
    stripe: {
        PUBLIC_KEY: "pk_live_XLjiiZB93KN0EjY8hwCxvKmB00whKEIj3U"
    }
};

const staging = {
    url: {
        PRIMARY_SERVER: "https://cube-celery-staging.herokuapp.com"
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb"
    }
};

const staging2 = {
    url: {
        PRIMARY_SERVER: "https://cube-celery-staging2.herokuapp.com"
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb"
    }
};

const development = {
    url: {
        PRIMARY_SERVER: "http://127.0.0.1:5000"
    },
    stripe: {
        PUBLIC_KEY: "pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb"
    }
};

export const config =
    process.env.NODE_ENV === "development" ? staging : production;

export const GOOGLE_CLIENT_ID =
    "581514545734-gml44hupfv9shlj68703s67crnbgjuoe.apps.googleusercontent.com";
