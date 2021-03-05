export enum EmailSendStatus {
    PROCESSING = "Processing"
    EMAIL_SENT = "Email sent"
}

export enum EmailServerResponse {
    SUCCESS = "We've sent you a password reset email."
    UNAUTHORIZED = "Your email was not recognized as a registered email."
    FAILURE = "Oops! Our servers unexpectedly lost connection."
}

export enum PasswordServerResponse {
    SUCCESS = "Your password has been reset successfully."
    UNAUTHORIZED = "Unexpected error: unauthorized password reset."
    FAILURE = "Oops! Our servers unexpectedly lost connection."
}