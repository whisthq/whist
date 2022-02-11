const execCommand = require("./execCommand").execCommand
const helpers = require("./helpers")

if (require.main === module) {
  const env = process.env.WHIST_ENV ?? "prod"

  console.log(
    `Creating config JSON file for ${env}. For a different environment, set the WHIST_ENV environment variable`
  )
  helpers.createConfigFileFromJSON(helpers.createConfigJSON(env))

  // Build Tailwind CSS file
  helpers.buildTailwind()

  execCommand("webpack --config webpack/webpack.prod.js")
}
