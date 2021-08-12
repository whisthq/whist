// auth
export type accessToken = { accessToken: string }
export type refreshToken = { refreshToken: string }
export type configToken = { configToken: string }
export type subscriptionStatus = { subscriptionStatus: string }

export type authCallbackURL = { authCallbackURL: string }

// mandelbox
export type mandelboxIP = { mandelboxIP: string }
export type mandelboxPorts = {
  mandelboxPorts: {
    port_32262: number
    port_32263: number
    port_32273: number
  }
}
export type mandelboxSecret = { mandelboxSecret: string }
export type regionAWS = { regionAWS: string }

// user
export type userEmail = { userEmail: string }
export type userPassword = { userPassword: string }

export type paymentPortalURL = { paymentPortalURL: string }
