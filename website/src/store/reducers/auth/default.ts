export const DEFAULT = {
    user: {
        // identifiers
        userID: null, // email
        name: null, // name
        // session auth tokens
        accessToken: null, // access token that lets you talk to the server when logged in
        refreshToken: null, // refresh token to refresh your access token when it is used up
        configToken: null, // config token to encrypt user configs
        // email tokens
        emailVerificationToken: null, // the token that shows up in /verify?token needed to verify email
        resetToken: null, // the token that shows up in /reset?token needed to validate a reset
        // verification status for this specific user
        emailVerified: false, // keeps track of whether this user is verified so we can display the proper info
        usingGoogleLogin: false, // if the user signed in using Google
    },
    authFlow: {
        mode: "Sign up", // what auth is doing right now (todo in the future make it an enum) https://github.com/fractal/website/issues/339
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
        resetTokenStatus: null, // null | "verified" | "invalid", may want to handle expired tokens in the future
        resetDone: false, // whether we are done resetting our password (i.e. server says "I've done it")
        passwordResetEmail: null, // forgot password email
        passwordResetToken: null,
        passwordVerified: null, // null | "success" | "failed"
        callback: null,
        // fields for testing reset password
        token: null, //reset password token
        url: null, // should be dev.fractal.co
    },
}

export default DEFAULT
