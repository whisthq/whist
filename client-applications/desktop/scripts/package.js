// Package the app using snowpack and electron-builder

const execCommand = require("./execCommand").execCommand
const path = require("path")

const args = process.argv.slice(2)
if (args.filter.length > 1) {
    console.error("Error: Too many flags!")
    process.exit(-1)
}

let publishFlag = ""
if (args[0] === "--never") {
    publishFlag = "--publish never"
}
if (args[0] === "--always") {
    publishFlag = "--publish always"
}

console.log("Getting current client-app version...")
version = execCommand("git describe --abbrev=0", ".", {}, "pipe")

console.log("Running 'snowpack build'...")
execCommand("snowpack build", ".", { VERSION: version })

console.log("Running 'electron-builder build'...")
execCommand(`electron-builder build --config electron-builder.config.js ${publishFlag}`, ".")
