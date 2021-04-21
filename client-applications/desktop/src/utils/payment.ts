import { post } from '@app/utils/api'
import { AsyncReturnType } from '@app/@types/state'

export const stripeCreateCheckout = async (
  customerId: string,
  priceId: string
) =>
  post({
    endpoint: '/stripe/create-checkout-session',
    body: { customerId, priceId }
  })

type ResponseAuth = AsyncReturnType<typeof stripeCreateCheckout>

export const stripeCheckoutValid = (response: ResponseAuth) =>
  (response?.json?.sessionId ?? '') !== ''

export const stripeCheckoutError = (response: ResponseAuth) =>
  response.status !== 200
