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

    console.log(
        "Patching `snowpack` to apply https://github.com/snowpackjs/rollup-plugin-polyfill-node/pulls #24 and #26..."
    )
    const snowpackLibIndex = path.join(
        "node_modules",
        "snowpack",
        "lib",
        "index.js"
    )
    const oldContents = fs.readFileSync(snowpackLibIndex, "utf8")
    const newContents = oldContents
        .replaceAll("\\0polyfill-node:", "\\0polyfill-node.")
        .replaceAll(
            "dirs.set(id, path$L.dirname(" /
                " + path$L.relative(basedir, importer)));",
            "dirs.set(id, path$L.posix.dirname(" /
                " + path$L.posix.relative(basedir, importer)));"
        )
        .replaceAll(
            "path$L.join(importer.substr(PREFIX_LENGTH).replace('.js', ''), '..', importee)",
            "path$L.posix.join(importer.substr(PREFIX_LENGTH).replace('.js', ''), '..', importee)"
        )
    fs.writeFileSync(snowpackLibIndex, newContents, "utf8")
    console.log("Success patching `snowpack`!")
}

module.exports = postInstall

if (require.main === module) {
    postInstall()
}
