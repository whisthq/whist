import { app } from 'electron'
import path from 'path'
import { spawn, ChildProcess } from 'child_process'

// Temporarily pointing to the executable already installed in my applications
// folder so that I have something to launch.
//
// const iconPath = path.join(app.getAppPath(), "build/icon64.png")
//
const getProtocolName = () => {
  if (process.platform === 'win32') {
    return 'Fractal.exe'
  } else if (process.platform === 'darwin') {
    return '_Fractal'
  } else {
    return 'Fractal'
  }
}

const getProtocolFolder = () => {
  if (app.isPackaged) {
    if (process.platform === 'darwin') {
      return path.join(app.getAppPath(), '../..', 'MacOS')
    } else {
      return path.join(app.getAppPath(), '../..', 'protocol-build/client')
    }
  } else {
    return path.join(app.getAppPath(), '../../..', 'protocol-build/client')
  }
}

let environment = ''
if (process.env.NODE_ENV != null) {
  environment = process.env.NODE_ENV.trim()
}

// Protocol arguments
const protocolParameters = {
  environment: environment
}

const protocolArguments = [
  ...Object.entries(protocolParameters)
    .map(([flag, arg]) => [`--${flag}`, arg])
    .flat(),
  '--read-pipe'
]

export const protocolFolder = getProtocolFolder()

export const protocolPath = path.join(protocolFolder, getProtocolName())

export const serializePorts = (ps: {
  port_32262: number
  port_32263: number
  port_32273: number
}) => `32262:${ps.port_32262}.32263:${ps.port_32263}.32273:${ps.port_32273}`

export const writeStream = (process: ChildProcess, message: string) => {
  process.stdin?.write(message)
  process.stdin?.write('\n')
}

export const endStream = (process: ChildProcess, message: string) => {
  process.stdin?.end(message)
}

export const protocolLaunch = () => {
  if (process.platform !== 'win32') spawn('chmod', ['+x', protocolPath])

  const protocol = spawn(protocolPath, protocolArguments, {
    detached: false,
    stdio: ['pipe', process.stdout, process.stderr],

    // On packaged macOS, the protocol is moved to the MacOS folder,
    // but expects to be in the Fractal.app root alongside the loading
    // animation PNG files.
    ...(app.isPackaged &&
        process.platform === 'darwin' && {
      cwd: path.join(protocolFolder, '..')
    })
  })

  return protocol
}

export const protocolStreamInfo = (
  protocol: ChildProcess,
  info: {
    ports: {
      port_32262: number
      port_32263: number
      port_32273: number
    }
    secret_key: string
    ip: string
  }
) => {
  writeStream(protocol, `ports?${serializePorts(info.ports)}`)
  writeStream(protocol, `private-key?${info.secret_key}`)
  writeStream(protocol, `ip?${info.ip}`)
  writeStream(protocol, 'finished?0')
}

export const protocolStreamKill = (protocol: ChildProcess) => {
  writeStream(protocol, 'kill?0')
  protocol.kill('SIGINT')
}
