const helpers = require("./helpers")

const postInstall = () => {
  // Create config file
  helpers.createConfigFileFromJSON(helpers.createConfigJSON("dev"))
}

module.exports = postInstall

if (require.main === module) {
  postInstall()
}
