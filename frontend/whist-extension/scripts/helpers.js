const fs = require("fs")
const config = require("./config")

module.exports = {
  createConfigFileFromJSON: (json) => {
    fs.writeFileSync("src/config.json", JSON.stringify(json))
  },
  createConfigJSON: (env) => {
    if (env === "dev") {
      return {
        AUTH_DOMAIN_URL: config.AUTH_DOMAIN_URL_DEV,
        AUTH_CLIENT_ID: config.AUTH_CLIENT_ID_DEV,
        AUTH_API_IDENTIFIER: config.AUTH_API_IDENTIFIER,
        CLIENT_CALLBACK_URL: config.CLIENT_CALLBACK_URL,
      }
    } else if (env === "staging") {
      return {
        AUTH_DOMAIN_URL: config.AUTH_DOMAIN_URL_STAGING,
        AUTH_CLIENT_ID: config.AUTH_CLIENT_ID_STAGING,
        AUTH_API_IDENTIFIER: config.AUTH_API_IDENTIFIER,
        CLIENT_CALLBACK_URL: config.CLIENT_CALLBACK_URL,
      }
    } else {
      return {
        AUTH_DOMAIN_URL: config.AUTH_DOMAIN_URL_PROD,
        AUTH_CLIENT_ID: config.AUTH_CLIENT_ID_PROD,
        AUTH_API_IDENTIFIER: config.AUTH_API_IDENTIFIER,
        CLIENT_CALLBACK_URL: config.CLIENT_CALLBACK_URL,
      }
    }
  },
}
