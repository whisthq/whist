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
import config from "@app/config/environment"
import { appEnvironment, WhistEnvironments } from "../../../config/configs"
import { MAX_URL_LENGTH } from "@app/constants/app"

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
  // We send the environment so that the protocol can init sentry if necessary
  const protocolParameters = {
    ...(appEnvironment !== WhistEnvironments.LOCAL && {
      environment: config.deployEnv,
      ...(info !== undefined && {
        ports: serializePorts(info.mandelboxPorts),
        ip: info.mandelboxIP,
        "private-key": info.mandelboxSecret,
      }),
    }),
  }

  const protocolArguments = [
    ...Object.entries(protocolParameters)
      .map(([flag, arg]) => [`--${flag}`, arg])
      .flat(),
    ...(info === undefined ? ["--read-pipe"] : []),
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

export {
  protocol,
  launchProtocol,
  pipeNetworkInfo,
  destroyProtocol,
  pipeURLToProtocol,
}
