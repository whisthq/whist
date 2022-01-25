/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file protocol.ts
 * @brief This file contains utility functions launching the protocol ChildProcess. This file contains
 * all the code used to interact with the protocol through the client-app. Everything -- launching
 * passing args, and closing args -- should be done here.
 */

import { app } from "electron"
import path from "path"
import events from "events"
import fs from "fs"
import { spawn, ChildProcess } from "child_process"

import { appEnvironment, WhistEnvironments } from "../../../config/configs"
import { sessionID, MAX_URL_LENGTH } from "@app/constants/app"
import { electronLogPath } from "@app/main/utils/logging"
import config, { loggingFiles } from "@app/config/environment"

const { protocolName, protocolFolder } = config

const protocolPath = path.join(protocolFolder, protocolName)
const protocol = new events.EventEmitter()
// Helper functions

const serializePorts = (ps: {
  port_32262: number
  port_32263: number
  port_32273: number
}) => `32262:${ps?.port_32262}.32263:${ps?.port_32263}.32273:${ps?.port_32273}`

const pipeToProtocol = (process: ChildProcess | undefined, message: string) => {
  try {
    process?.stdin?.write?.(message)
  } catch (err) {
    console.error(err)
  }
}

// Main functions

const launchProtocol = async (info?: {
  mandelboxIP: string
  mandelboxSecret: string
  mandelboxPorts: {
    port_32262: number
    port_32263: number
    port_32273: number
  }
}) => {
  // Protocol arguments
  const protocolParameters = {
    // If non-local, send the environment and session id for sentry
    ...(appEnvironment !== WhistEnvironments.LOCAL && {
      environment: config.deployEnv,
      "session-id": sessionID.toString(),
    }),
  }

  const protocolArguments = [
    ...Object.entries(protocolParameters)
      .map(([flag, arg]) => [`--${flag}`, arg])
      .flat(),
    "--read-pipe",
  ]

  const child = spawn(protocolPath, protocolArguments, {
    detached: false,
    // options are for [stdin, stdout, stderr]. pipe creates a pipe, ignore will ignore the
    // output. We pipe stdin since that's how we send args to the protocol. We pipe stdout
    // and later use that pipe to write logs to `protocol.log` and potentially stdout.
    stdio: ["pipe", "pipe", "ignore"],

    // On packaged macOS, the protocol is moved to the MacOS folder,
    // but expects to be in the Whist.app root alongside the loading
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

  protocol.emit("launched", child)

  if (info !== undefined) pipeNetworkInfo(child, info)
}

// Stream the rest of the info that the protocol needs
const pipeNetworkInfo = (
  childProcess: ChildProcess,
  info: {
    mandelboxIP: string
    mandelboxSecret: string
    mandelboxPorts: {
      port_32262: number
      port_32263: number
      port_32273: number
    }
  }
) => {
  pipeToProtocol(
    childProcess,
    `ports?${serializePorts(info.mandelboxPorts)}\nprivate-key?${
      info.mandelboxSecret
    }\nip?${info.mandelboxIP}\nfinished?0\n`
  )
}

const destroyProtocol = (childProcess: ChildProcess) => {
  // We send SIGINT just in case
  pipeToProtocol(childProcess, "kill?0\n")
}

const pipeURLToProtocol = (childProcess: ChildProcess, message: string) => {
  if (
    message === undefined ||
    message === "" ||
    message.length > MAX_URL_LENGTH
  )
    return
  pipeToProtocol(childProcess, `new-tab-url?${message}\n`)
}

const logProtocolStdoutLocally = async (childProcess: ChildProcess) => {
  // Create a pipe to the protocol logs file
  if (!fs.existsSync(electronLogPath))
    fs.mkdirSync(electronLogPath, { recursive: true })

  const protocolLogFile = fs.createWriteStream(
    path.join(electronLogPath, loggingFiles.protocol)
  )

  // In order to pipe a child process to this stream, we must wait until an underlying file
  // descriptor is created. This corresponds to the "open" event in the stream.
  await new Promise<void>((resolve) => {
    protocolLogFile.on("open", () => resolve())
  })

  childProcess?.stdout?.pipe(protocolLogFile)

  // Also print protocol logs in terminal if requested by the developer
  if (process.env.SHOW_PROTOCOL_LOGS === "true")
    childProcess?.stdout?.pipe(process.stdout)
}

export {
  protocol,
  launchProtocol,
  pipeNetworkInfo,
  destroyProtocol,
  pipeURLToProtocol,
  logProtocolStdoutLocally,
}
