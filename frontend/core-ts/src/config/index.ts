/*
    All environment variables.
*/
const configs = {
  LOCAL: {
    url: {
      WEBSERVER_URL: "https://dev-server.whist.com",
      CLIENT_CALLBACK_URL: "http://localhost/callback",
    },
    auth0: {
      auth0Domain: "fractal-dev.us.auth0.com",
      clientId: "F4M4J6VjbXlT2UzCfnxFaJK2yKJXbxcF", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
  },
  DEVELOPMENT: {
    url: {
      WEBSERVER_URL: "https://dev-server.whist.com",
      CLIENT_CALLBACK_URL: "http://localhost/callback",
    },
    auth0: {
      auth0Domain: "fractal-dev.us.auth0.com",
      clientId: "F4M4J6VjbXlT2UzCfnxFaJK2yKJXbxcF", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
  },
  STAGING: {
    url: {
      WEBSERVER_URL: "https://staging-server.whist.com",
      CLIENT_CALLBACK_URL: "http://localhost/callback",
    },
    auth0: {
      auth0Domain: "fractal-staging.us.auth0.com",
      clientId: "WXO2cphPECuDc7DkDQeuQzYUtCR3ehjz", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
  },
  PRODUCTION: {
    url: {
      WEBSERVER_URL: "https://prod-server.whist.com",
      CLIENT_CALLBACK_URL: "http://localhost/callback",
    },
    auth0: {
      auth0Domain: "auth.whist.com",
      clientId: "Ulk5B2RfB7mM8BVjA3JtkrZT7HhWIBLD", // public; not a secret
      apiIdentifier: "https://api.fractal.co",
    },
  },
}

export const config = (() => {
  // This should be LOCAL, DEVELOPMENT, STAGING, PRODUCTION
  const environment = JSON.parse(process.env.environmment ?? "{\"env\": \"LOCAL\"}")
  return configs[environment]
})()
