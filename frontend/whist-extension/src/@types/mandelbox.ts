export type MandelboxInfo = {
  ip: string
  mandelboxID: string
}

export type HostInfo = {
  secret: string
  ports: {
    port_32262: number
    port_32263: number
    port_32273: number
  }
}
