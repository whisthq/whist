/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file build.js
 * @brief This file contains utility functions for building the Whist Extension.
 */

const execCommand = require("./execCommand").execCommand
const helpers = require("./helpers")
const yargs = require("yargs")

const build = (argv) => {
  let json = {
    COMMIT_SHA: argv.commit ?? "local_dev",
    AUTH0_CLIENT_ID: argv.auth0_client_id,
    AUTH0_DOMAIN_URL: argv.auth0_domain_url,
    AUTH0_REDIRECT_URL: argv.auth0_redirect_url,
    SCALING_SERVICE_URL: argv.scaling_service_url,
    AMPLITUDE_KEY: argv.amplitude_key,
    HOST_IP: argv.host_ip,
  }

  console.log("Building Whist extension with config", json)
  helpers.createConfigFileFromJSON(json)

  // Build Tailwind CSS file
  helpers.buildTailwind()

  execCommand(
    `webpack --config webpack.config.js --env webpackMode=development ${
      argv.output_dir ? `--env targetDir=${argv.output_dir}` : ""
    }`
  )
}

if (require.main === module) {
  const argv = yargs(process.argv.slice(2))
    .version(false)
    .option("commit", {
      description: "Set the commit hash of the current branch",
      type: "string",
      requiresArg: true,
      demandOption: false,
    })
    .option("whistVersion", {
      description: "Set the commit hash of the current branch",
      type: "string",
      requiresArg: true,
      demandOption: false,
    })
    .option("auth0_client_id", {
      description: "Auth0 client ID",
      type: "string",
      requiresArg: true,
      demandOption: false,
    })
    .option("auth0_domain_url", {
      description: "Auth0 domain URL",
      type: "string",
      requiresArg: true,
      demandOption: false,
    })
    .option("auth0_redirect_url", {
      description: "Auth0 redirect URL",
      type: "string",
      requiresArg: true,
      demandOption: false,
    })
    .option("scaling_service_url", {
      description: "Scaling service URL",
      type: "string",
      requiresArg: true,
      demandOption: false,
    })
    .option("host_ip", {
      description: "Set a testing host IP",
      type: "string",
      requiresArg: true,
      demandOption: false,
    })
    .option("amplitude_key", {
      description: "Amplitude public key",
      type: "string",
      requiresArg: true,
      demandOption: false,
    })
    .option("output_dir", {
      description: "Build output directory (defaults to `build`)",
      type: "string",
      requiresArg: true,
      demandOption: false,
    })
    .help().argv

  build(argv)
}
