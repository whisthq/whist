import { post } from "@app/utils/api"
import { FractalCallbackUrls } from "@app/config/urls"
import { store } from "@app/utils/persist"

const stripeBillingPortalCreate = (accessToken: any) =>
  /*
    Description: 
      Makes a webserver call to get a stripe customer portal url 
    
    Arguments: 
      accessToken (str): access token for the user
    
    Returns: 
      JSON response containing stripe customer portal url 
  */
  post({
    endpoint: "/stripe/customer_portal",
    accessToken,
    body: {
      return_url: FractalCallbackUrls,
    },
  })

const accessToken = store.get("accessToken") ?? ""
export const billingPortalURL =
  stripeBillingPortalCreate(accessToken)?.json?.url
