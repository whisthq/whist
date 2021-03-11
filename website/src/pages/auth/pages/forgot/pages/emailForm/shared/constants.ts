export class EmailStatus  {
    static SUCCESS = {
        title: "We've sent you a password reset email.",
        text: "If you don't see an email, please check your spam folder."
    }
    static UNAUTHORIZED = {
        title: "Your email has not been verified",
        text: "We sent you an email verification email so you can verify your account first."
    }
    static FAILURE = {
        title: "Error: Server returned an unexpected response",
        text: "We apologize for this, please contact support@fractal.co for assistance!"
    }
}