const fs = require("fs")

const packagedEnv = process.argv[2] ?? "dev"

const env = {
    PACKAGED_ENV: packagedEnv
}

const envString = JSON.stringify(env)

fs.writeFileSync("env.json", envString)