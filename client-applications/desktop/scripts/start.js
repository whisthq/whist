// Build the protcol and run `snowpack dev`

const { execCommand } = require("./execCommand")
const { sync: rimrafSync } = require("rimraf")

console.log("Building the protocol...")
execCommand("mkdir -p build-clientapp", "../../protocol")
if (process.platform === "win32") {
    execCommand("cmake -S . -B build-clientapp -G Ninja", "../../protocol")
} else {
    execCommand("cmake -S . -B build-clientapp", "../../protocol")
}

execCommand(
  "cmake --build build-clientapp -j --target FractalClient",
  "../../protocol"
)

console.log("Copying over the built protocol...")

rimrafSync("./protocol-build")
execCommand("mkdir -p protocol-build/client", ".")
execCommand(
  "cp -r protocol/build-clientapp/client/build64/* client-applications/desktop/protocol-build/client",
  "../.."
)
rimrafSync("./loading")

if (process.platform === "darwin") {
    execCommand("mv FractalClient _Fractal", "protocol-build/client")
}
execCommand("mv protocol-build/client/loading ./loading", ".")

console.log("Building CSS with tailwind...")
execCommand("tailwindcss build -o public/css/tailwind.css", ".")

console.log("Getting current client-app version...")
version = execCommand("git describe --abbrev=0", ".", {}, "pipe")

console.log("Running 'snowpack dev'...")
execCommand("snowpack dev", ".", { VERSION: version })
