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
import { spawn, ChildProcess } from "child_process"

import { appEnvironment, WhistEnvironments } from "../../../config/configs"
import { sessionID, MAX_URL_LENGTH } from "@app/constants/app"
import config from "@app/config/environment"

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
    "disable-features": "LongTermReference",
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

  // The protocol ChildProcess can throw errors if we try to write to it as it is
  // dying. We don't really care about these errors; we just want to try/catch them
  // and adding this listener accomplishes that
  child.on("error", (err: any) => console.error(err))
  child?.stdin?.on("error", (err) => console.error(err))
  child?.stdout?.on("error", (err) => console.error(err))

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
    message.length > MAX_URL_LENGTH ||
    !(message.startsWith("http://") || message.startsWith("https://"))
  )
    return
  pipeToProtocol(childProcess, `new-tab-url?${message}\n`)
}

export {
  protocol,
  launchProtocol,
  pipeNetworkInfo,
  destroyProtocol,
  pipeURLToProtocol,
}
