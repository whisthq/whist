import { app } from "electron"
import remove from "lodash.remove"

const relaunch = (options: { quietLaunch: boolean }) => {
  // First destroy all Electron windows
  let args = process.argv.slice(1)
  remove(args, (arg) => arg === "--quietLaunch")
  args = options.quietLaunch ? args.concat(["--quietLaunch"]) : args

  // Then quit/relaunch the app
  app.relaunch({ args })

  // Finally quit the app
  app.quit()
}

export { relaunch }
