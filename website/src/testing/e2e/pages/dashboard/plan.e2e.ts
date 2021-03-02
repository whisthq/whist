import puppeteer from "puppeteer"
import expectPuppeteer from "expect-puppeteer"
import { msWait } from "testing/utils/utils"
import {
    E2E_AUTH_IDS,
    E2E_DASHBOARD_IDS,
    E2E_HOMGEPAGE_IDS,
    LOCAL_URL,
} from "testing/utils/testIDs"
import { verifiedUser } from "testing/utils/testState"
import {
    type,
    launchURL,
    insertUserDB,
    deleteUserDB,
    insertUserInvite,
    deleteUserInvite,
    loadHasuraToken,
} from "testing/utils/testHelpers"

let browser: puppeteer.Browser
let page: puppeteer.Page
let hasuraToken: string

beforeAll(async () => {
    hasuraToken = loadHasuraToken()
    await insertUserDB(verifiedUser, hasuraToken)
    await insertUserInvite(verifiedUser, hasuraToken)
})

afterAll(async () => {
    await deleteUserInvite(verifiedUser, hasuraToken)
    await deleteUserDB(verifiedUser, hasuraToken)
})

describe("Dashboard Plan page", () => {
    beforeEach(async () => {
        const { testBrowser, testPage } = await launchURL()
        browser = testBrowser
        page = testPage
    })
    afterEach(async () => {
        if (browser) {
            await browser.close()
        }
    })
    jest.setTimeout(120000)

    /**
     * Simulates clicking a button on the screen
     * @param text text content of a button
     */
    const clickButton = async (text: string) =>
        await expectPuppeteer(page).toClick("button", { text })

    test("Logs in and adds a plan", async () => {
        await page.goto(LOCAL_URL)
        await expectPuppeteer(page).toClick("#" + E2E_HOMGEPAGE_IDS.SIGNIN)
        expect(page.url()).toMatch(LOCAL_URL + "/auth")

        await expectPuppeteer(page).toClick("#" + E2E_AUTH_IDS.LOGINSWITCH)
        await type(page, E2E_AUTH_IDS.EMAIL, verifiedUser.userID)
        await type(page, E2E_AUTH_IDS.PASSWORD, verifiedUser.password)

        await clickButton("Log In")
        await msWait(2000)

        expect(page.url()).toMatch(LOCAL_URL + "/dashboard")
        await expectPuppeteer(page).toClick("#" + E2E_DASHBOARD_IDS.SETTINGS)
        await msWait(1000)
        expect(page.url()).toMatch(LOCAL_URL + "/dashboard/settings")
        await expectPuppeteer(page).toClick(
            "#" + E2E_DASHBOARD_IDS.SETTINGSPAYMENT
        )
        await msWait(1000)
        expect(page.url()).toMatch(LOCAL_URL + "/dashboard/settings/payment")

        await expectPuppeteer(page).toClick("#" + E2E_DASHBOARD_IDS.ADDPLAN)
        await msWait(2000)

        expect(page.url()).toMatch(
            LOCAL_URL + "/dashboard/settings/payment/plan"
        )

        await clickButton("Continue to Payment")
        await msWait(2000)

        expect(page.url()).toMatch(
            LOCAL_URL + "/dashboard/settings/payment/billing"
        )

        await page.waitForSelector("iframe")
        let elementHandle = await page.$("iframe")

        if (!elementHandle) {
            throw new Error("StripeElement iFrame not found")
        }

        let frame = await elementHandle.contentFrame()

        if (!frame) {
            throw new Error("StripeElement iFrame content not found")
        }

        await msWait(1000)
        await frame.click("input[name=cardnumber]")
        await msWait(1000)
        await page.keyboard.type("4242424242424242", { delay: 10 })
        await msWait(1000)
        await page.keyboard.type("424", { delay: 10 })
        await msWait(1000)
        await page.keyboard.type("424", { delay: 10 })
        await msWait(1000)

        await page.keyboard.type("02141", { delay: 10 })
        await msWait(1000)

        await clickButton("SAVE")

        await msWait(3000)

        await clickButton("Submit")
        await msWait(5000)

        expect(page.url()).toMatch(
            LOCAL_URL + "/dashboard/settings/payment/confirmation"
        )

        await clickButton("Back to Profile")
        await msWait(2000)
        await expectPuppeteer(page).toMatch("Starter")
        await msWait(1000)
        await expectPuppeteer(page).toClick("#" + E2E_DASHBOARD_IDS.CANCELPLAN)

        await clickButton("Continue")
        await msWait(2000)
        await clickButton("Back")
    })

    test("Logs in and edits card", async () => {
        await page.goto(LOCAL_URL)
        await expectPuppeteer(page).toClick("#" + E2E_HOMGEPAGE_IDS.SIGNIN)
        expect(page.url()).toMatch(LOCAL_URL + "/auth")

        await expectPuppeteer(page).toClick("#" + E2E_AUTH_IDS.LOGINSWITCH)
        await type(page, E2E_AUTH_IDS.EMAIL, verifiedUser.userID)
        await type(page, E2E_AUTH_IDS.PASSWORD, verifiedUser.password)

        await clickButton("Log In")
        await msWait(2000)

        expect(page.url()).toMatch(LOCAL_URL + "/dashboard")
        await expectPuppeteer(page).toClick("#" + E2E_DASHBOARD_IDS.SETTINGS)
        await msWait(1000)
        expect(page.url()).toMatch(LOCAL_URL + "/dashboard/settings")
        await expectPuppeteer(page).toClick(
            "#" + E2E_DASHBOARD_IDS.SETTINGSPAYMENT
        )
        await msWait(1000)
        expect(page.url()).toMatch(LOCAL_URL + "/dashboard/settings/payment")
    })
})
