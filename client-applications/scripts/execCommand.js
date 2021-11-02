// Execute a command in a cross-platform manner.
const execSync = require("child_process").execSync

const execCommand = (command, cwd, env = {}, stdio = "inherit") => {
  try {
    const output = execSync(command, {
      encoding: "utf-8",
      cwd: cwd,
      env: { ...process.env, ...env },
      stdio: stdio,
    })
    return output
  } catch (error) {
    console.error(`Error running "${command}": ${error}`)
    process.exit(-1)
  }
}

module.exports = {
  execCommandByOS: (
    macCommand,
    linuxCommand,
    windowsCommand,
    cwd,
    env = {},
    stdio = "inherit"
  ) => {
    const currentPlatform = process.platform
    if (currentPlatform === "darwin") {
      if (macCommand != null) {
        return execCommand(macCommand, cwd, env, stdio)
      } else {
        return null
      }
    } else if (currentPlatform === "win32" || currentPlatform === "win64") {
      if (windowsCommand != null) {
        return execCommand(windowsCommand, cwd, env, stdio)
      } else {
        return null
      }
    } else if (currentPlatform === "linux") {
      if (linuxCommand != null) {
        return execCommand(linuxCommand, cwd, env, stdio)
      } else {
        return null
      }
    } else {
      console.error(`Error detecting platform!`)
      process.exit(-1)
    }
  },
}
