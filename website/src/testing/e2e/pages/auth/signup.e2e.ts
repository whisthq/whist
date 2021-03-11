import puppeteer from "puppeteer"
import expectPuppeteer from "expect-puppeteer"
import { msWait } from "testing/utils/utils"
import { generateUser } from "testing/utils/testHelpers"
import {
    E2E_AUTH_IDS,
    E2E_HOMEPAGE_IDS,
    LOCAL_URL,
    EMAIL_API_KEY,
} from "testing/utils/testIDs"
import { invalidTestUser, TestUser } from "testing/utils/testState"
import {
    deleteInput,
    type,
    launchURL,
    deleteUserDB,
    loadHasuraToken,
} from "testing/utils/testHelpers"
import { MailSlurp } from "mailslurp-client"

const mailslurp = new MailSlurp({
    apiKey: EMAIL_API_KEY,
})
let browser: puppeteer.Browser
let page: puppeteer.Page
let emailAddress: string
let hasuraToken: string
let inbox

afterAll(async () => {
    hasuraToken = loadHasuraToken()
    const generatedUser: TestUser = {
        userID: emailAddress,
        password: "",
        name: "",
        feedback: "",
        stripeCustomerID: "",
        verified: true,
    }
    await deleteUserDB(generatedUser, hasuraToken)
})

describe("Signup page", () => {
    beforeEach(async () => {
        const { testBrowser, testPage } = await launchURL()
        browser = testBrowser
        page = testPage
    })
    afterEach(async () => {
        await browser.close()
    })
    jest.setTimeout(120000)
    /**
     * Simulates clicking a button on the screen
     * @param text text content of a button
     */
    const clickButton = async (text: string) =>
        await expectPuppeteer(page).toClick("button", { text })

    test("renders correctly", async () => {
        await expect(page).toClick("#" + E2E_HOMEPAGE_IDS.SIGNIN)
        await msWait(500)
        expect(page.url()).toMatch(LOCAL_URL + "/auth")
        // checks sign up form fields
        await expectPuppeteer(page).toMatch("Email")
        await expectPuppeteer(page).toMatch("Name")
        await expectPuppeteer(page).toMatch("Password")
        await expectPuppeteer(page).toMatch("Confirm Password")
    })

    test("fills out signup and sees warnings", async () => {
        await page.goto(LOCAL_URL)
        await expect(page).toClick("#" + E2E_HOMEPAGE_IDS.SIGNIN)
        await msWait(500)
        expect(page.url()).toMatch(LOCAL_URL + "/auth")

        await type(page, E2E_AUTH_IDS.EMAIL, invalidTestUser.email)
        await type(page, E2E_AUTH_IDS.NAME, invalidTestUser.name)
        await type(page, E2E_AUTH_IDS.PASSWORD, invalidTestUser.noUpper)
        await type(
            page,
            E2E_AUTH_IDS.CONFIRMPASSWORD,
            invalidTestUser.confirmPassword
        )

        await expectPuppeteer(page).toMatch("Needs uppercase letter")
        await expectPuppeteer(page).toMatch("Doesn't match")

        await deleteInput(page, E2E_AUTH_IDS.PASSWORD)
        await type(page, E2E_AUTH_IDS.PASSWORD, invalidTestUser.noLower)
        await expectPuppeteer(page).toMatch("Needs lowercase letter")
        await deleteInput(page, E2E_AUTH_IDS.PASSWORD)
        await type(page, E2E_AUTH_IDS.PASSWORD, invalidTestUser.noNumber)
        await expectPuppeteer(page).toMatch("Needs number")
        await expect(page.url()).toMatch(LOCAL_URL + "/auth")
    })

    test("fills out signup form and sends verification email ", async () => {
        await page.goto(LOCAL_URL)
        await expect(page).toClick("#" + E2E_HOMEPAGE_IDS.SIGNIN)
        await msWait(500)
        await expect(page.url()).toMatch(LOCAL_URL + "/auth")
        // creates new inbox
        const { password, name } = generateUser()

        inbox = await mailslurp.createInbox()
        if (inbox) {
            emailAddress = inbox.emailAddress ? inbox.emailAddress : "email"
        }
        await deleteInput(page, E2E_AUTH_IDS.PASSWORD)
        await deleteInput(page, E2E_AUTH_IDS.CONFIRMPASSWORD)

        await type(page, E2E_AUTH_IDS.EMAIL, emailAddress)
        await type(page, E2E_AUTH_IDS.NAME, name)
        await type(page, E2E_AUTH_IDS.PASSWORD, password)
        await type(page, E2E_AUTH_IDS.CONFIRMPASSWORD, password)

        await clickButton("Sign up")

        await msWait(2000)

        await expectPuppeteer(page).toMatch(
            "Check your inbox to verify your email"
        )
        // gets email body
        const email = await mailslurp.waitForLatestEmail(inbox.id)
        const body = email.body
        if (body) {
            // const aStart = body.indexOf("http")
            // const aEnd = body.indexOf(` style = "text-decoration: none">`)

            // const url = body.substring(aStart, aEnd)
            // await expect(url).toContain("redirect.fractal.co")
            // await page.goto(url)
            expect(body).toContain("Verify email address")
        }

        // await clickButton("Back to Login")
        await msWait(5000)
        // expect(page.url()).toMatch(LOCAL_URL + "/auth")
    })
})
