export const DEFAULT = {
    user: {
        // identifiers
        user_id: null, //usually email
        name: null, // not really used except for waitlist
        // session auth tokens
        accessToken: null, // access token that lets you talk to the server when logged in
        refreshToken: null, // refresh token to refresh your access token when it is used up
        // email tokens
        emailVerificationToken: null, // the token that shows up in /verify?token needed to verify email
        resetToken: null, // the token that shows up in /reset?token needed to validate a reset
        // verification status for this specific user
        emailVerified: false, // keeps track of whether this user is verified so we can display the proper info
    },
    authFlow: {
        mode: "Log in", // what auth is doing right now (todo in the future make it an enum)
        // error message
        loginStatus: null, // text to tell the user if they messed up logging in (primarily used as warning)
        signupStatus: null, // text to tell the user if they messed up signing up (primarily used as warning)
        forgotStatus: null, // text/status info about what happend to the forgot email request sent (status and warning)
        // verify email
        verificationEmailsSent: 0, // how many emails we send
        verificationAttemptsExecuted: 0, // how many tokens someone tried to verify (know when server responded)
        // google login
        googleLoginStatus: null, // null | an http status (used to display warnings)
        // reset password
        resetTokenStatus: null, // null | "verified" | "expired" | "invalid"
        resetDone: false, // whether we are done resetting our password (i.e. server says "I've done it")
    },
}
