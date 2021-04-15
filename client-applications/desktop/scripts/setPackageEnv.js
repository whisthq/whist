// Generates environment variables to be hardcoded into the
// packaged electron bundle.
//
// To be called via:
//   $ yarn package:set-env -- [dev|staging|prod]
// before calling `yarn package` or `yarn package:ci`.

const fs = require("fs")

const packagedEnv = process.argv[2] ?? "dev"

const env = {
  PACKAGED_ENV: packagedEnv,
}

const envString = JSON.stringify(env)

fs.writeFileSync("env.json", envString)
