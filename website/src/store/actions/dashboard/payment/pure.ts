export const UPDATE_STRIPE_INFO = "UPDATE_STRIPE_INFO"
export const UPDATE_PAYMENT_FLOW = "UPDATE_PAYMENT_FLOW"

export function updateStripeInfo(body: {
    subscription?: null
    cards?: number
    stripeRequestRecieved?: boolean
    stripeStatus?: string | null
}) {
    return {
        type: UPDATE_STRIPE_INFO,
        body,
    }
}

export function updatePaymentFlow(body: { plan?: string }) {
    return {
        type: UPDATE_PAYMENT_FLOW,
        body,
    }
}
