// Build the protcol and run `snowpack dev`

const { execCommand } = require("./execCommand")
const { sync: rimrafSync } = require("rimraf")
const fs = require("fs")
const fse = require("fs-extra")
const path = require("path")

console.log("Building the protocol...")
const cmakeBuildDir = "build-clientapp"
fs.mkdirSync(path.join("../../protocol", cmakeBuildDir), { recursive: true })

if (process.platform === "win32") {
  // On Windows, cmake wants rc.exe but is provided node_modules/.bin/rc.js
  // Hence, we must modify the path to pop out the node_modules entries.
  const pathArray = process.env.Path.split(";")
  while (["node_modules", "yarn", "Yarn"].some((v) => pathArray[0].includes(v)))
    pathArray.shift()
  const path = pathArray.join(";")
  execCommand(`cmake -S . -B ${cmakeBuildDir} -G Ninja`, "../../protocol", {
    Path: path,
  })
} else {
  execCommand(`cmake -S . -B ${cmakeBuildDir}`, "../../protocol")
}

execCommand(
  `cmake --build ${cmakeBuildDir} -j --target FractalClient`,
  "../../protocol"
)

console.log("Copying over the built protocol...")

const protocolBuildDir = path.join("protocol-build", "client")
rimrafSync("protocol-build")
fse.copySync(
  path.join("../../protocol", cmakeBuildDir, "client/build64"),
  protocolBuildDir
)

const ext = process.platform === "win32" ? ".exe" : ""
const oldExecutable = "FractalClient"
const newExecutable = process.platform === "darwin" ? "_Fractal" : "Fractal"

fse.moveSync(
  path.join(protocolBuildDir, `${oldExecutable}${ext}`),
  path.join(protocolBuildDir, `${newExecutable}${ext}`)
)

rimrafSync("./loading")
fse.moveSync(
  path.join("protocol-build/client", "loading"),
  path.join("loading")
)

console.log("Building CSS with tailwind...")
execCommand("tailwindcss build -o public/css/tailwind.css", ".")

console.log("Getting current client-app version...")
version = execCommand("git describe --abbrev=0", ".", {}, "pipe")

console.log("Running 'snowpack dev'...")
execCommand("snowpack dev", ".", { VERSION: version })
