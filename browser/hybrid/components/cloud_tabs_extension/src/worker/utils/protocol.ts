import { HostInfo, MandelboxInfo } from "@app/@types/payload"

const serializePorts = (ps: {
  port_32262: number
  port_32263: number
  port_32273: number
}) => `32262:${ps?.port_32262}.32263:${ps?.port_32263}.32273:${ps?.port_32273}`

const serializeProtocolArgs = (info: HostInfo & MandelboxInfo) => ({
  mandelboxIP: info.mandelboxIP,
  mandelboxPorts: serializePorts(info.mandelboxPorts),
  mandelboxSecret: info.mandelboxSecret,
})

export { serializeProtocolArgs }
