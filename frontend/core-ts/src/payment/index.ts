import { config } from "../config"
import { configGet } from "../"
import { accessToken, subscriptionStatus } from "../types/data"

const get = configGet({ server: config.WEBSERVER_URL })

export const paymentPortalRequest = async ({ accessToken }: accessToken) => {
  /*
    Description:
      Makes a webserver call to get a stripe customer portal url

    Arguments:
      {
        accessToken: string
      }

    Returns:
      the stripe customer portal url
  */
  return await get({
    endpoint: "/payment_portal_url",
    accessToken,
  })
}

export const paymentPortalParse = (res: any) => {
  const url = res?.json?.url
  if (typeof url === "string") return { paymentPortalURL: url }
  return {
    error: {
      message: "No json.url string found.",
      data: res,
    },
  }
}

export const hasValidSubscription = ({
  subscriptionStatus,
}: subscriptionStatus): boolean => {
  return ["active", "trialing"].includes(subscriptionStatus)
}
