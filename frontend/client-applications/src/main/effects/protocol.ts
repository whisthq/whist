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
  take,
  count,
  switchMap,
  map,
  mapTo,
  startWith,
} from "rxjs/operators"
import Sentry from "@sentry/electron"

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
import { amplitudeLog, logToLogz } from "@app/main/utils/logging"
import { persistGet } from "../utils/persist"
import { AWS_REGIONS_SORTED_BY_PROXIMITY } from "@app/constants/store"

const threeProtocolFailures = fromTrigger(WhistTrigger.protocolClosed).pipe(
  filter((args: { crashed: boolean }) => args.crashed),
  withLatestFrom(fromTrigger(WhistTrigger.mandelboxFlowSuccess)),
  take(3)
)

// When launched from node.js, the protocol can take several seconds to spawn,
// which we want to avoid.

// We solve this by starting the protocol ahead of time and piping the network info
// (IP, ports, private key) to the protocol when they become available
withAppActivated(fromTrigger(WhistTrigger.protocol))
  .pipe(take(1))
  .subscribe((p) => {
    if (p === undefined)
      launchProtocol().catch((err) => Sentry.captureException(err))
  })

fromTrigger(WhistTrigger.mandelboxFlowSuccess)
  .pipe(
    withLatestFrom(
      fromTrigger(WhistTrigger.protocol),
      fromTrigger(WhistTrigger.beginImport).pipe(mapTo(true), startWith(false))
    ),
    map((x) => ({
      info: x[0],
      protocol: x[1],
      import: x[2],
    }))
  )
  .subscribe((args: { info: any; protocol: ChildProcess; import: boolean }) => {
    amplitudeLog("Creating window PROTOCOL", {
      ...args.info,
      region: JSON.stringify(
        ((persistGet(AWS_REGIONS_SORTED_BY_PROXIMITY) as any[]) ?? [])[0]
      ),
    })

    if (args.import) destroyProtocol(args.protocol)

    args.protocol === undefined || args.import
      ? launchProtocol(args.info).catch((err) => Sentry.captureException(err))
      : pipeNetworkInfo(args.protocol, args.info)
  })

// When the protocol is launched, pipe stdout to a .log file in the user's cache
fromTrigger(WhistTrigger.protocol)
  .pipe(filter((p) => p !== undefined))
  .subscribe((p) => {
    logProtocolStdoutLocally(p).catch((err) => Sentry.captureException(err))
  })

// Also send protocol logs to logz.io
const stdoutBuffer = {
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
  lines.forEach((line: string) => logToLogz(line, "protocol"))
})

fromTrigger(WhistTrigger.protocolStdoutEnd).subscribe(() => {
  // Send the last line, so long as it's not empty
  if (stdoutBuffer.buffer !== "") {
    logToLogz(stdoutBuffer.buffer, "protocol")
    stdoutBuffer.buffer = ""
  }
})

threeProtocolFailures.subscribe(([, info]: [any, any]) => {
  launchProtocol(info).catch((err) => Sentry.captureException(err))
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
fromTrigger(WhistTrigger.protocolConnection)
  .pipe(
    filter((connected: boolean) => connected),
    switchMap(() => fromEvent(app, "open-url")),
    withLatestFrom(fromTrigger(WhistTrigger.protocol))
  )
  .subscribe(([[event, url], p]: any) => {
    event.preventDefault()
    pipeURLToProtocol(p, url)
  })
