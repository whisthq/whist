const MINIMUM_EMAIL_LENGTH = 5
const MINIMUM_PASSWORD_LENGTH = 8
const PASSWORD_REQUIRE_LOWERCASE = true
const PASSWORD_REQUIRE_UPPERCASE = true
const PASSWORD_REQUIRE_NUMBER = true

const passwordLowercase = /^(?=.*[a-z])/
const passwordUppercase = /^(?=.*[A-Z])/
const passwordNumber = /^(?=.*[0-9])/

export const checkEmail = (email: string): boolean => {
    return (
        email.length >= MINIMUM_EMAIL_LENGTH &&
        email.includes("@") &&
        email.includes(".")
    )
}

export const checkPassword = (password: string): boolean => {
    if (password.length < MINIMUM_PASSWORD_LENGTH) {
        return false
    }
    if (PASSWORD_REQUIRE_LOWERCASE && !passwordLowercase.test(password)) {
        return false
    }
    if (PASSWORD_REQUIRE_UPPERCASE && !passwordUppercase.test(password)) {
        return false
    }
    if (PASSWORD_REQUIRE_NUMBER && !passwordNumber.test(password)) {
        return false
    }
    return true
}

export const checkPasswordVerbose = (password: string): string => {
    if (password.length === 0) {
        return ""
    }
    if (password.length < MINIMUM_PASSWORD_LENGTH && password.length > 0) {
        return "Too short"
    } else if (
        !passwordLowercase.test(password) &&
        PASSWORD_REQUIRE_LOWERCASE
    ) {
        return "Needs lowercase letter"
    } else if (
        !passwordUppercase.test(password) &&
        PASSWORD_REQUIRE_UPPERCASE
    ) {
        return "Needs uppercase letter"
    } else if (!passwordNumber.test(password) && PASSWORD_REQUIRE_NUMBER) {
        return "Needs number"
    } else {
        return ""
    }
}

export const checkEmailVerbose = (email: string): string => {
    if (
        email.length === 0 ||
        (email.length >= MINIMUM_EMAIL_LENGTH &&
            email.includes("@") &&
            email.includes("."))
    ) {
        return ""
    } else {
        return "Invalid email"
    }
}

export const signupEnabled = (
    email: string,
    password: string,
    confirmPassword: string
): boolean => {
    return (
        checkEmail(email) &&
        checkPassword(password) &&
        password === confirmPassword
    )
}

export const loginEnabled = (email: string, password: string): boolean => {
    return checkEmail(email) && checkPassword(password)
}
