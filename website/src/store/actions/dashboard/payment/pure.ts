export const UPDATE_STRIPE_INFO = "UPDATE_STRIPE_INFO"
export const UPDATE_PAYMENT_FLOW = "UPDATE_PAYMENT_FLOW"

export function updateStripeInfo(body: {
    cardBrand?: string | null
    cardLastFour?: string | null
    postalCode?: string | null
    plan?: string | null
    stripeRequestRecieved?: boolean
    stripeStatus?: string | null
    checkoutStatus?: string | null
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
