/*
    This file contains all the code used to interact with the protocol through the client-app. Everything -- launching, passing args, and closing args -- should be done here.
 */

import { app } from "electron"
import path from "path"
import fs from "fs"
import { spawn, ChildProcess } from "child_process"
import config, {
  getLoggingBaseFilePath,
  loggingFiles,
} from "@app/config/environment"

const { protocolName, protocolFolder } = config

// Protocol arguments
// We send the environment so that the protocol can init sentry if necessary
const protocolParameters = {
  environment: config.sentryEnv,
}

const protocolArguments = [
  ...Object.entries(protocolParameters)
    .map(([flag, arg]) => [`--${flag}`, arg])
    .flat(),
  "--read-pipe",
]

export const protocolPath = path.join(protocolFolder, protocolName)

export const serializePorts = (ps: {
  port_32262: number
  port_32263: number
  port_32273: number
}) => `32262:${ps.port_32262}.32263:${ps.port_32263}.32273:${ps.port_32273}`

export const writeStream = (process: ChildProcess, message: string) => {
  process.stdin?.write(message)
  process.stdin?.write("\n")
}

export const endStream = (process: ChildProcess, message: string) => {
  process.stdin?.end(message)
}

// Spawn the child process with the initial arguments passed in
export const protocolLaunch = async () => {
  if (process.platform !== "win32") spawn("chmod", ["+x", protocolPath])

  // Create a pipe to the protocol logs file
  const loggingBaseFilePath = getLoggingBaseFilePath()
  fs.mkdirSync(loggingBaseFilePath, { recursive: true })
  const protocolLogFile = fs.createWriteStream(
    path.join(loggingBaseFilePath, loggingFiles.protocol)
  )

  // In order to pipe a child process to this stream, we must wait until an underlying file
  // descriptor is created. This corresponds to the "open" event in the stream.
  await new Promise<void>((resolve) => {
    protocolLogFile.on("open", () => resolve())
  })

  const protocol = spawn(protocolPath, protocolArguments, {
    detached: false,
    // options are for [stdin, stdout, stderr]. pipe creates a pipe, ignore will ignore the
    // output. We only pipe stdin since that's how we send args to the protocol. Meanwhile,
    // we will write stdout to the log file client.log.
    stdio: ["pipe", protocolLogFile, "ignore"],

    // On packaged macOS, the protocol is moved to the MacOS folder,
    // but expects to be in the Fractal.app root alongside the loading
    // animation PNG files.
    ...(app.isPackaged &&
      process.platform === "darwin" && {
        cwd: path.join(protocolFolder, ".."),
      }),
    ...(app.isPackaged &&
      process.platform !== "darwin" && {
        cwd: path.join(protocolFolder, "../.."),
      }),
  })
  return protocol
}

// Stream the rest of the info that the protocol needs
export const protocolStreamInfo = (
  protocol: ChildProcess,
  info: {
    ports: {
      port_32262: number
      port_32263: number
      port_32273: number
    }
    secret_key: string
    ip: string
  }
) => {
  writeStream(protocol, `ports?${serializePorts(info.ports)}`)
  writeStream(protocol, `private-key?${info.secret_key}`)
  writeStream(protocol, `ip?${info.ip}`)
  writeStream(protocol, "finished?0")
}

export const protocolStreamKill = (protocol: ChildProcess) => {
  writeStream(protocol, "kill?0")
  protocol.kill("SIGINT")
}
