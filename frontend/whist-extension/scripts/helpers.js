const fs = require("fs")
const config = require("./config")
const execCommand = require("./execCommand").execCommand

module.exports = {
  // Creates the Tailwind CSS file
  buildTailwind: () => {
    console.log("Building CSS with tailwind...")
    execCommand("tailwindcss build -o public/css/tailwind.css", ".")
  },
  // Function to set the commit sha for the packaged app, which gets
  // hardcoded into the electron bundle via `env_overrides.json`.
  // Acceptable arguments: a git commit sha
  checkCommitSha: (e) => {
    if (!(e.length === 40 && /^[a-f0-9]+$/.test(e))) {
      console.error(`setPackagedCommitSha passed a bad commit sha: ${e}`)
      process.exit(-1)
    }
  },
  createConfigFileFromJSON: (json) => {
    fs.writeFileSync("src/config.json", JSON.stringify(json))
  },
  createConfigJSON: (env) => {
    if (env === "dev") return config.dev
    if (env === "staging") return config.staging
    if (env === "prod") return config.prod
    return config.local
  },
}
