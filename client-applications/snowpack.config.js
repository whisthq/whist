/** @type {import("snowpack").SnowpackUserConfig } */
// This comment above enables TypeScript information in IDEs.

// This file configures Javascript compilation + bundling for both the Electron
// main process and the renderer process.
//
// Both main/renderer first have their `.ts/.tsx` files compiled by TypeScript,
// with settings from src/tsconfig.json. Outside of TypeScript, they have very
// little in common in their "bundling" procedures. This is because the main
// process will run in a NodeJS environment, and the renderer process will run
// in a browser environment.
//
// Snowpack deals almost entirely with the renderer process, we use a separate
// set of commands to bundle the main process files. However, we use Snowpack
// to provide our "hot-reload" engine for the renderer process, and Snowpack
// provides some useful "hooks" to call other commands at different stages of
// its bundling. We leverage these hooks to call some external commands that
// do the main process bundling, even though those external commands have nothing
// to do with Snowpack.
//
// This allows us to have a (mostly) seamless bundling experience. Everything
// will run on yarn start, and reload on changes.

// The main process still requires some "bundling", as its npm dependencies need
// to be packaged up with the app. We use esbuild to do this bundling for us.
// This esbuild command below is on of the "external commands" that will be run
// by a Snowpack hook.
//
// We don't want to bundle Electron. It's included automatically by the
// Electron build tooling.
//
// We follow a consistent style with our commands below, which is to write them
// out as a list, and then concatenate the list with a " " to build the final
// command.
const electronMainBuildCommand = [
  "esbuild",
  "src/main/index.ts",
  "--bundle",
  "--platform=node",
  "--outdir=build/dist/main",
  "--external:electron",
  ...((process.env.NODE_ENV === "production") ? ["--minify"] : []),
].join(" ")

const nodeEntrypointBuildCommand = [
  "esbuild",
  "src/entrypoint/index.ts",
  "--bundle",
  "--platform=node",
  "--outdir=build/dist/entrypoint",
  "--external:electron",
  ...((process.env.NODE_ENV === "production") ? ["--minify"] : []),
].join(" ")

// Every time we run Electron, we want to first compile the main process files.
// This command will be called by a Snowpack hook, and Snowpack will take care
// of compiling the renderer process files.
const mainBuildCommand = [electronMainBuildCommand, "&&", nodeEntrypointBuildCommand].join(" ")
const mainRunCommand = [mainBuildCommand, "&&", "node build/dist/entrypoint"].join(" ")

// Snowpack only supplies "hot-reload" for the renderer process, so we use
// nodemon as a "hot-reload" for the main process. We give it some folders to
// watch, as well as a command to re-run on every file change. We simply re-run
// our esbuild && electron command on every main process change, which restarts
// the entire Electron application.
const mainWatchCommand = [
  "nodemon",
  "--watch ./src/main",
  "--watch ./src/testing",
  "--watch ./src/utils",
  "--ext js,jsx,ts,tsx,svg",
  `--exec "${mainRunCommand}"`,
].join(" ")

// This is the actual Snowpack configuration. Most snowpack.config.js files
// will just have the data below, be we've defined our external commands above
// so they can be easily referenced.
module.exports = {
  // Map of folder: url. Our public folder is the "root" url, which means
  // that "public/index.html" will be loaded at the url "/". We compile our
  // src files into "/dist", so that "dist/renderer/index.js" can be loaded in
  // a <script/> tag from index.html.
  mount: {
    public: { url: "/", static: true },
    src: { url: "/dist" },
  },
  // We don't want to copy over the node_modules folder because Snowpack will
  // handle that. This is Snowpack's default behavior, but we're being explicit
  // in this exclude list. We also exclude "**/src/main/**", because we want to
  // separately compile our main process files using the commands above.
  exclude: [
    "**/node_modules/**/*",
    "**/src/main/**/*",
    "**/src/utils/mandelbox.ts",
    "**/src/utils/protocol.ts",
    "**/src/utils/host.ts",
  ],
  // This needs to match the "paths" mapping in ts.config.json. We choose to
  // prefix all our source code imports with "@app".
  alias: {
    "@app": "./src",
  },
  // Snowpack has a useful ecosystem of plugins, and we use a few here. These
  // need to be installed separately through npm.
  plugins: [
    // Adds state-preserving refresh behavior to react, which is common for
    // react bundling tools.
    "@snowpack/plugin-react-refresh",
    // Allows loading environment variables into Snowpack files. Note that
    // a "SNOWPACK_PUBLIC_" prefix is required for security reasons.
    "@snowpack/plugin-dotenv",
    // This plugin calls the typescript compiler on every build/reload. It uses
    // the tsconfig.json in the root of the project.
    "@snowpack/plugin-typescript",
    // This is a custom plugin that we've written to deal with a annoying quirk
    // of Electron/Snowpack integration. Snowpack, which expects to be deployed
    // on a server, exports files with file path imports relative to "public".
    // Electron expects file path imports relative to the project root.
    // This script keeps them both happy.
    "./scripts/snowpack-proxy.js",
    // This plugin makes use of Snowpack's "hooks" to allow you to run external
    // commands in different build stages. This is where we can make use of the
    // external commands we defined above for compiling the main process.
    // Note the separate "cmd" and "watch" keys. These correspond to commands
    // that will be called in "snowpack build" and "snowpack dev".
    [
      "@snowpack/plugin-run-script",
      { name: "entrypoint|electron", cmd: mainBuildCommand, watch: mainWatchCommand },
    ],
  ],
  // We do not want to open the devTools right away. We choose instead to
  // open them manually from the Electron application, because we want Electron
  // to be in a "ready" state before the devTools window opens.
  //
  // We use "output:stream" to ask Snowpack not to try and manage the console
  // output. This is important, because the console output is shared with the
  // external commands defined above.
  devOptions: {
    open: "none",
    output: "stream",
  },
  // We need to explicily exclude NodeJS packages/modules that cannot be loaded
  // in the browser. This includes modules with file system or subprocess access,
  // because the browser does not have those features. If you don't exclude them
  // here, you'll get a runtime error in the renderer process.
  //
  // Someday, we may separate the main process and renderer process into
  // completely seperate "subprojects" to mitigate issues like this.
  packageOptions: {
    external: [
      "logzio-nodejs",
      "@amplitude/node",
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
    // We ask Snowpack to polyfill and NodeJS APIs that it can, so that we can
    // still make some use of the NodeJS standard library. It cannot polyfill
    // things the browser cannot do, like access files.
    polyfillNode: true,
  },
}
