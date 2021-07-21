import { config } from "../config"
import { configPost } from "../"
import { accessToken } from "../types/data"

const post = configPost({ server: config.WEBSERVER_URL })

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
    return await post({
        endpoint: "/stripe/customer_portal",
        accessToken,
        body: {
            return_url: "http://localhost/callback?payment",
        },
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
