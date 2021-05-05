// This file contains helper functions for building, packaging, and publishing the client application.

const { execCommand } = require("./execCommand")
const { sync: rimrafSync } = require("rimraf")
const fs = require("fs")
const fse = require("fs-extra")
const path = require("path")

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
  snowpackDev: (version) => {
    console.log(`Running 'snowpack dev' with version ${version} ...`)
    execCommand("snowpack dev", ".", { VERSION: version })
  },

  // Run snowpack in build mode
  snowpackBuild: (version) => {
    console.log(`Running 'snowpack build' with version ${version} ...`)
    execCommand("snowpack build", ".", { VERSION: version })
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
  // electron bundle. Acceptable arguments: dev|staging|prod
  setPackageEnv: (e) => {
    if (e === "dev" || e === "staging" || e === "prod") {
      const env = {
        PACKAGED_ENV: e,
      }
      const envString = JSON.stringify(env)
      fs.writeFileSync("env.json", envString)
    } else {
      console.error(`setPackageEnv passed a bad environment: ${e}`)
      process.exit(-1)
    }
  },

  // This function reinitializes yarn to ensure clean packaging in CI
  reinitializeYarn: () => {
    // Increase Yarn network timeout, to avoid ESOCKETTIMEDOUT on weaker devices (like GitHub Action VMs)
    execCommand("yarn config set network-timeout 600000", ".")
    execCommand("yarn cache clean", ".")
    execCommand("yarn install", ".")
  },
}
