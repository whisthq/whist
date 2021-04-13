// This file is home to observables that manage launching the protocol process
// on the user's machine. They are distinct from the observables that
// communicate with the webserver to manage container creation.
//
// Many of these observables emit the protocol ChildProcess object, which
// carries important data about the state of the protocol process.

import { protocolLaunch } from '@app/utils/protocol'
import {
  containerInfoIP,
  containerInfoPorts,
  containerInfoSecretKey
} from '@app/utils/container'
import { LogLevel, debug } from '@app/utils/logging'
import {
  containerAssignRequest,
  containerAssignSuccess,
  containerAssignFailure
} from '@app/main/observables/container'
import { hostConfigFailure } from '@app/main/observables/host'
import { loadingFrom } from '@app/utils/observables'
import { zip, of, fromEvent, merge } from 'rxjs'
import { map, filter, share, mergeMap } from 'rxjs/operators'
import { pick } from 'lodash'

export const protocolLaunchProcess = containerAssignRequest.pipe(
  map(() => protocolLaunch()),
  share(),
  debug(LogLevel.DEBUG, 'protocolLaunchProcess', 'value:', ({
    connected, exitCode, pid
  }) => ({
    connected,
    exitCode,
    pid
  }))
)

export const protocolLaunchSuccess = containerAssignSuccess.pipe(
  map((res) => ({
    ip: containerInfoIP(res),
    secret_key: containerInfoSecretKey(res),
    ports: containerInfoPorts(res)
  })),
  debug(LogLevel.DEBUG, 'protocolLaunchSuccess')
)

export const protocolLaunchFailure = merge(
  hostConfigFailure,
  containerAssignFailure
).pipe(debug(LogLevel.ERROR, 'protocolLaunchFailure', 'error:'))

export const protocolLoading = loadingFrom(
  protocolLaunchProcess,
  protocolLaunchSuccess,
  protocolLaunchFailure
).pipe(debug(LogLevel.DEBUG, 'protocolLoading'))

export const protocolCloseRequest = protocolLaunchProcess.pipe(
  mergeMap((protocol) => zip(of(protocol), fromEvent(protocol, 'close'))),
  debug(LogLevel.DEBUG, 'protocolCloseRequest', 'printing subset of protocol object:', ([protocol]) =>
    pick(protocol, ['killed', 'connected', 'exitCode', 'signalCode', 'pid'])
  )
)

export const protocolCloseFailure = protocolCloseRequest.pipe(
  filter(([protocol]) => protocol.killed)
)

export const protocolCloseSuccess = protocolCloseRequest.pipe(
  filter(([protocol]) => !(protocol.killed as boolean))
)
