import { FractalPlan } from "shared/types/payment"

export type TestUser = {
    userID: string
    password: string
    name: string
    feedback: string
    stripeCustomerID: string
    verified: boolean
    accessGranted?: boolean
    typeformSubmitted?: boolean
}

export const payingUser: TestUser = {
    userID: "payinguser@fractal.co",
    password: "Test123!",
    name: "Paying User",
    feedback: "Test feedback",
    stripeCustomerID: "cus_IjbkVL7Ce50J5E",
    verified: true,
    accessGranted: true,
    typeformSubmitted: true,
}

export const verifiedUser: TestUser = {
    userID: "verified@fractal.co",
    password: "Test123!",
    name: "Test Name",
    feedback: "Test feedback",
    stripeCustomerID: "",
    verified: true,
    accessGranted: true,
    typeformSubmitted: true,
}

export const unverifiedUser: TestUser = {
    userID: "nonverified@fractal.co",
    password: "Test123!",
    name: "Test Name",
    feedback: "Test feedback",
    stripeCustomerID: "",
    verified: false,
    accessGranted: false,
    typeformSubmitted: false,
}

export const validFractalUser = {
    user_id: "test-user+9y4c8c@fractal.co",
    emailToken: "uIbuUHkYS6m-4iQ",
}

export const invalidTestUser = {
    email: "asdf",
    password: "asdf",
    noNumber: "Asdfasdfasdf",
    noUpper: "asdfasdfasdf1",
    noLower: "ASDFASDFASDF1",
    name: "bob",
    confirmPassword: "asdfa",
}

export const validUserCanLogin = {
    userID: "userID@fractal.co",
    name: "Valid User",
    accessToken: "accessToken",
    emailVerificationToken: "emailVerificationToken",
    emailVerified: true,
}

export const invalidUser = {
    userID: undefined,
    canLogin: false,
    accessToken: undefined,
    emailVerificationToken: undefined,
}

//// Stripe
export const validStripeInfo = {
    cardBrand: "brand",
    cardLastFour: 1000,
    postalCode: "12345",
    plan: FractalPlan.STARTER,
    stripeRequestReceived: false,
    stripeStatus: "success",
    checkoutStatus: "success",
}

export const invalidStripeInfo = {
    cardBrand: null,
    cardLastFour: null,
    postalCode: null,
    plan: null,
    stripeRequestReceived: false,
    stripeStatus: null,
    checkoutStatus: null,
}

/**
 * Create a default (starting, all empty) auth flow based on a given mode (trying to log in,
 * sign up, or query for a forgotten password).
 * @param mode the mode to start in.
 */
export const startingAuthFlow = (mode: string) => {
    return {
        mode: mode,
        loginWarning: null,
        signupWarning: null,
        signupSuccess: false,
        verificationStatus: null,
        forgotStatus: null,
        verificationEmailsSent: 0,
        forgotEmailsSent: 0,
        resetTokenStatus: null,
        resetDone: false,
        passwordResetEmail: null,
        passwordResetToken: null,
        passwordVerified: null,
        callback: null,
    }
}
