export const UPDATE_STRIPE_INFO = "UPDATE_STRIPE_INFO"

export function updateStripeInfo(body: {}) {
    return {
        type: UPDATE_STRIPE_INFO,
        body,
    }
}
