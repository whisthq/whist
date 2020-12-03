export const DEFAULT = {
    user: {
        // identifiers
        userID: null, // email
        name: null, // not really used except for waitlist
        // session auth tokens
        accessToken: null, // access token that lets you talk to the server when logged in
        refreshToken: null, // refresh token to refresh your access token when it is used up
        // email tokens
        emailVerificationToken: null, // the token that shows up in /verify?token needed to verify email
        resetToken: null, // the token that shows up in /reset?token needed to validate a reset
        // verification status for this specific user
        emailVerified: false, // keeps track of whether this user is verified so we can display the proper info
        canLogin: false, // are they off the waitlist?
        waitlistToken: null,
        usingGoogleLogin: false, // if the user signed in using Google
        cardBrand: null, // brand of card the user has added
        cardLastFour: null, // last four digits of the card the user has added, should be a string
        postalCode: null, // two character code for billing state used to get tax rate
        plan: null, // name of plan user is subscribed to, null || "hourly" | "unlimited"
    },
    authFlow: {
        mode: "Sign up", // what auth is doing right now (todo in the future make it an enum)
        // error message
        loginWarning: null, // text to tell the user if they messed up logging in (primarily used as warning)
        signupWarning: null, // text to tell the user if they messed up signing up (primarily used as warning)
        signupSuccess: false,
        verificationStatus: null, // null | "success" | "failed"
        forgotStatus: null, // text/status info about what happend to the forgot email request sent (status and warning)
        // verify emails
        verificationEmailsSent: 0, // how many emails we send
        // reset password
        forgotEmailsSent: 0,
        resetTokenStatus: null, // null | "verified" | "expired" | "invalid"
        resetDone: false, // whether we are done resetting our password (i.e. server says "I've done it")
        passwordResetEmail: null, // forgot password email
        passwordResetToken: null,
        passwordVerified: null, // null | "success" | "failed"
        callback: null,
    },
}
