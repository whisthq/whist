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

const getBuildOptions = () => {
  // Return `{ baseUrl: "https://the.base.url" }` if the URL environment variable is
  // defined and this is not a Netlify deploy preview. Otherwise, return `{}`.
  // Intended to used to generate `buildOptions` below.
  const netlifyUrl = buildEnv.URL
  const isNetlifyPreview = buildEnv.CONTEXT === "deploy-preview"

  if (!isNetlifyPreview && netlifyUrl) {
    // `baseUrl` controls what replaces `%PUBLIC_URL%` in index.html; the default is `/`.
    // Having a full path is important in order for OpenGraph previews to display the
    // thumbnail, as OpenGraph requires a full path to assets.
    return { baseUrl: netlifyUrl }
  } else {
    return {}
  }
}

const getFractalEnv = () => {
  // Return environment variables to bake into our application as a dictionary;
  // these can be imported via `import.meta.env`.
  return {
    FRACTAL_ENVIRONMENT: process.env.FRACTAL_ENVIRONMENT
      ? process.env.FRACTAL_ENVIRONMENT
      : "development",
    FRACTAL_VERSION: buildEnv.COMMIT_REF ? buildEnv.COMMIT_REF : "local",
  }
}

module.exports = {
  mount: {
    public: { url: "/", static: true },
    src: { url: "/dist" },
  },

  // This simplifies import syntax in our React app
  alias: {
    "@app/assets": "./public/assets",
    "@app": "./src",
  },

  plugins: [
    "@snowpack/plugin-react-refresh",
    "@snowpack/plugin-dotenv",
    "@snowpack/plugin-typescript",
  ],

  buildOptions: getBuildOptions(),

  // This controls environment variables which can be used in the app via `import.meta.env`
  env: getFractalEnv(),

  // This points all routes to our single-page app rooted at index.html
  routes: [{ match: "routes", src: ".*", dest: "/index.html" }],
}
