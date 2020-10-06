/* eslint-disable no-unused-vars */
const production = {
    url: {
        PRIMARY_SERVER: 'https://main-webserver.fractalcomputers.com',
    },
    stripe: {
        PUBLIC_KEY: 'pk_live_XLjiiZB93KN0EjY8hwCxvKmB00whKEIj3U',
    },
    azure: {
        RESOURCE_GROUP: 'Fractal',
    },
}

const staging = {
    url: {
        PRIMARY_SERVER: 'https://staging-webserver.fractalcomputers.com',
    },
    stripe: {
        PUBLIC_KEY: 'pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb',
    },
    azure: {
        RESOURCE_GROUP: 'Fractal',
    },
}

const development = {
    url: {
        PRIMARY_SERVER: 'http://127.0.0.1:7730',
    },
    stripe: {
        PUBLIC_KEY: 'pk_test_7y07LrJWC5LzNu17sybyn9ce004CLPaOXb',
    },
    azure: {
        RESOURCE_GROUP: 'FractalStaging',
    },
}

export const config =
    process.env.NODE_ENV === 'development' ? development : development

export const GOOGLE_CLIENT_ID =
    '581514545734-4t3fd64bus687mcd3s7te11bv8hs532h.apps.googleusercontent.com'

export const GOOGLE_REDIRECT_URI = `com.tryfractal.app:/oauth2Callback`
