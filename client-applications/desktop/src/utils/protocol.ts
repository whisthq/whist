import { spawn, ChildProcess } from "child_process"

// Temporarily pointing to the executable already installed in my applications
// folder so that I have something to launch.
//
// const iconPath = path.join(app.getAppPath(), "build/icon64.png")
//
// const getProtocolName = () => {
//     if (process.platform !== "darwin") return "Fractal.exe"
//     if (app.isPackaged) return "./FractalClient"
//     return "./Fractal"
// }

// const protocolPath = path.join(app.getAppPath(), getProtocolName())
const protocolPath = "./protocol-build/client/Fractal"

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
    if (process.platform === "darwin") spawn("chmod", ["+x", protocolPath])

    const protocol = spawn(protocolPath, ["--read-pipe"], {
        detached: false,
        stdio: ["pipe", process.stdout, process.stderr],
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
    writeStream(protocol, `finished?0`)
}

export const protocolStreamKill = (protocol: ChildProcess) => {
    writeStream(protocol, "kill?0")
    protocol.kill("SIGINT")
}
