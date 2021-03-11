import { first, isEmpty } from "lodash"
import {
    E2E_AUTH_IDS,
    E2E_DASHBOARD_IDS,
    E2E_HOMEPAGE_IDS,
    LOCAL_URL,
} from "testing/utils/testIDs"

const compareUrl = async (endpoint: string) => {
    return endpoint === (await page.url()).substring(LOCAL_URL.length)
        ? ((await page.url()) as unknown)
        : (null as unknown)
}

const findByText = async (cls: string, text: string) => {
    const candidates = await page.$x(`//${cls}[contains(., \"${text}\")]`)
    if (isEmpty(candidates)) return null
    return first(candidates)
}

export const UrlVerify = async () => await compareUrl("/verify")

export const UrlDashboard = async () => await compareUrl("/dashboard")

export const UrlHome = async () => await compareUrl("")

export const ButtonVerifyBack = async () =>
    await findByText("button", "Back to Login")

export const ButtonMyAccount = async () => await findByText("a", "My Account")

export const ButtonDashboardSignout = async () =>
    await page.$("#" + E2E_DASHBOARD_IDS.SIGNOUT)

export const ButtonDashboardSignoutConfirm = async () =>
    await findByText("button", "Sign Out")

export const ButtonLoginSubmit = async () => findByText("button", "Log In")

export const ButtonSignupSubmit = async () => findByText("button", "Sign up")

export const ButtonNavbarSignin = async () => {
    const selector = "#" + E2E_HOMEPAGE_IDS.SIGNIN
    await page.waitForSelector(selector)
    return await page.$(selector)
}

export const ButtonForgotSwitch = async () =>
    await page.$("#" + E2E_AUTH_IDS.FORGOTSWITCH)

export const ButtonForgotSubmit = async () => findByText("button", "Reset")

export const ButtonSignupSwitch = async () =>
    await page.$("#" + E2E_AUTH_IDS.SIGNUPSWITCH)

export const ButtonLoginSwitch = async () =>
    (await page.$("#" + E2E_AUTH_IDS.LOGINSWITCH)) ||
    findByText("button", "Log in")

export const InputSignupEmail = async () =>
    await page.$("#" + E2E_AUTH_IDS.EMAIL)

export const InputSignupName = async () => await page.$("#" + E2E_AUTH_IDS.NAME)

export const InputSignupPassword = async () =>
    await page.$("#" + E2E_AUTH_IDS.PASSWORD)

export const InputSignupConfirmPassword = async () =>
    await page.$("#" + E2E_AUTH_IDS.CONFIRMPASSWORD)

export const InfoInvalidLogin = async () =>
    findByText("div", "Invalid email or password. Try again.")

export const LabelFooterResources = async () => findByText("div", "RESOURCES")

export const LabelFooterContact = async () => findByText("div", "CONTACT")

export const LabelFooterSales = async () => findByText("a", "Sales")

export const LabelSupport = async () => await page.$("#support")

export const LabelAbout = async () => await page.$("#about")

export const LabelCareers = async () => await page.$("#careers")

export const LabelFooterBlog = async () => await findByText("a", "Blog")

export const LabelFooterDiscord = async () =>
    await findByText("a", "Join our Discord")

export const LabelFooterLinkedIn = async () => await page.$("#linkedin")

export const LabelFooterTwitter = async () => await page.$("#twitter")

export const LabelFooterMedium = async () => await page.$("#medium")

export const LabelFooterInstagram = async () => await page.$("#instagram")

export const LabelFooterTOS = async () => await page.$("#tosPage")

export const LabelFooterPrivacy = async () => await page.$("#privacyPage")

export const LabelAboutHowItWorks = async () =>
    await findByText("h2", "How It Works")

export const LabelAboutOurStories = async () =>
    await findByText("h2", "Our Stories")

export const LabelAboutOurInvestors = async () =>
    await findByText("h2", "Our Investors")

export const LabelSignupWarningMatch = async () =>
    await findByText("div", "Doesn't match")

export const LabelSignupWarningEmail = async () =>
    await findByText("div", "Invalid email")

export const LabelSignupWarningLowercase = async () =>
    await findByText("div", "Needs lowercase letter")

export const LabelSignupWarningUppercase = async () =>
    await findByText("div", "Needs uppercase letter")

export const LabelSignupWarningNumber = async () =>
    await findByText("div", "Needs number")

export const LabelVerifyEmail = async () =>
    await findByText("div", "Check your inbox to verify your email")

export const ButtonVerifyBackToLogin = async () =>
    await findByText("button", "Back to Login")
