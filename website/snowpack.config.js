/** @type {import("snowpack").SnowpackUserConfig } */

const getBaseUrl = () => {
    // Return `{ baseUrl: "https://the.base.url" }` if the URL environment variable is
    // defined and this is not a Netlify deploy preview. Otherwise, return `{}`.
    // Intended to be splatted as `...getBaseUrl()` into the `buildOptions`.

    // These environment variables are defined by Netlify at build time, as per:
    // https://docs.netlify.com/configure-builds/environment-variables/#deploy-urls-and-metadata
    const netlifyUrl = process.env.URL
    const isNetlifyPreview = process.env.CONTEXT === "deploy-preview"
    
    if (!isNetlifyPreview && netlifyUrl) {
        return { baseUrl: netlifyUrl }
    } else {
        return {}
    }
}

module.exports = {
    mount: {
        public: { url: "/", static: true },
        src: { url: "/dist" },
    },
    alias: {
        "@app/assets": "./public/assets",
        "@app": "./src",
    },
    plugins: [
        "@snowpack/plugin-react-refresh",
        "@snowpack/plugin-dotenv",
        "@snowpack/plugin-typescript",
    ],
    buildOptions: {
        // This controls what replaces `%PUBLIC_URL%` in index.html; the default is `/`.
        // Having a full path is important in order for OpenGraph previews to display the
        // thumbnail, as OpenGraph requires a full path to assets.
        ...getBaseUrl()
    },
    routes: [{ match: "routes", src: ".*", dest: "/index.html" }],
}
