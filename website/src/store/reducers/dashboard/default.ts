export const DEFAULT = {
    stripeInfo: {
        cardBrand: null, // name of the card brand
        cardLastFour: null, // last four digits of the user's cart
        postalCode: null, // postal code used to calculate taxes
        plan: null, // null | "Hourly" | "Monthly" | "Unlimited"
        stripeRequestRecieved: false, // TODO: potentially remove stripeRequestRecieved and just keep stripeStatus
        stripeStatus: null, // null | "success" | "failure"
        checkoutStatus: null, // null | "success" | "failure"
    },
    paymentFlow: {
        plan: null, // plan user has selected while going through checkout flow
    },
}
