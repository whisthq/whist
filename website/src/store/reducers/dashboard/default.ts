export const DEFAULT = {
    stripeInfo: {
        cardBrand: null, // name of the card brand
        cardLastFour: null, // last four digits of the user's cart
        postalCode: null, // postal code used to calculate taxes
        plan: null, // null | "Hourly" | "Monthly" | "Unlimited"
        stripeRequestReceived: false, // TODO: potentially remove stripeRequestReceived and just keep stripeStatus (https://github.com/fractal/website/issues/336)
        stripeStatus: null, // null | "success" | "failure"
        checkoutStatus: null, // null | "success" | "failure"
        createdTimestamp: null,
    },
    paymentFlow: {
        plan: null, // plan user has selected while going through checkout flow
    },
}

export default DEFAULT
