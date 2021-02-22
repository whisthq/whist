import puppeteer from "puppeteer"
import expectPuppeteer from "expect-puppeteer"
import { msWait } from "testing/utils/utils"
import { LOCAL_URL } from "testing/utils/testIDs"
import { launchURL } from "testing/utils/testHelpers"

let browser: puppeteer.Browser
let page: puppeteer.Page

beforeAll(async () => {
    const { testBrowser, testPage } = await launchURL()
    browser = testBrowser
    page = testPage
})
afterAll(() => {
    browser.close()
})

jest.setTimeout(120000)

describe("Shared components", () => {
    test("Renders header", async () => {
        await expectPuppeteer(page).toMatch("About")
        await expectPuppeteer(page).toMatch("Support")
        await expectPuppeteer(page).toMatch("Careers")
    })

    test("Renders footer", async () => {
        await msWait(6000)
        // tried putting these in a loop but it always returned unhandled promise rejection errors
        await expectPuppeteer(page).toMatch("RESOURCES")
        await expectPuppeteer(page).toMatch("CONTACT")
        await expectPuppeteer(page).toMatch("Sales")
        await expectPuppeteer(page).toMatch("Support")
        await expectPuppeteer(page).toMatch("Careers")
        await expectPuppeteer(page).toMatch("Blog")
        await expectPuppeteer(page).toMatch("Join our Discord")

        await expectPuppeteer(page).toMatchElement("#linkedin")
        await expectPuppeteer(page).toMatchElement("#twitter")
        await expectPuppeteer(page).toMatchElement("#medium")
        await expectPuppeteer(page).toMatchElement("#instagram")
    })
})

describe("about page", () => {
    test("renders about page correctly", async () => {
        await page.goto(LOCAL_URL + "/about")
        await expect(page).toMatch("How It Works")
        await expect(page).toMatch("Our Stories")
        await expect(page).toMatch("Our Investors")
    })
})

describe("navigate homepage", () => {
    test("go to login and about", async () => {
        await page.goto(LOCAL_URL)
        // goes to login page
        await expect(page).toClick("button", { text: "GET STARTED" })
        await expect(page.url()).toMatch(LOCAL_URL + "/auth")

        const about = await page.$("#about")
        await about?.click()
        await msWait(500)
        await expect(page.url()).toMatch(LOCAL_URL + "/about")
    })

    test("footer links", async () => {
        await page.goto(LOCAL_URL)
        /* social links can be unreliable as some may ask to login
    first but here's how to do it:
    const [instagram] = await Promise.all([
      new Promise(resolve => page.once('popup', resolve)),
      page.click('#instagram')
    ])

    await msWait(4000)
    await expect(instagram.url()).toMatch("https://www.instagram.com/tryfractal")
    */
        // terms of service
        await expect(page).toClick("#tosPage")
        await expect(page.url()).toMatch(LOCAL_URL + "/termsofservice#top")

        await page.goto(LOCAL_URL)

        // privacy page
        await expect(page).toClick("#privacyPage")
        await expect(page.url()).toMatch(LOCAL_URL + "/privacy#top")
    })
})
