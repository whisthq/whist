export const DEFAULT = {
    stripeInfo: {
        // TODO add some information regarding stripe endpoints and whatnot
        subscription: null,
        cards: 0, // here we will want to put a list of some sort of object that we can visualize
        // stripeRequestRecieved below is
        // meant to signal the UI so it can check the subscription and see if
        // it was properly set
        // use this in a useEffect
        // should be set to false previously
        stripeRequestRecieved: false, // TODO: potentially remove stripeRequestRecieved and just keep stripeStatus
        stripeStatus: null, // null | "success" | "failure"
    },
    paymentFlow: {
        plan: null, // plan user has selected while going through checkout flow
    },
}
