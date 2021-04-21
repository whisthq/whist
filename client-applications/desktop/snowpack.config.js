/** @type {import("snowpack").SnowpackUserConfig } */

const esbuildCommand = [
  "esbuild",
  "src/main/index.ts",
  "--bundle",
  "--platform=node",
  "--outdir=build/dist/main",
  "--external:electron",
]
if (process.env.NODE_ENV === "production") {
  esbuildCommand.push("--minify")
}

const cmdMainCompile = esbuildCommand.join(" ")

const cmdElectron = [cmdMainCompile, "&&", "electron build/dist/main"].join(" ")

const cmdMainWatch = [
  "nodemon",
  "--watch ./src/main",
  "--watch ./src/utils",
  "--ext js,jsx,ts,tsx,svg",
  `--exec \"${cmdElectron}\"`,
].join(" ")

console.log(cmdMainWatch)

module.exports = {
  mount: {
    public: { url: "/", static: true },
    src: { url: "/dist" },
  },
  exclude: ["**/node_modules/**/*", "**/src/main/**/*"],
  alias: {
    "@app": "./src",
    "@app/assets": "./public/assets",
  },
  plugins: [
    "@snowpack/plugin-react-refresh",
    "@snowpack/plugin-dotenv",
    "@snowpack/plugin-typescript",
    "./scripts/snowpack-proxy.js",
    [
      "@snowpack/plugin-run-script",
      { name: "electron", cmd: cmdMainCompile, watch: cmdMainWatch },
    ],
  ],
  devOptions: {
    open: "none",
    output: "stream",
  },
  packageOptions: {
    external: [
      "logzio-nodejs",
      "aws-sdk",
      "ping",
      "https",
      "crypto",
      "path",
      "child_process",
      "fs",
      "electron",
      "electron-store",
      "net",
      "os",
      "events",
      "punycode",
      "querystring",
    ],
    polyfillNode: true,
  },
}
