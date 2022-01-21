import { app } from "electron"

const relaunch = (options?: { sleep: boolean }) => {
  options?.sleep ?? false
    ? app.relaunch({ args: process.argv.slice(1).concat(["--sleep"]) })
    : app.relaunch()
  app.quit()
}

export { relaunch }
