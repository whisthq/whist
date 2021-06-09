// Build core-ts and electron-notarize after the normal install procedures.
const execCommand = require("./execCommand").execCommand
const fs = require("fs")
const path = require("path")

console.log("Building `@fractal/core-ts`...")
execCommand("npm install", path.join("node_modules", "@fractal/core-ts"))
execCommand("npm run build", path.join("node_modules", "@fractal/core-ts"))
console.log("Success building `@fractal/core-ts`!")

console.log("Building `electron-notarize`...")
execCommand("yarn install", path.join("node_modules", "electron-notarize"))
execCommand("yarn build", path.join("node_modules", "electron-notarize"))
console.log("Success building `electron-notarize`!")

console.log(
  "Patching `snowpack` to apply https://github.com/snowpackjs/rollup-plugin-polyfill-node/pull/24..."
)
const snowpackLibIndex = path.join(
  "node_modules",
  "snowpack",
  "lib",
  "index.js"
)
const oldContents = fs.readFileSync(snowpackLibIndex, "utf8")
const newContents = oldContents.replaceAll(
  "\\0polyfill-node:",
  "\\0polyfill-node."
)
fs.writeFileSync(snowpackLibIndex, newContents, "utf8")
console.log("Success patching `snowpack`!")
