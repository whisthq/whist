import { post } from "@app/utils/api"
import { AsyncReturnType } from "@app/@types/state"
import { string1To1000 } from "aws-sdk/clients/customerprofiles"

export const stripeCheckoutCreate = async (
  /*
    Description: 
      Makes a webserver call to get a stripe checkout id 
    
    Arguments: 
      successUrl (str): URL to redirect to upon success 
      cancelUrl (str): URL to redirect to upon cancelation
    
    Returns: 
      JSON request containing checkout id 
  */
  successUrl: string,
  cancelUrl: string1To1000
) =>
  post({
    endpoint: "/stripe/create_checkout_session",
    body: {
      success_url: successUrl,
      cancel_url: cancelUrl,
    },
  })

export const stripePortalCreate = async (
  /*
    Description: 
      Makes a webserver call to get a stripe customer portal url 
    
    Arguments: 
      returnUrl (str): URL to redirect to upon after leaving the portal
    
    Returns: 
      JSON request containing stripe customer portal url 
  */
  returnUrl: string
) =>
  post({
    endpoint: "/stripe/customer_portal",
    body: {
      return_url: returnUrl,
    },
  })

// Error checking for checkout portal creation
type CheckoutResponseAuth = AsyncReturnType<typeof stripeCheckoutCreate>

export const stripeCheckoutValid = (response: CheckoutResponseAuth) =>
  (response?.json?.session_id ?? "") !== ""

export const stripeCheckoutError = (response: CheckoutResponseAuth) =>
  response.status !== 200

// Error checking for customer portal creation
type PortalResponseAuth = AsyncReturnType<typeof stripePortalCreate>

export const stripePortalValid = (response: PortalResponseAuth) =>
  (response?.json?.url ?? "") !== ""

export const stripePortalError = (response: PortalResponseAuth) =>
  response.status !== 200
