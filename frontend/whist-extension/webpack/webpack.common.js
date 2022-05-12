const path = require("path")
const CopyPlugin = require("copy-webpack-plugin")
const HtmlWebpackPlugin = require("html-webpack-plugin")
const { ProvidePlugin } = require("webpack")

const srcDir = path.join(__dirname, "..", "src")
const outDir = path.join(__dirname, "..", "build", "src")

module.exports = {
  entry: {
    worker: path.join(srcDir, "worker", "index.ts"),
    tabs: path.join(srcDir, "tabs", "index.tsx"),
    content: path.join(srcDir, "content", "index.ts"),
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
      template: path.join(srcDir, "tabs", "index.html"),
      filename: "tabs.html",
      chunks: ["tabs"],
      cache: false,
    }),
  ],
  externals: {
    modules: [path.join(srcDir, "..", "node_modules")],
  },
}
