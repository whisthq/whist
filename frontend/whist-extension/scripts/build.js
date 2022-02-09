const execCommand = require("./execCommand").execCommand
const helpers = require("./helpers")

if (require.main === module) {
  const env = process.env.WHIST_ENV ?? "prod"

  console.log(`Creating config JSON file for ${env}`)
  helpers.createConfigFileFromJSON(helpers.createConfigJSON(env))

  execCommand("webpack --config webpack/webpack.prod.js")
}
