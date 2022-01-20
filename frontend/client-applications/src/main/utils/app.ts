import { app } from "electron"
import { ChildProcess } from "child_process"

import { destroyProtocol } from "@app/main/utils/protocol"

const relaunch = (childProcess: ChildProcess, options?: { args: string[] }) => {
  destroyProtocol(childProcess)
  options === undefined ? app.relaunch() : app.relaunch(options)
  app.quit()
}

export { relaunch }
