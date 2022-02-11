const helpers = require("./helpers")

const postInstall = () => {
  helpers.createConfigFileFromJSON(helpers.createConfigJSON("dev"))
  helpers.buildTailwind()
}

module.exports = postInstall

if (require.main === module) {
  postInstall()
}
