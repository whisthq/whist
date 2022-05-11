const execCommand = require("./execCommand").execCommand
const helpers = require("./helpers")
const yargs = require("yargs")

const build = (commit, env) => {
  let json = {
    ...helpers.createConfigJSON(env),
    COMMIT_SHA: commit ?? "local_dev",
    ENVIRONMENT: env,
  }

  helpers.createConfigFileFromJSON(json)

  // Build Tailwind CSS file
  helpers.buildTailwind()

  execCommand("webpack --config webpack/webpack.prod.js")
}

if (require.main === module) {
  const argv = yargs(process.argv.slice(2))
    .option("commit", {
      description: "Set the commit hash of the current branch",
      type: "string",
      requiresArg: false,
      demandOption: true,
    })
    .help().argv

  const env = process.env.WHIST_ENV ?? "local"

  console.log(
    `Creating config JSON file for ${env}. For a different environment, set the WHIST_ENV environment variable`
  )

  build(argv.commit, env)
}
