/**
 * Copyright (c) 2021-2022 Whist Technologies, Inc.
 * @file app.ts
 * @brief This file contains effects that deal with the protocol
 */

import { app } from "electron"
import { ChildProcess } from "child_process"
import { fromEvent, of } from "rxjs"
import {
  filter,
  withLatestFrom,
  map,
  take,
  count,
  switchMap,
} from "rxjs/operators"

import { createTrigger, fromTrigger } from "@app/main/utils/flows"
import {
  pipeNetworkInfo,
  pipeURLToProtocol,
  destroyProtocol,
  launchProtocol,
  logProtocolStdoutLocally,
} from "@app/main/utils/protocol"
import { WhistTrigger } from "@app/constants/triggers"
import { withAppActivated, emitOnSignal } from "@app/main/utils/observables"
import { protocolToLogz } from "@app/main/utils/logging"

const threeProtocolFailures = fromTrigger(WhistTrigger.protocolClosed).pipe(
  filter((args: { crashed: boolean }) => args.crashed),
  withLatestFrom(fromTrigger(WhistTrigger.mandelboxFlowSuccess)),
  take(3)
)

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
    logProtocolStdoutLocally(p)
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

threeProtocolFailures.subscribe(([, info]: [any, any]) => {
  launchProtocol(info)
})

threeProtocolFailures
  .pipe(
    count(), // count() emits when the piped observable finishes i.e. when the protocol has failed three times
    switchMap(() => fromTrigger(WhistTrigger.protocolClosed)), // this catches the fourth failure
    take(1)
  )
  .subscribe(() => createTrigger(WhistTrigger.protocolError, of(undefined)))

// If you put your computer to sleep, kill the protocol so we don't keep your mandelbox
// running unnecessarily
emitOnSignal(
  fromTrigger(WhistTrigger.protocol),
  fromTrigger(WhistTrigger.powerSuspend)
).subscribe((p: ChildProcess) => destroyProtocol(p))

// Redirect URLs to the protocol
fromTrigger(WhistTrigger.protocolConnected)
  .pipe(
    filter((connected: boolean) => connected),
    switchMap(() => fromEvent(app, "open-url")),
    withLatestFrom(fromTrigger(WhistTrigger.protocol))
  )
  .subscribe(([[event, url], p]: any) => {
    event.preventDefault()
    pipeURLToProtocol(p, url)
  })
