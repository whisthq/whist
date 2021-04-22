import { post } from '@app/utils/api'
import { AsyncReturnType } from '@app/@types/state'
import { string1To1000 } from 'aws-sdk/clients/customerprofiles'

export const stripeCheckoutCreate = async (
  customerId: string,
  priceId: string,
  successUrl: string,
  cancelUrl: string1To1000
) =>
  post({
    endpoint: '/stripe/create-checkout-session',
    body: { customerId, priceId, successUrl, cancelUrl }
  })

export const stripePortalCreate = async (
  customerId: string,
  returnUrl: string
) =>
  post({
    endpoint: '/stripe/customer-portal',
    body: { customerId, returnUrl }
  })

type CheckoutResponseAuth = AsyncReturnType<typeof stripeCheckoutCreate>

export const stripeCheckoutValid = (response: CheckoutResponseAuth) =>
  (response?.json?.sessionId ?? '') !== ''

export const stripeCheckoutError = (response: CheckoutResponseAuth) =>
  response.status !== 200

type PortalResponseAuth = AsyncReturnType<typeof stripePortalCreate>

export const stripePortalValid = (response: PortalResponseAuth) =>
  (response?.json?.url ?? '') !== ''

export const stripePortalError = (response: PortalResponseAuth) =>
  response.status !== 200
