/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file helpers.js
 * @brief This file contains helpers for building the Whist Extension.
 */

const fs = require("fs")
const execCommand = require("./execCommand").execCommand

module.exports = {
  // Creates the Tailwind CSS file
  buildTailwind: () => {
    process.stdout.write("\nBuilding CSS with Tailwind...");
    execCommand("tailwindcss build -o public/css/tailwind.css", ".")
    process.stdout.write("\n")
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
    fs.writeFileSync("src/generated_config.json", JSON.stringify(json))
  },
}
