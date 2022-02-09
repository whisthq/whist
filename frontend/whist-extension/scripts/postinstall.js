const helpers = require("./helpers")

const postInstall = () => {
  console.log(`Creating config JSON file for dev`)
  helpers.createConfigFileFromJSON(helpers.createConfigJSON("dev"))
}

module.exports = postInstall

if (require.main === module) {
  postInstall()
}
