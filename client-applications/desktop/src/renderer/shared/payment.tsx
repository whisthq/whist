import React, { useState, useEffect } from "react"
import { useMainState } from "@app/utils/ipc"
import { useStripe } from "@stripe/react-stripe-js"

interface StripeContextInterface {
  action: string
  stripeCheckoutId: string
  stripePortalUrl: string
  getCheckoutSession: (
    priceId: string,
    customerId: string,
    successUrl: string,
    cancelUrl: string
  ) => void
  getPortalSession: (customerId: string, returnUrl: string) => void
}

const StripeContext = React.createContext<StripeContextInterface>({
  action: "",
  stripeCheckoutId: "",
  stripePortalUrl: "",
  getCheckoutSession: () => {},
  getPortalSession: () => {},
})

export const StripeProvider = ({ children }: any) => {
  /**
   *  Stripe Provider for client app payment flow, responsible for
   *  redirecting the user to the customer checkout portal and the customer portal, where
   *  the user can pay, edit subscription information, change payment methods, etc.
   *
   *  This provider aims to expose functions getCheckoutSession() and getPortalSession()
   *  to the rest of the client app, whereever we choose the payment flow to be. The heart
   *  of its functionality is the single IPC channel of communication between the renderer
   *  and mani thread through setMainState(). Rxjs observables then update the mainState with
   *  the necessary stripe info, allowing our useEffect hooks to parse and act accordingly.
   *
   *  The provider also exposes extra values, such as the checkout session id or the
   *  customer portal url, in case another component wishes to use them for redirection
   *  purposes.
   *
   */
  const [mainState, setMainState] = useMainState()
  const [action, setAction] = useState("")
  const [stripeCheckoutId, setStripeCheckoutId] = useState("")
  const [stripePortalUrl, setStripePortalUrl] = useState("")
  const stripe = useStripe()

  const getCheckoutSession = async (
    priceId: string,
    customerId: string,
    successUrl: string,
    cancelUrl: string
  ) => {
    const body = {
      priceId: "plan_J632kqGTMkcro6",
      customerId: "cus_JKiqEsRwvk9gds",
      successUrl: "https://www.fractal.co/",
      cancelUrl: "https://www.fractal.co/",
    }
    setMainState({
      stripeCheckoutRequest: { priceId, customerId, successUrl, cancelUrl },
      stripeAction: {},
    })
  }

  const getPortalSession = async (customerId: string, returnUrl: string) => {
    const body = {
      customerId: "cus_JKiqEsRwvk9gds",
      returnUrl: "https://www.fractal.co/",
    }
    setMainState({
      stripePortalRequest: { customerId, returnUrl },
      stripeAction: {},
    })
  }

  useEffect(() => {
    if (mainState.stripeAction) {
      setAction(mainState.stripeAction.action ?? "")
      mainState.stripeAction.action === "CHECKOUT"
        ? setStripeCheckoutId(mainState.stripeAction.stripeCheckoutId ?? "")
        : setStripePortalUrl(mainState.stripeAction.stripePortalUrl ?? "")
    }
  }, [mainState])

  useEffect(() => {
    const redirect = async () => {
      if (stripe != null) {
        await stripe.redirectToCheckout({
          sessionId: stripeCheckoutId,
        })
      }
    }
    if (action === "CHECKOUT") {
      redirect()
        .then((res) => console.log(res))
        .catch((err) => console.log(err))
    } else if (action === "PORTAL") {
      window.location.href = stripePortalUrl
    }
  }, [action])

  return (
    <StripeContext.Provider
      value={{
        action,
        stripeCheckoutId,
        stripePortalUrl,
        getCheckoutSession,
        getPortalSession,
      }}
    >
      {children}
    </StripeContext.Provider>
  )
}

export const StripeConsumer = StripeContext.Consumer

export default StripeContext
