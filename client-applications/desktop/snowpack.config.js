/** @type {import("snowpack").SnowpackUserConfig } */

module.exports = {
    mount: {
        public: { url: "/", static: true },
        src: { url: "/dist" },
    },
    exclude: ["**/node_modules/**/*"],
    alias: {
        "@app/assets": "./public/assets",
        "@app": "./src"
    },
    plugins: [
        "@snowpack/plugin-react-refresh",
        "@snowpack/plugin-dotenv",
        "@snowpack/plugin-typescript",
    ],
    devOptions: {
        open: "none"
    },
    packageOptions: {
        polyfillNode: true
    }
}
