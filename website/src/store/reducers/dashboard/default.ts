export type StripeInfo = {
    cardBrand: string | null // name of the card brand
    cardLastFour: string | null // last four digits of the user's cart
    postalCode: string | null // postal code used to calculate taxes
    plan: string | null
    stripeRequestReceived: boolean // TODO: potentially remove stripeRequestReceived and just keep stripeStatus
    stripeStatus: string | null // null | "success" | "failure"
    checkoutStatus: string | null // null | "success" | "failure"
    createdTimestamp: number | null // when the account was created
}

export type PaymentFlow = {
    plan: string
}

export const DEFAULT = {
    stripeInfo: {
        cardBrand: null, // name of the card brand
        cardLastFour: null, // last four digits of the user's cart
        postalCode: null, // postal code used to calculate taxes
        plan: null, // null | "Hourly" | "Monthly" | "Unlimited"
        stripeRequestReceived: false, // TODO: potentially remove stripeRequestReceived and just keep stripeStatus
        stripeStatus: null, // null | "success" | "failure"
        checkoutStatus: null, // null | "success" | "failure"
        createdTimestamp: null,
    },
    paymentFlow: {
        plan: null, // plan user has selected while going through checkout flow
    },
}

export default DEFAULT
