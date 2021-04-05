/** @type {import("snowpack").SnowpackUserConfig } */

const cmdMainCompile = [
    "esbuild",
    "src/main/index.ts",
    "--bundle",
    "--platform=node",
    "--outdir=build/dist/main",
    "--external:electron",
].join(" ")

const cmdElectron = [cmdMainCompile, "&&", "electron build/dist/main"].join(" ")

const cmdMainWatch = [
    "nodemon",
    "--watch ./src/main",
    "--watch ./src/utils",
    "--ext js,jsx,ts,tsx,svg",
    "--exec '" + cmdElectron + "'",
].join(" ")

module.exports = {
    mount: {
        public: { url: "/", static: true },
        src: { url: "/dist" },
    },
    exclude: ["**/node_modules/**/*", "**/src/main/**/*"],
    alias: {
        "@app": "./src",
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
            "aws-sdk",
            "ping",
            "https",
            "crypto",
            "path",
            "child_process",
            "fs",
            "electron",
            "electron-store",
        ],
    },
}
