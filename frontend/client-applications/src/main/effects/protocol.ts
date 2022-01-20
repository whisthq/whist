/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains subscriptions to Observables related to protocol launching.
 */

import Sentry from "@sentry/electron"
import fs from "fs"
import path from "path"
import { ChildProcess } from "child_process"
import { of } from "rxjs"
import { map, filter, withLatestFrom } from "rxjs/operators"

import config, { loggingFiles } from "@app/config/environment"
import { fromTrigger } from "@app/main/utils/flows"
import {
  pipeNetworkInfo,
  pipeURLToProtocol,
  destroyProtocol,
  launchProtocol,
} from "@app/main/utils/protocol"
import { WhistTrigger } from "@app/constants/triggers"
import { withAppActivated } from "@app/main/utils/observables"
import {
  electronLogPath,
  protocolToLogz,
  logBase,
} from "@app/main/utils/logging"

// When launched from node.js, the protocol can take several seconds to spawn,
// which we want to avoid.

// We solve this by starting the protocol ahead of time and piping the network info
// (IP, ports, private key) to the protocol when they become available
withAppActivated(of(null)).subscribe(() => launchProtocol())

fromTrigger(WhistTrigger.mandelboxFlowSuccess)
  .pipe(withLatestFrom(fromTrigger(WhistTrigger.protocol)))
  .subscribe(
    ([info, protocol]: [
      {
        mandelboxIP: string
        mandelboxSecret: string
        mandelboxPorts: {
          port_32262: number
          port_32263: number
          port_32273: number
        }
      },
      ChildProcess | undefined
    ]) => {
      protocol === undefined
        ? launchProtocol(info)
        : pipeNetworkInfo(protocol, info)
    }
  )

// When the protocol is launched, pipe stdout to a .log file in the user's cache
fromTrigger(WhistTrigger.protocol)
  .pipe(filter((p) => p !== undefined))
  .subscribe(async (p) => {
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

    p.stdout.pipe(protocolLogFile)

    // Also print protocol logs in terminal if requested by the developer
    if (process.env.SHOW_PROTOCOL_LOGS === "true") p.stdout.pipe(process.stdout)
  })

// Also send protocol logs to logz.io
let stdoutBuffer = {
  buffer: "",
}

fromTrigger(WhistTrigger.protocolStdoutData).subscribe((data: string) => {
  // Combine the previous line with the current msg
  const newmsg = `${stdoutBuffer.buffer}${data}`
  // Split on newline
  const lines = newmsg.split(/\r?\n/)
  // Leave the last line in the buffer to be appended to later
  stdoutBuffer.buffer = lines.length === 0 ? "" : (lines.pop() as string)
  // Print the rest of the lines
  lines.forEach((line: string) => protocolToLogz(line))
})

fromTrigger(WhistTrigger.protocolStdoutEnd).subscribe(() => {
  // Send the last line, so long as it's not empty
  if (stdoutBuffer.buffer !== "") {
    protocolToLogz(stdoutBuffer.buffer)
    stdoutBuffer.buffer = ""
  }
})
