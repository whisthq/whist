import { eventIPC } from '@app/main/events/ipc'

import { pluck } from 'rxjs/operators'

import { logDebug, logError, LogLevel } from '@app/utils/logging'
import { Observable, combineLatest } from 'rxjs'

import * as e from '@app/main/events/app'
import * as h from '@app/main/observables/host'
import * as c from '@app/main/observables/container'
import * as p from '@app/main/observables/protocol'
import * as s from '@app/main/observables/signup'
import * as l from '@app/main/observables/login'

const logWithEmail = (observable: Observable<any>, logs: string, level: LogLevel) => {
  if (level === LogLevel.ERROR) {
    return combineLatest(
      eventIPC.pipe(pluck('email')),
      observable
    ).subscribe(([email, _observable]) => logError(email as string, logs))
  }

  return combineLatest(
    eventIPC.pipe(pluck('email')),
    observable
  ).subscribe(([email, _observable]) => logDebug(email as string, logs))
}

const pluckLog = (observable: Observable<any>, logs: string, level: LogLevel) => {
  if (level === LogLevel.ERROR) return observable.pipe(pluck('email')).subscribe((email) => logError(email as string, logs))
  return observable
    .pipe(pluck('email'))
    .subscribe((email) => logDebug(email as string, logs))
}

// For logging observables that don't emit an email
logWithEmail(e.eventAppReady, 'App ready event emitted', LogLevel.DEBUG)
logWithEmail(
  p.protocolLaunchSuccess,
  'Protocol successfully launched',
  LogLevel.DEBUG
)
logWithEmail(p.protocolLaunchFailure, 'Protocol launch failed', LogLevel.DEBUG)
logWithEmail(
  p.protocolLaunchProcess,
  'spawn() protocol command sent',
  LogLevel.DEBUG
)

// For logging observables that already emit an email
pluckLog(l.loginRequest, 'Login attempt', LogLevel.DEBUG)

l.loginFailure.subscribe((err) =>
  logError('loginFailure', 'Could not login', err)
)

s.signupFailure.subscribe((err) =>
  logError('signupFailure', 'Could not signup', err)
)

c.containerCreateFailure.subscribe((err) =>
  logError('containerCreateFailure', 'Could create container', err)
)

c.containerAssignFailure.subscribe((err) =>
  logError('containerAssignFailure', 'Could assign container', err)
)

h.hostInfoFailure.subscribe((err) =>
  logError('hostInfoFailure', 'Could not get host service info', err)
)

h.hostConfigFailure.subscribe((err) =>
  logError('hostConfigFailure', 'Could not set host service config', err)
)

p.protocolLaunchFailure.subscribe((err) =>
  logError('protocolLaunchFailure', 'Could not launch protocol', err)
)
