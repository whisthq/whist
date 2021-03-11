export type User = {
    userID?: string | null // email
    name?: string | null // name
    // session auth tokens
    accessToken?: string | null // access token that lets you talk to the server when logged in
    refreshToken?: string | null // refresh token to refresh your access token when it is used up
    configToken?: string | null // config token for encrypting app configs
    // email tokens
    emailVerificationToken?: string | null // the token that shows up in /verify?token needed to verify email
    resetToken?: string | null // the token that shows up in /reset?token needed to validate a reset
    // verification status for this specific user
    emailVerified?: boolean // keeps track of whether this user is verified so we can display the proper info
    usingGoogleLogin?: boolean // if the user signed in using Google
}

export type AuthFlow = {
    mode?: string // what auth is doing right now (todo in the future make it an enum) https://github.com/fractal/website/issues/339
    // error message
    loginWarning?: string | null // text to tell the user if they messed up logging in (primarily used as warning)
    signupWarning?: string | null // text to tell the user if they messed up signing up (primarily used as warning)
    signupSuccess?: boolean
    verificationStatus?: string | null // null | "success" | "failed"
    forgotStatus?: string | null // text/status info about what happend to the forgot email request sent (status and warning)
    // verify emails
    verificationEmailsSent?: number // how many emails we send
    // reset password
    forgotEmailsSent?: number
    resetTokenStatus?: string | null // null | "verified" | "invalid", may want to handle expired tokens in the future
    resetDone?: boolean // whether we are done resetting our password (i.e. server says "I've done it")
    passwordResetEmail?: string | null // forgot password email
    passwordResetToken?: string | null
    passwordVerified?: string | null // null | "success" | "failed"
    callback?: string | undefined // the URL to "call back" to (usually "fractal://")
    token?: string | null // password reset token for testing password reset
    url?: string | null // password rest url for testing pasword reset
}

export type StripeInfo = {
    cardBrand: string | null // name of the card brand
    cardLastFour: string | null // last four digits of the user's cart
    postalCode: string | null // postal code used to calculate taxes
    plan: string | null
    stripeRequestReceived: boolean // TODO: potentially remove stripeRequestReceived and just keep stripeStatus (https://github.com/fractal/website/issues/336)
    stripeStatus: string | null // null | "success" | "failure"
    checkoutStatus: string | null // null | "success" | "failure"
    createdTimestamp: number | null // when the account was created
}

export type PaymentFlow = {
    plan: string // plan user has selected while going through checkout flow
}
