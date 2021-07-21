// This is a custom plugin that we've written to deal with a annoying quirk
// of Electron/Snowpack integration. Snowpack, which expects to be deployed
// on a server, exports files with file path imports relative to "public".
// Electron expects file path imports relative to the project root.
// This script keeps them both happy. It should be called from the "plugins"
// section of snowpack.config.js.
//
// As snowpack.config.js needs to control how this file is loaded, it doesn't
// conform to the (env, ...args) => {} signature used by the rest of the scripts.
const path = require("path")
const { promisify } = require("util")
const fs = require("fs")
const readline = require("readline")
const glob = require("glob")

const PROXY_SUFFIX = ".proxy.js"

const readFirstLine = async (filePath) => {
    const readable = fs.createReadStream(filePath)
    const reader = readline.createInterface({ input: readable })
    const line = await new Promise((resolve) => {
        reader.on("line", (content) => {
            reader.close()
            resolve(content)
        })
    })
    readable.close()
    return line
}

const writeFirstLine = async (filePath, content) => {
    const file = await promisify(fs.readFile)(filePath, "utf-8")
    const lines = file.split("\n")
    lines[0] = content
    await promisify(fs.writeFile)(filePath, lines.join("\n"), "utf-8")
}

module.exports = () => {
    return {
        name: "@electron-snowpack/snowpack-plugin-relative-proxy-import",
        async optimize({ buildDirectory }) {
            const proxies = await promisify(glob)(
                path.join(buildDirectory, `**/*${PROXY_SUFFIX}`)
            )
            await Promise.all(
                proxies.map(async (filePath) => {
                    const firstLine = await readFirstLine(filePath)
                    const relativePath = path.relative(buildDirectory, filePath)

                    // The normalization here is important. Even on Windows, node imports
                    // are going to use forward slash / instead of back slash \. Hence,
                    // we are going to have to explicitly force this for proxy imports to
                    // work; else, we try to import from C:\ on Windows.
                    const relativeProxyImport = relativePath
                        .split(path.sep)
                        .join(path.posix.sep)

                    const relativeImport = relativeProxyImport.substring(
                        0,
                        relativeProxyImport.length - PROXY_SUFFIX.length
                    )
                    if (firstLine === `export default "/${relativeImport}";`) {
                        await writeFirstLine(
                            filePath,
                            `export default "${relativeImport}";`
                        )
                    }
                })
            )
        },
    }
}
