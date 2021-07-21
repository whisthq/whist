/**
 * Copyright Fractal Computers, Inc. 2021
 * @file payment.ts
 * @brief This file contains utility functions for interacting with Stripe.
 */

import { post } from "@app/utils/api"
import { FractalCallbackUrls } from "@app/config/urls"
import { store } from "@app/utils/persist"

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
