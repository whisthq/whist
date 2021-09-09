// Execute a command in a cross-platform manner.
const execSync = require("child_process").execSync

module.exports = {
  execCommand: (command, cwd, env = {}, stdio = "inherit") => {
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
  },
}
