const path = require("path")
const CopyPlugin = require("copy-webpack-plugin")
const { ProvidePlugin } = require("webpack")

module.exports = (env) => {
  const srcDir = path.join(__dirname, "src")
  const config = {
    entry: {
      worker: path.join(srcDir, "worker", "index.ts"),
      content: path.join(srcDir, "content", "index.ts"),
      popup: path.join(srcDir, "popup", "index.tsx"),
      tab: path.join(srcDir, "tab", "index.tsx"),
    },
    output: {
      path: env.targetDir ?? path.resolve("build"),
      filename: "[name].js",
    },
    module: {
      rules: [
        {
          test: /\.tsx?$/,
          use: "ts-loader",
        },
        {
          test: /\.css$/i,
          use: ["style-loader", "css-loader"],
        },
        {
          test: /\.(png|jpe?g|gif)$/i,
          loader: "file-loader",
          options: {
            name: "[name].[ext]",
            outputPath: "assets",
            publicPath: "../assets",
          },
        },
      ],
    },
    resolve: {
      extensions: [".ts", ".tsx", ".js"],
      fallback: {
        path: require.resolve("path-browserify"),
        http: require.resolve("stream-http"),
        https: require.resolve("https-browserify"),
      },
      alias: {
        "@app": srcDir,
      },
    },
    plugins: [
      new CopyPlugin({
        patterns: [{ from: "public" }],
      }),
      new ProvidePlugin({
        process: "process/browser",
      }),
    ],
    externals: {
      modules: [path.join(srcDir, "node_modules")],
    },
    performance: {
      hints: false,
      maxEntrypointSize: 512000,
      maxAssetSize: 512000,
    },
  }

  config.mode = env.webpackMode ?? "production"
  if (env.webpackMode === "development") {
    config.devtool = "inline-source-map"
  }

  return config
}
