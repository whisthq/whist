const path = require("path")
const execCommand = require("../../../client-applications/desktop/scripts/execCommand").execCommand

console.log("Building `@fractal/core-ts`...")
execCommand("npm install", path.join("node_modules", "@fractal/core-ts"))
execCommand("npm run build", path.join("node_modules", "@fractal/core-ts"))
console.log("Success building `@fractal/core-ts`!")
