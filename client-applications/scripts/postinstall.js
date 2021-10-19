// Build core-ts and electron-notarize after the normal install procedures.
const execCommand = require("./execCommand").execCommand
const path = require("path")
const fs = require("fs")

const postInstall = (_env, ..._args) => {
  console.log("Building `@fractal/core-ts`...")
  execCommand("npm install", path.join("node_modules", "@fractal/core-ts"))
  execCommand("npm run build", path.join("node_modules", "@fractal/core-ts"))
  console.log("Success building `@fractal/core-ts`!")

  console.log("Building `electron-notarize`...")
  execCommand("yarn install", path.join("node_modules", "electron-notarize"))
  execCommand("yarn build", path.join("node_modules", "electron-notarize"))
  console.log("Success building `electron-notarize`!")

  // electron-builder's S3 uploader seems to spuriously fail with a TCP
  // error on our tiny macstadium m1 build instance. Our best bet is
  // setting up logic to retry the publish command on failure!
  console.log("Patching `app-builder-lib` to retry on upload failures...")
  const appBuilderLibS3Publisher = path.join(
    "node_modules",
    "app-builder-lib",
    "out",
    "publish",
    "s3",
    "BaseS3Publisher.js"
  )
  const maxRetries = 5
  const oldContents = fs.readFileSync(appBuilderLibS3Publisher, "utf8")
  const newContents = oldContents.replaceAll(
    "builder_util_1.executeAppBuilder",
    `(async (a, p, o={}, m=${maxRetries}) => await builder_util_1.executeAppBuilder(a, p, o, m))`
  )
  fs.writeFileSync(appBuilderLibS3Publisher, newContents, "utf8")
  console.log("Success patching `app-builder-lib`!")
}

module.exports = postInstall

if (require.main === module) {
  postInstall()
}
