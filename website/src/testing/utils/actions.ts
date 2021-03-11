import { LOCAL_URL } from "testing/utils/testIDs"
import puppeteer from "puppeteer"
import {
    ButtonLoginSubmit,
    ButtonLoginSwitch,
    ButtonMyAccount,
    ButtonNavbarSignin,
    ButtonForgotSwitch,
    ButtonForgotSubmit,
    ButtonSignupSwitch,
    ButtonSignupSubmit,
    ButtonVerifyBack,
    InputSignupEmail,
    InputSignupPassword,
    UrlVerify,
    UrlDashboard,
    UrlHome,
    ButtonDashboardSignout,
    InfoInvalidLogin,
    ButtonDashboardSignoutConfirm,
    InputSignupName,
    InputSignupConfirmPassword,
} from "testing/utils/selectors"
import {
    expectClickNavigate,
    maybeClick,
    expectClick,
    waitForElement,
    expectElement,
    maybeClickNavigate,
} from "testing/utils/queries"

export const goHome = async (page: puppeteer.Page) => {
    if (page.url() !== LOCAL_URL)
        await page.goto(LOCAL_URL, { waitUntil: "domcontentloaded" })
    await expectElement(page, UrlHome)
}

export const goLoginPage = async (page: puppeteer.Page) => {
    await goHome(page)
    await expectClickNavigate(page, ButtonNavbarSignin)
    await maybeClick(page, ButtonLoginSwitch)
    await waitForElement(page, InputSignupEmail)
    await waitForElement(page, InputSignupPassword)
}

export const goSignupPage = async (page: puppeteer.Page) => {
    await goHome(page)
    await expectClickNavigate(page, ButtonNavbarSignin)
    await maybeClick(page, ButtonSignupSwitch)
    await waitForElement(page, InputSignupEmail)
    await waitForElement(page, InputSignupPassword)
}

export const goForgotPage = async (page: puppeteer.Page) => {
    await goHome(page)
    await goLoginPage(page)
    await expectClick(page, ButtonForgotSwitch)
    await expectElement(page, ButtonForgotSubmit)
}

export const fillFormLogin = async (
    page: puppeteer.Page,
    email: string,
    password: string
) => {
    const emailForm = await expectElement(page, InputSignupEmail)
    const passwordForm = await expectElement(page, InputSignupPassword)
    await emailForm!.type(email)
    await passwordForm!.type(password)
    await expectClick(page, ButtonLoginSubmit)
}

export const fillFormSignup = async (
    page: puppeteer.Page,
    email: string,
    name: string,
    password: string,
    confirm: string
) => {
    const emailForm = await expectElement(page, InputSignupEmail)
    const nameForm = await expectElement(page, InputSignupName)
    const passwordForm = await expectElement(page, InputSignupPassword)
    const confirmForm = await expectElement(page, InputSignupConfirmPassword)
    await page.evaluate((f) => (f.value = ""), emailForm)
    await page.evaluate((f) => (f.value = ""), nameForm)
    await page.evaluate((f) => (f.value = ""), passwordForm)
    await page.evaluate((f) => (f.value = ""), confirmForm)
    await emailForm!.type(email)
    await nameForm!.type(name)
    await passwordForm!.type(password)
    await confirmForm!.type(confirm)
    await expectClick(page, ButtonSignupSubmit)
}

export const fillFormForgot = async (page: puppeteer.Page, email: string) => {
    const emailForm = await expectElement(page, InputSignupEmail)
    await emailForm!.type(email)
    await expectClick(page, ButtonForgotSubmit)
}

export const doLogout = async (page: puppeteer.Page) => {
    await goHome(page)
    await expectClickNavigate(page, ButtonMyAccount)
    await expectClick(page, ButtonDashboardSignout)
    await waitForElement(page, ButtonDashboardSignoutConfirm)
    await expectClickNavigate(page, ButtonDashboardSignoutConfirm)
}

export const maybeLogout = async (page: puppeteer.Page) => {
    await goHome(page)
    await maybeClickNavigate(page, ButtonMyAccount)
    if (await UrlVerify()) await expectClickNavigate(page, ButtonVerifyBack)
    if (await UrlDashboard()) {
        await expectClick(page, ButtonDashboardSignout)
        await waitForElement(page, ButtonDashboardSignoutConfirm)
        await expectClickNavigate(page, ButtonDashboardSignoutConfirm)
    }
    await goHome(page)
}

export const doLogin = async (
    page: puppeteer.Page,
    email: string,
    password: string
) => {
    await goLoginPage(page)
    await fillFormLogin(page, email, password)
    await Promise.race([
        waitForElement(page, UrlDashboard),
        waitForElement(page, UrlVerify),
        waitForElement(page, InfoInvalidLogin),
    ])
    let message = "Did not navigate to dashboard after pressing 'Login'."
    if (await InfoInvalidLogin())
        message = "Verified user's login met with 'Invalid email or Password'."
    if (await UrlVerify())
        message = "Verfied user was sent to verify page instead of dashboard."

    await expectElement(page, UrlDashboard, doLogin, message)
}

export const doLoginInvalid = async (
    page: puppeteer.Page,
    email: string,
    password: string
) => {
    await goLoginPage(page)
    await fillFormLogin(page, email, password)
    await waitForElement(page, InfoInvalidLogin)
}
