// This file contains helper functions for building, packaging, and publishing the client application.

const { execCommand } = require("./execCommand")
const { execSync } = require("child_process")
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

const getMacOSArchConfig = () => {
  return {
    "": {
      electronBuilderFlag: "",
      electronBuilderInstallAppDepsFlag: "",
      publishS3BucketInfix: "",
      cmakeArguments: "",
    },
    x86_64: {
      electronBuilderFlag: "--x64",
      electronBuilderInstallAppDepsFlag: "--arch x64",
      publishS3BucketInfix: "",
      cmakeArguments: "-D MACOS_TARGET_ARCHITECTURE=x86_64",
    },
    arm64: {
      electronBuilderFlag: "--arm64",
      electronBuilderInstallAppDepsFlag: "--arch arm64",
      publishS3BucketInfix: "-arm64",
      cmakeArguments: "-D MACOS_TARGET_ARCHITECTURE=arm64",
    },
  }[process.env.MACOS_ARCH ?? ""]
}

// This function gets the name of the publish bucket for AWS S3
const getPublishS3BucketName = (environment) => {
  switch (process.platform) {
    case "darwin":
      return `fractal-chromium-macos${
        getMacOSArchConfig().publishS3BucketInfix
      }-${environment}`
    case "win32":
      return `fractal-chromium-windows-${environment}`
    case "linux":
      return `fractal-chromium-ubuntu-${environment}`
  }
}

const configOS = () => {
  if (process.platform === "win32") return "win32"
  if (process.platform === "darwin") return "macos"
  return "linux"
}

const configImageTag = "whist/config"

const gitRepoRoot = "../.."
const configDirectory = path.join(gitRepoRoot, "config")
const protocolSourceDir = path.join(gitRepoRoot, "protocol")
const cmakeBuildDir = "build-clientapp"
const protocolTargetDir = "WhistProtocolClient"
const protocolTargetBuild = path.join(protocolTargetDir, "build")
const protocolTargetDebug = path.join(protocolTargetDir, "debugSymbols")

module.exports = {
  buildConfigContainer: () => {
    // Build the docker image from whist/config/Dockerfile
    // When the --quiet flag is used, then the stdout of docker build
    // will be the sha256 hash of the image.
    console.log(`Building ${configImageTag} Docker image`)
    const build = `docker build --tag ${configImageTag} ${configDirectory}`
    execSync(build, { encoding: "utf-8", stdio: "pipe" }).trim()
  },
  getConfig: (params = {}) => {
    // Using the params argument, we'll build some strings that pass options
    // to the config CLI. Everything should be coerced to JSON strings first.
    const os = `--os ${JSON.stringify(params.os ?? configOS())}`
    const secrets = params.secrets
      ? `--secrets ${JSON.stringify(params.secrets)}`
      : ""
    const deploy = params.deploy
      ? `--deploy ${JSON.stringify(params.deploy)}`
      : ""
    console.log(`Parsing config with: ${secrets} ${os} ${deploy}`)

    // Pass the sha256 hash of the image to docker run, removing the created
    // container afterwards. The output of these will be a JSON string
    // of the config object.
    const run = `docker run --rm ${configImageTag} ${secrets} ${os} ${deploy}`
    return execSync(run, { encoding: "utf-8", stdio: "pipe" })
  },
  // Build the protocol and copy it into the expected location
  buildAndCopyProtocol: (freshCmake) => {
    console.log("Building the protocol...")

    if (
      freshCmake &&
      fs.existsSync(path.join(protocolSourceDir, cmakeBuildDir))
    ) {
      fs.rmSync(path.join(protocolSourceDir, cmakeBuildDir), {
        recursive: true,
        force: true,
      })
    }

    if (!fs.existsSync(path.join(protocolSourceDir, cmakeBuildDir))) {
      if (process.platform === "win32") {
        // On Windows, cmake wants rc.exe but is provided node_modules/.bin/rc.js
        // Hence, we must modify the path to pop out the node_modules entries.
        const pathArray = process.env.Path.split(";")
        while (
          ["node_modules", "yarn", "Yarn"].some((v) => pathArray[0].includes(v))
        )
          pathArray.shift()
        const path = pathArray.join(";")
        execCommand(
          `cmake -S . -B ${cmakeBuildDir} -G Ninja`,
          protocolSourceDir,
          {
            Path: path,
          }
        )
      } else if (process.platform === "darwin") {
        execCommand(
          `cmake -S . -B ${cmakeBuildDir} ${
            getMacOSArchConfig().cmakeArguments
          }`,
          protocolSourceDir
        )
      } else {
        execCommand(`cmake -S . -B ${cmakeBuildDir}`, protocolSourceDir)
      }
    }

    execCommand(
      `cmake --build ${cmakeBuildDir} -j --target WhistClient`,
      protocolSourceDir
    )

    console.log("Copying over the built protocol...")

    rimrafSync(protocolTargetDir)

    // Copy protocol binaries to WhistProtocolClient/build
    fse.copySync(
      path.join(protocolSourceDir, cmakeBuildDir, "client/build64"),
      protocolTargetBuild,
      {
        filter: (src) => !src.endsWith(".debug"),
      }
    )

    // Copy protocol debug symbols to WhistProtocolClient/debug
    fse.copySync(
      path.join(protocolSourceDir, cmakeBuildDir, "client/build64"),
      protocolTargetDebug,
      {
        filter: (src) => src.endsWith(".debug"),
      }
    )

    const ext = process.platform === "win32" ? ".exe" : ""
    const oldExecutable = "WhistClient"
    const newExecutable =
      process.platform === "darwin" ? "WhistClient" : "Whist"

    if (oldExecutable !== newExecutable) {
      fse.moveSync(
        path.join(protocolTargetBuild, `${oldExecutable}${ext}`),
        path.join(protocolTargetBuild, `${newExecutable}${ext}`)
      )
    }

    rimrafSync("images")
    fse.moveSync(path.join(protocolTargetBuild, "images"), "images")
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

  // Build via electron-builder
  electronBuild: () => {
    console.log("Running 'electron-builder build'...")
    execCommand(
      `electron-builder build --config electron-builder.config.js --publish never ${
        process.platform === "darwin"
          ? getMacOSArchConfig().electronBuilderFlag
          : ""
      }`,
      "."
    )
  },

  // Build note native modules via electron-builder
  electronBuilderInstallAppDeps: () => {
    console.log("Recompiling node modules for your computer...")
    execCommand(
      `electron-builder install-app-deps ${
        process.platform === "darwin"
          ? getMacOSArchConfig().electronBuilderInstallAppDepsFlag
          : ""
      }`
    )
  },

  // Publish via electron-build
  electronPublish: (environment) => {
    const bucket = getPublishS3BucketName(environment)
    console.log(
      `Running 'electron-builder publish' and uploading to S3 bucket ${bucket}...`
    )
    execCommand(
      `electron-builder build --config electron-builder.config.js --publish always ${
        process.platform === "darwin"
          ? getMacOSArchConfig().electronBuilderFlag
          : ""
      }`,
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
        console.log(`Codesigning everything in ${protocolTargetBuild}...`)
        execCommand(
          `find ${protocolTargetBuild} -type f -exec codesign -f -v -s "Fractal Computers, Inc." {} \\;`,
          "."
        )
      } else {
        console.log("Codesigning is only used for macOS.")
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

  // Function to set add the CONFIG= environment variabled to the overrides.
  setPackagedConfig: (config) => addEnvOverride({ CONFIG: config }),

  // Function to set the commit sha for the packaged app, which gets
  // hardcoded into the electron bundle via `env_overrides.json`.
  // Acceptable arguments: a git commit sha
  setPackagedCommitSha: (e) => {
    if (e.length === 40 && /^[a-f0-9]+$/.test(e)) {
      addEnvOverride({ COMMIT_SHA: e })
    } else {
      console.error(`setPackagedCommitSha passed a bad commit sha: ${e}`)
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
