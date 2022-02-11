const fs = require("fs")
const config = require("./config")
const execCommand = require("./execCommand").execCommand

module.exports = {
  createConfigFileFromJSON: (json) => {
    fs.writeFileSync("src/config.json", JSON.stringify(json))
  },
  createConfigJSON: (env) => {
    if (env === "dev") return config.dev
    if (env === "staging") return config.staging
    return config.prod
  },
  buildTailwind: () => {
    console.log("Building CSS with tailwind...")
    execCommand("tailwindcss build -o public/css/tailwind.css", ".")
  },
}
