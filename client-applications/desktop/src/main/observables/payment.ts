import { fromEventIPC } from '@app/main/events/ipc'
import { from } from 'rxjs'
import { debugObservables, errorObservables } from '@app/utils/logging'
import {
  stripeCreateCheckout,
  stripeCheckoutValid,
  stripeCheckoutError
} from '@app/utils/payment'
import { filter, map, share, exhaustMap } from 'rxjs/operators'

export const stripeCheckoutRequest = fromEventIPC('stripeCheckoutRequest').pipe(
  filter(
    (req) => (req?.customerId ?? '') !== '' && (req?.priceId ?? '') !== ''
  ),
  map(({ customerId, priceId }) => [customerId, priceId]),
  share()
)

export const stripeCheckoutProcess = stripeCheckoutRequest.pipe(
  map(
    async ([customerId, priceId]) =>
      await stripeCreateCheckout(customerId, priceId)
  ),
  exhaustMap((req) => from(req)),
  share()
)

export const stripeCheckoutSuccess = stripeCheckoutProcess.pipe(
  filter((res) => stripeCheckoutValid(res))
)

export const stripeCheckoutFailure = stripeCheckoutProcess.pipe(
  filter((res) => stripeCheckoutError(res))
)

debugObservables(
  [stripeCheckoutRequest, 'stripeCheckoutRequest'],
  [stripeCheckoutProcess, 'stripeCheckoutProcess'],
  [stripeCheckoutSuccess, 'stripeCheckoutSuccess']
)

errorObservables([stripeCheckoutFailure, 'stripeCheckoutFailure'])
