import puppeteer from "puppeteer"
import expectPuppeteer from "expect-puppeteer"
import { msWait } from "testing/utils/utils"
import {
    E2E_AUTH_IDS,
    E2E_HOMEPAGE_IDS,
    LOCAL_URL,
    EMAIL_API_KEY,
} from "testing/utils/testIDs"
import { verifiedUser } from "testing/utils/testState"
import { MailSlurp } from "mailslurp-client"
import {
    generateUser,
    deleteInput,
    type,
    launchURL,
    insertUserDB,
    deleteUserDB,
    loadHasuraToken,
} from "testing/utils/testHelpers"

const mailslurp = new MailSlurp({
    apiKey: EMAIL_API_KEY,
})
let browser: puppeteer.Browser
let page: puppeteer.Page
let hasuraToken: string
let inbox

beforeAll(async () => {
    hasuraToken = loadHasuraToken()
    await insertUserDB(verifiedUser, hasuraToken)
})

afterAll(async () => {
    await deleteUserDB(verifiedUser, hasuraToken)
})

describe("Login Page", () => {
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
        await page.goto(LOCAL_URL)
        // sign in page
        await expectPuppeteer(page).toClick("#" + E2E_HOMEPAGE_IDS.SIGNIN)
        await msWait(500)
        expect(page.url()).toMatch(LOCAL_URL + "/auth")
        // log in page
        await expectPuppeteer(page).toClick("#" + E2E_AUTH_IDS.LOGINSWITCH)
        await msWait(2000)
        // looks at email fields
        await expectPuppeteer(page).toMatch("Email")
        await expectPuppeteer(page).toMatch("Password")
    })

    test("Logs in a valid user", async () => {
        await page.goto(LOCAL_URL)
        // sign in page
        await expectPuppeteer(page).toClick("#" + E2E_HOMEPAGE_IDS.SIGNIN)
        expect(page.url()).toMatch(LOCAL_URL + "/auth")
        // log in page
        await expectPuppeteer(page).toClick("#" + E2E_AUTH_IDS.LOGINSWITCH)
        // fills out fields
        await type(page, E2E_AUTH_IDS.EMAIL, verifiedUser.userID)
        await type(page, E2E_AUTH_IDS.PASSWORD, verifiedUser.password)

        await clickButton("Log In")
        await msWait(4000)

        // checks if on dashboard
        expect(page.url()).toMatch(LOCAL_URL + "/dashboard")
    })

    test("Submit forgot password request", async () => {
        await page.goto(LOCAL_URL)
        // sign in page
        await expect(page).toClick("#" + E2E_HOMEPAGE_IDS.SIGNIN)
        await msWait(500)
        await expect(page.url()).toMatch(LOCAL_URL + "/auth")
        // creates new inbox
        const { password, name } = generateUser()
        let emailAddress = ""
        inbox = await mailslurp.createInbox()
        if (inbox) {
            emailAddress = inbox.emailAddress ? inbox.emailAddress : "email"
        }
        // deletes any leftover text in fields
        await deleteInput(page, E2E_AUTH_IDS.PASSWORD)
        await deleteInput(page, E2E_AUTH_IDS.CONFIRMPASSWORD)

        // creates account by signing up a new user
        await type(page, E2E_AUTH_IDS.EMAIL, emailAddress)
        await type(page, E2E_AUTH_IDS.NAME, name)
        await type(page, E2E_AUTH_IDS.PASSWORD, password)
        await type(page, E2E_AUTH_IDS.CONFIRMPASSWORD, password)

        await clickButton("Sign up")

        await msWait(4000)

        await expectPuppeteer(page).toMatch(
            "Check your inbox to verify your email"
        )
        await msWait(2000)

        await clickButton("Back to Login")
        await msWait(2000)
        // log in page
        await expectPuppeteer(page).toClick("#" + E2E_AUTH_IDS.LOGINSWITCH)

        // forgot password page
        await expectPuppeteer(page).toClick("#" + E2E_AUTH_IDS.FORGOTSWITCH)
        await msWait(3000)

        //fills out email
        await type(page, E2E_AUTH_IDS.EMAIL, emailAddress)
        await clickButton("Reset")

        await msWait(6000)
        // waits for mail in inbox
        const email = await mailslurp.waitForLatestEmail(inbox.id)
        const body = email.body

        // checks if email body contains token
        expect(body).toContain("Forgot your password?")
    })
})
