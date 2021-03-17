/** @type {import("snowpack").SnowpackUserConfig } */

const cmdMainCompile = [
    "esbuild",
    "src/main/index.ts",
    "--bundle",
    "--platform=node",
    "--outdir=build/dist/main",
    "--external:electron",
].join(" ")

const cmdMainWatch = [
    "nodemon",
    "--watch ./src/main",
    "--watch ./src/utils",
    "--ext js,jsx,ts,tsx",
    "--exec '" + cmdMainCompile + "'",
].join(" ")

module.exports = {
    mount: {
        public: { url: "/", static: true },
        src: { url: "/dist" },
    },
    exclude: ["**/node_modules/**/*", "**/src/main/**/*"],
    alias: {
        "@app/assets": "./public/assets",
        "@app": "./src",
    },
    plugins: [
        "@snowpack/plugin-react-refresh",
        "@snowpack/plugin-dotenv",
        "@snowpack/plugin-typescript",
        [
            "@snowpack/plugin-run-script",
            { cmd: cmdMainCompile, watch: cmdMainWatch },
        ],
    ],
    devOptions: {
        open: "none",
        output: "stream",
    },
    packageOptions: {
        polyfillNode: true,
    },
}
