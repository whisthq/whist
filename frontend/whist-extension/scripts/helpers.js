const fs = require("fs")
const config = require("./config")

module.exports = {
  createConfigFileFromJSON: (json) => {
    fs.writeFileSync("src/config.json", JSON.stringify(json))
  },
  createConfigJSON: (env) => {
    if (env === "dev") return config.dev
    if (env === "staging") return config.staging
    return config.prod
  },
}
