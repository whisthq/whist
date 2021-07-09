// This file contains helper functions for building, packaging, and publishing the client application.

const { execCommand } = require("./execCommand")
const { sync: rimrafSync } = require("rimraf")
const fs = require("fs")
const fse = require("fs-extra")
const path = require("path")

const envOverrideFile = "env_overrides.json"

const readExistingEnvOverrides = () => {
  let envFileContents = ""

  try {
    envFileContents = fs.readFileSync(envOverrideFile, "utf-8")
  } catch (err) {
    if (err.code !== "ENOENT") {
      console.error(`Error reading contents of ${envOverrideFile}: ${err}`)
      process.exit(-1)
    }
  }

  // Treat an empty/non-existent file as valid
  envFileContents = envFileContents.trim() === "" ? "{}" : envFileContents
  const existingEnv = JSON.parse(envFileContents)

  return existingEnv
}

const addEnvOverride = (envs) => {
  const existingEnv = readExistingEnvOverrides()

  const newEnv = {
    ...existingEnv,
    ...envs,
  }

  const envString = JSON.stringify(newEnv)
  fs.writeFileSync(envOverrideFile, envString)
}

module.exports = {
  // Build the protocol and copy it into the expected location
  buildAndCopyProtocol: () => {
    console.log("Building the protocol...")
    const cmakeBuildDir = "build-clientapp"
    fs.mkdirSync(path.join("../../protocol", cmakeBuildDir), {
      recursive: true,
    })

    if (process.platform === "win32") {
      // On Windows, cmake wants rc.exe but is provided node_modules/.bin/rc.js
      // Hence, we must modify the path to pop out the node_modules entries.
      const pathArray = process.env.Path.split(";")
      while (
        ["node_modules", "yarn", "Yarn"].some((v) => pathArray[0].includes(v))
      )
        pathArray.shift()
      const path = pathArray.join(";")
      execCommand(`cmake -S . -B ${cmakeBuildDir} -G Ninja`, "../../protocol", {
        Path: path,
      })
    } else {
      execCommand(`cmake -S . -B ${cmakeBuildDir}`, "../../protocol")
    }

    execCommand(
      `cmake --build ${cmakeBuildDir} -j --target FractalClient`,
      "../../protocol"
    )

    console.log("Copying over the built protocol...")

    const protocolBuildDir = path.join("protocol-build", "client")
    rimrafSync("protocol-build")
    fse.copySync(
      path.join("../../protocol", cmakeBuildDir, "client/build64"),
      protocolBuildDir
    )

    const ext = process.platform === "win32" ? ".exe" : ""
    const oldExecutable = "FractalClient"
    const newExecutable = process.platform === "darwin" ? "_Fractal" : "Fractal"

    fse.moveSync(
      path.join(protocolBuildDir, `${oldExecutable}${ext}`),
      path.join(protocolBuildDir, `${newExecutable}${ext}`)
    )

    rimrafSync("./loading")
    fse.moveSync(
      path.join("protocol-build/client", "loading"),
      path.join("loading")
    )
  },

  // Build TailwindCSS
  buildTailwind: () => {
    console.log("Building CSS with tailwind...")
    execCommand("tailwindcss build -o public/css/tailwind.css", ".")
  },

  // Get the current client-app version
  getCurrentClientAppVersion: () => {
    const version = execCommand(
      "git describe --tags --abbrev=0",
      ".",
      {},
      "pipe"
    )
    console.log(`Getting current client-app version: ${version}`)
    return version
  },

  // Run snowpack in development mode
  snowpackDev: (env) => {
    console.log(`Running 'snowpack dev' with version ${env.VERSION} ...`)
    execCommand("snowpack dev", ".", env)
  },

  // Run snowpack in build mode
  snowpackBuild: (env) => {
    console.log(`Running 'snowpack build' with version ${env.VERSION} ...`)
    execCommand("snowpack build", ".", env)
  },

  // Build via electron-build
  electronBuild: () => {
    console.log("Running 'electron-builder build'...")
    execCommand(
      `electron-builder build --config electron-builder.config.js --publish never`,
      "."
    )
  },

  // Publish via electron-build
  electronPublish: (bucket) => {
    console.log(
      `Running 'electron-builder publish' and uploading to S3 bucket ${bucket}...`
    )
    execCommand(
      `electron-builder build --config electron-builder.config.js --publish always`,
      ".",
      { S3_BUCKET: bucket }
    )
  },

  // This function takes in a boolean for whether to codesign the client-app.
  // If passed false, it disables codesigning entirely, and if passed true, it
  // codesigns the existing files.
  configureCodeSigning: (enableCodeSigning) => {
    if (!(enableCodeSigning === true || enableCodeSigning === false)) {
      console.error(
        `configureCodeSigning passed a non-boolean value: ${enableCodeSigning}`
      )
      process.exit(-1)
    }

    if (enableCodeSigning) {
      if (process.platform === "darwin") {
        console.log("Codesigning everything in protocol-build/client")
        execCommand(
          'find protocol-build/client -type f -exec codesign -f -v -s "Fractal Computers, Inc." {} \\;',
          "."
        )
      } else {
        console.log(
          "We do not currently do any codesigning on non-MacOS platforms."
        )
      }
    } else {
      console.log("Disabling codesigning...")
      process.env.CSC_IDENTITY_AUTO_DISCOVERY = "false"
    }
  },

  // Function to set the packaging environment, which gets hardcoded into the
  // electron bundle via `env_overrides.json`. Acceptable arguments:
  // dev|staging|prod
  setPackagedEnv: (e) => {
    if (e === "dev" || e === "staging" || e === "prod" || e === "local") {
      addEnvOverride({ appEnvironment: e })
    } else {
      console.error(`setPackagedEnv passed a bad environment: ${e}`)
      process.exit(-1)
    }
  },

  // Function that removes env_overrides.json. It is used after the packaging
  // process is complete to prevent environment pollution of future builds and
  // `yarn start` events.
  removeEnvOverridesFile: () => {
    console.log(`Removing file ${envOverrideFile}`)
    rimrafSync(envOverrideFile)
  },

  // Function that stores the provided environment variables as key:value pairs
  // in `env_overrides.json`. This function is used to populate the client app
  // configuration with secrets we don't want to store in version control.
  populateSecretKeys: (keys) => {
    const newEnv = {}
    keys.forEach((k) => {
      newEnv[k] = process.env[k]
    })
    addEnvOverride(newEnv)
  },

  // This function reinitializes yarn to ensure clean packaging in CI
  reinitializeYarn: () => {
    // Increase Yarn network timeout, to avoid ESOCKETTIMEDOUT on weaker devices (like GitHub Action VMs)
    execCommand("yarn config set network-timeout 600000", ".")
    execCommand("yarn cache clean", ".")
    execCommand("yarn install", ".")
  },
}
