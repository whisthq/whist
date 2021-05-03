/** @type {import("snowpack").SnowpackUserConfig } */

const buildEnv = {
    // These environment variables are defined by Netlify at build time, as per:
    // https://docs.netlify.com/configure-builds/environment-variables. You can
    // also set these on your own.

    // [dev/staging/www].fractal.co
    URL: process.env.URL,

    // "deploy-preview" in the case this is a deploy preview
    CONTEXT: process.env.CONTEXT,

    // the git commit hash of the deployment
    COMMIT_REF: process.env.COMMIT_REF,
}

const getBaseUrl = () => {
    // Return `{ baseUrl: "https://the.base.url" }` if the URL environment variable is
    // defined and this is not a Netlify deploy preview. Otherwise, return `{}`.
    // Intended to be splatted as `...getBaseUrl()` into the `buildOptions`.
    const netlifyUrl = buildEnv.URL
    const isNetlifyPreview = buildEnv.CONTEXT === "deploy-preview"
    
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
    // This controls environment variables which can be used in the app via
    // `import.meta.env`.
    env: {
        FRACTAL_ENVIRONMENT: process.env.FRACTAL_ENVIRONMENT ?? "development",
        FRACTAL_VERSION: buildEnv.COMMIT_REF ?? "local",
    },

    routes: [{ match: "routes", src: ".*", dest: "/index.html" }],
}
