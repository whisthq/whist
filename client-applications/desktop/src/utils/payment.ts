import { post } from "@app/utils/api"
import { FractalCallbackUrls } from "@app/config/urls"
import { store } from "@app/utils/persist"

export const allowPayments = false

export const stripeBillingPortalCreate = async () => {
  /*
    Description: 
      Makes a webserver call to get a stripe customer portal url 
    
    Arguments: 
      None
    
    Returns: 
      the stripe customer portal url 
  */

  const accessToken = store.get("accessToken") ?? ""
  const response = await post({
    endpoint: "/stripe/customer_portal",
    accessToken,
    body: {
      return_url: FractalCallbackUrls.paymentCallBack,
    },
  })
  return response?.json?.url
}
