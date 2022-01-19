/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file protocol.ts
 * @brief This file contains utility functions launching the protocol ChildProcess. This file contains
 * all the code used to interact with the protocol through the client-app. Everything -- launching
 * passing args, and closing args -- should be done here.
 */

import { app } from "electron"
import path from "path"
import fs from "fs"
import { spawn, ChildProcess } from "child_process"
import config, { loggingFiles } from "@app/config/environment"
import {
  electronLogPath,
  protocolToLogz,
  logBase,
} from "@app/main/utils/logging"
import { appEnvironment, WhistEnvironments } from "../../../config/configs"
import logRotate from "log-rotate"
import { MAX_URL_LENGTH } from "@app/constants/app"

const NACK_LOOKBACK_PERIOD_IN_MS = 1500 // Number of milliseconds to look back when measuring # of nacks
const MAX_NACKS_ALLOWED = 6 // Maximum # of nacks allowed before we decide the network is unstable
let protocolConnected = false

export let childProcess: ChildProcess | undefined
// Current time in UNIX (seconds)
let lastNackTime = Date.now() / 1000
// Track how many nacks there were
let numberOfRecentNacks = 0
// Initialize log rotation
logRotate(
  path.join(electronLogPath, loggingFiles.protocol),
  { count: 4 },
  (err: any) => console.error(err)
)

const { protocolName, protocolFolder } = config

export const protocolPath = path.join(protocolFolder, protocolName)

export const serializePorts = (ps: {
  port_32262: number
  port_32263: number
  port_32273: number
}) => `32262:${ps?.port_32262}.32263:${ps?.port_32263}.32273:${ps?.port_32273}`

export const writeStream = (
  process: ChildProcess | undefined,
  message: string
) => {
  try {
    process?.stdin?.write?.(message)
  } catch (err) {
    console.error(err)
  }
}

// Spawn the child process with the initial arguments passed in
export const protocolLaunch = async () => {
  if (childProcess !== undefined) return childProcess

  // Protocol arguments
  // We send the environment so that the protocol can init sentry if necessary
  const protocolParameters = {
    ...(appEnvironment !== WhistEnvironments.LOCAL && {
      environment: config.deployEnv,
    }),
  }

  const protocolArguments = [
    ...Object.entries(protocolParameters)
      .map(([flag, arg]) => [`--${flag}`, arg])
      .flat(),
    "--read-pipe",
  ]

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

  const protocol = spawn(protocolPath, protocolArguments, {
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

  // Pipe to protocol.log
  protocol.stdout.pipe(protocolLogFile)

  // Pipe protocol's stdout to logz.io

  // Some shared buffer to store stdout messages in
  const stdoutBuffer = {
    buffer: "",
  }
  // This will separate and pipe the protocol's output into send_protocol_log_line
  protocol.stdout.on("data", (msg: string) => {
    // Combine the previous line with the current msg
    const newmsg = `${stdoutBuffer.buffer}${msg}`
    // Split on newline
    const lines = newmsg.split(/\r?\n/)
    // Leave the last line in the buffer to be appended to later
    stdoutBuffer.buffer = lines.length === 0 ? "" : (lines.pop() as string)
    // Print the rest of the lines
    lines.forEach((line: string) => protocolToLogz(line))
  })
  // When the datastream ends, send the last line out
  protocol.stdout.on("end", () => {
    // Send the last line, so long as it's not empty
    if (stdoutBuffer.buffer !== "") {
      protocolToLogz(stdoutBuffer.buffer)
      stdoutBuffer.buffer = ""
    }
  })

  // If true, also show in terminal (for local debugging)
  if (process.env.SHOW_PROTOCOL_LOGS === "true")
    protocol.stdout.pipe(process.stdout)

  // When the protocol closes, reset the childProcess to undefined and show the app dock on MacOS
  protocol.on("close", () => {
    childProcess = undefined
    protocolConnected = false
  })

  childProcess = protocol
  return protocol
}

// Stream the rest of the info that the protocol needs
export const protocolStreamInfo = (info: {
  mandelboxIP: string
  mandelboxSecret: string
  mandelboxPorts: {
    port_32262: number
    port_32263: number
    port_32273: number
  }
}) => {
  if (childProcess === undefined) return

  writeStream(
    childProcess,
    `ports?${serializePorts(info.mandelboxPorts)}\nprivate-key?${
      info.mandelboxSecret
    }\nip?${info.mandelboxIP}\nfinished?0\n`
  )
}

export const protocolStreamKill = () => {
  // We send SIGINT just in case
  writeStream(childProcess, "kill?0\n")
}

export const protocolOpenUrl = (message: string) => {
  if (message === undefined || message === "") {
    logBase("Attempted to open undefined/empty URL in new tab", {})
    return
  }
  if (message.length > MAX_URL_LENGTH) {
    logBase(
      `Attempted to open URL of length that exceeds ${MAX_URL_LENGTH}`,
      {}
    )
    return
  }
  logBase(`Sending URL ${message} to protocol to open in new tab!\n`, {})
  writeStream(childProcess, `new-tab-url?${message}\n`)
}

export const isNetworkUnstable = (message?: string) => {
  const currentTime = Date.now() / 1000
  if (message?.toString()?.includes("STATISTIC") ?? false)
    protocolConnected = true
  if (!protocolConnected) return false
  // eslint-disable-next-line @typescript-eslint/strict-boolean-expressions
  if (message?.toString().includes("NACKING")) {
    // Check when the last nack happened
    if ((currentTime - lastNackTime) * 1000 < NACK_LOOKBACK_PERIOD_IN_MS) {
      // If the last nack happened less than three seconds ago, increase # of nacks
      numberOfRecentNacks += 1
    } else {
      // If the last nack was more than three seconds ago, reset timer
      numberOfRecentNacks = 1
      lastNackTime = currentTime
    }

    return numberOfRecentNacks > MAX_NACKS_ALLOWED
  }

  return (currentTime - lastNackTime) * 1000 < NACK_LOOKBACK_PERIOD_IN_MS
}
