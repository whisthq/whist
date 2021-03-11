import puppeteer from "puppeteer"
import expectPuppeteer from "expect-puppeteer"
import { msWait } from "testing/utils/utils"
import {
    E2E_AUTH_IDS,
    E2E_DASHBOARD_IDS,
    E2E_HOMEPAGE_IDS,
    LOCAL_URL,
} from "testing/utils/testIDs"
import { verifiedUser, payingUser } from "testing/utils/testState"
import {
    generateUser,
    deleteInput,
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
    await insertUserDB(payingUser, hasuraToken)
    await insertUserInvite(verifiedUser, hasuraToken)
    await insertUserInvite(payingUser, hasuraToken)
})

afterAll(async () => {
    await deleteUserInvite(verifiedUser, hasuraToken)
    await deleteUserInvite(payingUser, hasuraToken)
    await deleteUserDB(verifiedUser, hasuraToken)
    await deleteUserDB(payingUser, hasuraToken)
})

describe("Dashboard Page", () => {
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

    test("Logs in and navigates side bar", async () => {
        await page.goto(LOCAL_URL)
        await expectPuppeteer(page).toClick("#" + E2E_HOMEPAGE_IDS.SIGNIN)
        expect(page.url()).toMatch(LOCAL_URL + "/auth")

        await expectPuppeteer(page).toClick("#" + E2E_AUTH_IDS.LOGINSWITCH)
        await type(page, E2E_AUTH_IDS.EMAIL, verifiedUser.userID)
        await type(page, E2E_AUTH_IDS.PASSWORD, verifiedUser.password)
        await msWait(2000)

        await clickButton("Log In")
        await msWait(3000)

        expect(page.url()).toMatch(LOCAL_URL + "/dashboard")
        await expectPuppeteer(page).toMatch("The first cloud-powered browser.")
        await msWait(1000)
        await expectPuppeteer(page).toClick("#" + E2E_DASHBOARD_IDS.SETTINGS)
        await msWait(1000)
        expect(page.url()).toMatch(LOCAL_URL + "/dashboard/settings")

        await expectPuppeteer(page).toClick("#" + E2E_DASHBOARD_IDS.HOME)
        await msWait(1000)
        expect(page.url()).toMatch(LOCAL_URL + "/dashboard")

        await expectPuppeteer(page).toClick("#" + E2E_DASHBOARD_IDS.SIGNOUT)
        await msWait(1000)
        await clickButton("Sign Out")
        await msWait(1000)
        expect(page.url()).toMatch(LOCAL_URL + "/auth")
    })

    test("Shows the correct UI to a paying user", async () => {
        await page.goto(LOCAL_URL)
        await expectPuppeteer(page).toClick("#" + E2E_HOMEPAGE_IDS.SIGNIN)
        expect(page.url()).toMatch(LOCAL_URL + "/auth")

        await expectPuppeteer(page).toClick("#" + E2E_AUTH_IDS.LOGINSWITCH)
        await type(page, E2E_AUTH_IDS.EMAIL, payingUser.userID)
        await type(page, E2E_AUTH_IDS.PASSWORD, payingUser.password)
        await msWait(2000)

        await clickButton("Log In")
        await msWait(3000)

        expect(page.url()).toMatch(LOCAL_URL + "/dashboard")

        await expectPuppeteer(page).toMatch("The first cloud-powered browser.")
    })

    test("Navigates to profile page and edits name", async () => {
        await page.goto(LOCAL_URL)
        await expectPuppeteer(page).toClick("#" + E2E_HOMEPAGE_IDS.SIGNIN)
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

        await expectPuppeteer(page).toClick("#" + E2E_DASHBOARD_IDS.EDITNAME)
        await msWait(1000)

        await deleteInput(page, E2E_DASHBOARD_IDS.NAMEFIELD)
        const { name } = generateUser()
        await type(page, E2E_DASHBOARD_IDS.NAMEFIELD, name)
        await clickButton("SAVE")
        await msWait(1000)
        await expectPuppeteer(page).toMatch(name)

        await expectPuppeteer(page).toClick("#" + E2E_DASHBOARD_IDS.EDITNAME)
        await msWait(1000)
        await clickButton("CANCEL")
        await expectPuppeteer(page).toMatch(name)
    })

    test("Navigates to profile page and edits password", async () => {
        await page.goto(LOCAL_URL)
        await expectPuppeteer(page).toClick("#" + E2E_HOMEPAGE_IDS.SIGNIN)
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
            "#" + E2E_DASHBOARD_IDS.EDITPASSWORD
        )

        await type(page, E2E_DASHBOARD_IDS.CURRENTPASSWORD, "Wavelf2020")
        await type(page, E2E_AUTH_IDS.PASSWORD, "Wavelf2020")
        await type(page, E2E_AUTH_IDS.CONFIRMPASSWORD, "Wavelf2020")

        await clickButton("SAVE")
    })
})
