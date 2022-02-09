// Build core-ts and electron-notarize after the normal install procedures.
const execCommand = require("./execCommand").execCommand
const path = require("path")
const fs = require("fs")
const helpers = require("./build-package-helpers")

const patch = (pathToPatchedFile, oldContents, newContents) => {
  const oldFile = fs.readFileSync(pathToPatchedFile, "utf8")
  const newFile = oldFile.replaceAll(oldContents, newContents)
  fs.writeFileSync(pathToPatchedFile, newFile, "utf8")
  console.log(`Success patching ${pathToPatchedFile}`)
}

const postInstall = (_env, ..._args) => {
  console.log("Building `@whist/core-ts`...")
  execCommand("yarn install", path.join("node_modules", "@whist/core-ts"))
  execCommand("yarn build", path.join("node_modules", "@whist/core-ts"))
  console.log("Success building `@whist/core-ts`!")

  console.log("Building `electron-notarize`...")
  execCommand("yarn install", path.join("node_modules", "electron-notarize"))
  execCommand("yarn build", path.join("node_modules", "electron-notarize"))
  console.log("Success building `electron-notarize`!")

  // electron-builder's S3 uploader seems to spuriously fail with a TCP
  // error on our tiny macstadium m1 build instance. Our best bet is
  // setting up logic to retry the publish command on failure!
  const appBuilderLibS3Publisher = path.join(
    "node_modules",
    "app-builder-lib",
    "out",
    "publish",
    "s3",
    "BaseS3Publisher.js"
  )
  const maxRetries = 5

  patch(
    appBuilderLibS3Publisher,
    "builder_util_1.executeAppBuilder(",
    `(async (a, p, o={}, m=${maxRetries}) => await builder_util_1.executeAppBuilder(a, p, o, m))(`
  )

  // @m-lab/ndt7 uses WebSocket requests which fail due to the Let's Encrypt
  // certificate error. This fixes it until Electron handles it.
  const ndt7DownloadWorker = path.join(
    "node_modules",
    "@m-lab",
    "ndt7",
    "src",
    "ndt7-download-worker.js"
  )
  const ndt7UploadWorker = path.join(
    "node_modules",
    "@m-lab",
    "ndt7",
    "src",
    "ndt7-upload-worker.js"
  )

  patch(
    ndt7DownloadWorker,
    "new WebSocket(url, 'net.measurementlab.ndt.v7')",
    "new WebSocket(url, 'net.measurementlab.ndt.v7', { rejectUnauthorized: false })"
  )

  patch(
    ndt7UploadWorker,
    "new WebSocket(url, 'net.measurementlab.ndt.v7')",
    "new WebSocket(url, 'net.measurementlab.ndt.v7', { rejectUnauthorized: false })"
  )

  // Recompile node native modules
  helpers.electronBuilderInstallAppDeps()
}

module.exports = postInstall

if (require.main === module) {
  postInstall()
}
