const path = require("path")
const CopyPlugin = require("copy-webpack-plugin")
const HtmlWebpackPlugin = require("html-webpack-plugin")
const { ProvidePlugin } = require("webpack")

const srcDir = path.join(__dirname, "..", "src")
const outDir = path.join(__dirname, "..", "build", "src")

module.exports = {
  entry: {
    worker: path.join(srcDir, "worker", "index.ts"),
    newtab: path.join(srcDir, "newtab", "index.tsx"),
  },
  output: {
    path: outDir,
    filename: "[name].js",
  },
  module: {
    rules: [
      {
        test: /\.tsx?$/,
        use: "ts-loader",
        exclude: /node_modules/,
      },
    ],
  },
  resolve: {
    extensions: [".ts", ".tsx", ".js"],
    fallback: {
      path: require.resolve("path-browserify"),
    },
    alias: {
      "@app": srcDir,
    },
  },
  plugins: [
    new CopyPlugin({
      patterns: [{ from: ".", to: "../", context: "public" }],
    }),
    new ProvidePlugin({
      process: "process/browser",
    }),
    new HtmlWebpackPlugin({
      template: path.join(srcDir, "newtab", "index.html"),
      filename: "newtab.html",
      chunks: ["newtab"],
      cache: false,
    }),
  ],
}
