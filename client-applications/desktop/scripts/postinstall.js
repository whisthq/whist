// Build core-ts and electron-notarize after the normal install procedures.
const execCommand = require("./execCommand").execCommand
const fs = require("fs")
const path = require("path")

const postInstall = (_env, ..._args) => {
  console.log("Building `@fractal/core-ts`...")
  execCommand("npm install", path.join("node_modules", "@fractal/core-ts"))
  execCommand("npm run build", path.join("node_modules", "@fractal/core-ts"))
  console.log("Success building `@fractal/core-ts`!")

  console.log("Building `electron-notarize`...")
  execCommand("yarn install", path.join("node_modules", "electron-notarize"))
  execCommand("yarn build", path.join("node_modules", "electron-notarize"))
  console.log("Success building `electron-notarize`!")
}

module.exports = postInstall

if (require.main === module) {
  postInstall()
}
