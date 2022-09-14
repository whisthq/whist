export interface AuthInfo {
  userEmail?: string
  accessToken?: string
  refreshToken?: string
  error?: {
    message: string
    data: {
      status: number
      json: {
        id_token?: string
        access_token?: string
        refresh_token?: string
      }
    }
  }
  isFirstAuth: boolean
}

export interface MandelboxInfo {
  mandelboxIP: string
  mandelboxID: string
}

export interface HostInfo {
  mandelboxSecret: string
  mandelboxPorts: {
    port_32261: number
    port_32262: number
    port_32263: number
    port_32273: number
  }
}

export interface ConfigTokenInfo {
  token?: string
  isNew: boolean
}
