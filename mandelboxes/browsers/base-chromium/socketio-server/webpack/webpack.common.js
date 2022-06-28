const path = require("path")

const srcDir = path.join(__dirname, "..", "src")
const outDir = path.join(__dirname, "..", "build")

module.exports = {
  entry: {
    index: path.join(srcDir, "index.ts")
  },
  target: "node",
  output: {
    path: outDir,
    filename: "[name].js",
  },
  resolve: {
    extensions: [".ts", ".js"],
    alias: {
      "@app": srcDir,
    },
  },
  externals: {
    modules: [path.join(srcDir, "..", "node_modules")],
    bufferutil: "commonjs bufferutil",
    "utf-8-validate": "commonjs utf-8-validate"
  },
}
