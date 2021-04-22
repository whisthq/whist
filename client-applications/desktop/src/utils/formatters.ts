import { ChildProcess } from 'child_process'
import { omit } from 'lodash'

export const formatChildProcess = (process: ChildProcess) =>
  omit(process, [
    'stdin',
    'stdio',
    'stdout',
    'stderr',
    '_events',
    '_eventsCount',
    '_closesNeeded',
    '_closesGot',
    '_handle',
  ])
