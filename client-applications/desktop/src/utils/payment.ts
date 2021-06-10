import { post } from "@app/utils/api"
import { FractalCallbackUrls } from "@app/config/urls"

const stripeBillingPortalCreate = () =>
  /*
    Description: 
      Makes a webserver call to get a stripe customer portal url 
    
    Arguments: 
      returnUrl (str): URL to redirect to upon after leaving the portal
    
    Returns: 
      JSON request containing stripe customer portal url 
  */
  post({
    endpoint: "/stripe/customer_portal",
    body: {
      return_url: FractalCallbackUrls,
    },
  })

export const billingPortalURL = stripeBillingPortalCreate()?.json?.url
