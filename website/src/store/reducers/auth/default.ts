export const DEFAULT = {
    user: {
        user_id: null,
        name: null, // not really used except for waitlist
        accessToken: null, // access token that lets you talk to the server when logged in
        refreshToken: null, // refresh token to refresh your access token when it is used up
        emailVerificationToken: null, // the token that shows up in /verify?token needed to verify email
        resetToken: null, // the token that shows up in /reset?token needed to validate a reset
        emailVerified: false, // keeps track of whether this user is verified so we can display the proper info
    },
    authFlow: {
        mode: "Log in", // what auth is doing right now (todo in the future make it an enum)
        loginWarning: null, // text to tell the user if they messed up logging in
        signupWarning: null, // text to tell the user if they messed up signing up
        verificationEmailsSent: 0, // how many emails we send
        verificationAttemptsExecuted: 0, // how many tokens someone tried to verify (know when server responded)
        resetTokenStatus: null, // null | "verified" | "expired" | "invalid"
    },
}
