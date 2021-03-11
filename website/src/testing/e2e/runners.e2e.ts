import puppeteer from "puppeteer"
import { MailSlurp } from "mailslurp-client"
import { EMAIL_API_KEY } from "testing/utils/testIDs"
import { verifiedUser, invalidTestUser } from "testing/utils/testState"
import {
    goHome,
    goForgotPage,
    goSignupPage,
    maybeLogout,
    doLogin,
    doLogout,
    doLoginInvalid,
    fillFormForgot,
    fillFormSignup,
} from "testing/utils/actions"
import {
    insertUserDBInvite,
    deleteUserDBInvite,
    generateUser,
    launchURL,
} from "testing/utils/testHelpers"
import {
    ButtonVerifyBackToLogin,
    LabelFooterResources,
    LabelFooterContact,
    LabelFooterSales,
    LabelSupport,
    LabelAbout,
    LabelCareers,
    LabelFooterBlog,
    LabelFooterDiscord,
    LabelFooterLinkedIn,
    LabelFooterTwitter,
    LabelFooterMedium,
    LabelFooterInstagram,
    LabelAboutHowItWorks,
    LabelAboutOurInvestors,
    LabelAboutOurStories,
    LabelFooterTOS,
    LabelFooterPrivacy,
    LabelSignupWarningUppercase,
    LabelSignupWarningEmail,
    LabelSignupWarningMatch,
    LabelVerifyEmail,
} from "testing/utils/selectors"
import {
    expectClickNavigate,
    expectElement,
    waitForElement,
} from "testing/utils/queries"

beforeEach(async () => {
    await insertUserDBInvite(verifiedUser)
})

afterEach(async () => {
    await deleteUserDBInvite(verifiedUser)
})

test("true", () => {
    true
})

test("Invalid users cannot login", async () => {
    const { email, password } = generateUser()
    await goHome(page)
    await doLoginInvalid(page, email, password)
})

test("Logs in and logs out", async () => {
    await goHome(page)
    await maybeLogout(page)
    await doLogin(page, verifiedUser.userID, verifiedUser.password)
    await doLogout(page)
})

test("Renders footer and links", async () => {
    await goHome(page)
    await expectElement(page, LabelFooterResources)
    await expectElement(page, LabelFooterContact)
    await expectElement(page, LabelFooterSales)
    await expectElement(page, LabelSupport)
    await expectElement(page, LabelAbout)
    await expectElement(page, LabelCareers)
    await expectElement(page, LabelFooterBlog)
    await expectElement(page, LabelFooterDiscord)
    await expectElement(page, LabelFooterLinkedIn)
    await expectElement(page, LabelFooterTwitter)
    await expectElement(page, LabelFooterMedium)
    await expectElement(page, LabelFooterInstagram)
    await expectClickNavigate(page, LabelFooterTOS)
    await goHome(page)
    await expectClickNavigate(page, LabelFooterPrivacy)
    await goHome(page)
})

test("Renders header and links", async () => {
    await goHome(page)
    await expectElement(page, LabelAbout)
})

test("Renders about page", async () => {
    await goHome(page)
    await expectClickNavigate(page, LabelAbout)
    await expectElement(page, LabelAboutHowItWorks)
    await expectElement(page, LabelAboutOurStories)
    await expectElement(page, LabelAboutOurInvestors)
})

test("Fills out signup and sees warnings", async () => {
    await goHome(page)
    await goSignupPage(page)
    await Promise.all([
        fillFormSignup(
            page,
            invalidTestUser.email,
            invalidTestUser.name,
            invalidTestUser.noUpper,
            invalidTestUser.confirmPassword
        ),
        waitForElement(page, LabelSignupWarningUppercase),
        waitForElement(page, LabelSignupWarningEmail),
        waitForElement(page, LabelSignupWarningMatch),
    ])
    await goHome(page)
})

test("Fills out signup and sends verification email", async () => {
    const { password, name, email } = generateUser()

    await goHome(page)
    await goSignupPage(page)
    await fillFormSignup(page, email, name, password, password)
    await waitForElement(page, LabelVerifyEmail)
    await expectClickNavigate(page, ButtonVerifyBackToLogin)
    await goHome(page)
    await maybeLogout(page)
})

test("Submit forgot password request", async () => {
    const mailSlurp = new MailSlurp({ apiKey: EMAIL_API_KEY })
    const inbox = await mailSlurp.createInbox()
    if (!inbox.emailAddress)
        throw new Error("Could not create MailSlurp inbox.")

    await goHome(page)
    await maybeLogout(page)
    await goForgotPage(page)
    await fillFormForgot(page, inbox.emailAddress)
})
