import { app } from "electron"
import path from "path"
import { spawn, ChildProcess } from "child_process"
import config from "@app/config/environment"

const { protocolName, protocolFolder } = config

// Protocol arguments
const protocolParameters = {
  environment: config.sentryEnv,
}

const protocolArguments = [
  ...Object.entries(protocolParameters)
    .map(([flag, arg]) => [`--${flag}`, arg])
    .flat(),
  "--read-pipe",
]

export const protocolPath = path.join(protocolFolder, protocolName)

export const serializePorts = (ps: {
  port_32262: number
  port_32263: number
  port_32273: number
}) => `32262:${ps.port_32262}.32263:${ps.port_32263}.32273:${ps.port_32273}`

export const writeStream = (process: ChildProcess, message: string) => {
  process.stdin?.write(message)
  process.stdin?.write("\n")
}

export const endStream = (process: ChildProcess, message: string) => {
  process.stdin?.end(message)
}

export const protocolLaunch = () => {
  if (process.platform !== "win32") spawn("chmod", ["+x", protocolPath])

  const protocol = spawn(protocolPath, protocolArguments, {
    detached: false,
    stdio: ["pipe", process.stdout, process.stderr],

    // On packaged macOS, the protocol is moved to the MacOS folder,
    // but expects to be in the Fractal.app root alongside the loading
    // animation PNG files.
    ...(app.isPackaged &&
      process.platform === "darwin" && {
        cwd: path.join(protocolFolder, ".."),
      }),
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
  writeStream(protocol, "finished?0")
}

export const protocolStreamKill = (protocol: ChildProcess) => {
  writeStream(protocol, "kill?0")
  protocol.kill("SIGINT")
}
