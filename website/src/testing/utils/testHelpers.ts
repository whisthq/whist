import puppeteer from "puppeteer"

import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT as authDefaultState } from "store/reducers/auth/default"
import { DEFAULT as dashboardDefaultState } from "store/reducers/dashboard/default"
import { LOCAL_URL } from "testing/utils/testIDs"
import { msWait } from "testing/utils/utils"
import { TestUser } from "testing/utils/testState"
import { signupEmail, deleteAccount } from "shared/api/index"
import { graphQLPost } from "shared/api/index"
import {
    UPDATE_USER,
    DELETE_USER,
    INSERT_INVITE,
    DELETE_INVITE,
} from "testing/constants/graphql"
import * as dotenv from "dotenv"

export const rootState: { [key: string]: Object } = {
    DashboardReducer: dashboardDefaultState,
    AuthReducer: authDefaultState,
}

// TODO (future implementers) change singleAssign to be more intelligent by looking at
// shared/utils/reducerHelpers and taking those merge functions and using them here

/**
 * Given a base object basically merge objs into it. This is meant to be in conjunction with a known
 * key and one of the states below used to mock the store.
 * @param objs the object to merge into it
 * @param base the base to merge into, usually the default state in the store.
 */
export const singleAssign = (
    objs: Object,
    base: Object = deepCopy(rootState)
) => {
    // objs should be
    // {
    // key: { this is an object that I'll assign the root state [key] to }
    // }
    // this will not work with recursive sort of merging
    // look at reducerHelpers for that
    return Object.assign(base, objs)
}

/**
 * Execute singleAssign (above) multiple times, basically inserting various different mock pieces
 * of the state into a base redux state. Useful when a component depends on various different elements
 * from the state.
 * @param objs the objects to stich into the default state.
 * @param base the base state to merge into.
 */
export const chainAssign = (
    objs: Object[],
    base: Object = deepCopy(rootState)
) => {
    let previousBase = base
    objs.forEach((obj: Object) => {
        previousBase = singleAssign(obj, previousBase)
    })
    return previousBase
}

/**
 * Generates a new user with a fractal.co email with a new random password
 */
export const generateUser = () => {
    let emailFirst = Math.random().toString(36).substring(7)
    let emailSecond = Math.random().toString(36).substring(7)
    const email = "test-user+" + emailFirst + "@fractal.co"

    let numbers = Math.random().toString(10).substring(8)
    const password = numbers + emailSecond + "A"

    const name = Math.random().toString(36).substring(7)

    return { email, password, name }
}

/**
 * Deletes content from an input field with given id
 * @param id id of the input field
 */
export const deleteInput = async (page: puppeteer.Page, id: string) => {
    let inputValue = await page.$eval("#" + id, (el) => {
        if (el) {
            const htmlelem = el as HTMLElement
            const input = htmlelem as HTMLInputElement
            return input.value
        }
    })
    await page.focus("#" + id)
    if (inputValue) {
        for (let i = 0; i < inputValue.length; i++) {
            await page.keyboard.press("Backspace")
        }
    }
}
/**
 * Simulates typing into an input field with a given id
 * @param id id of input field
 * @param content content ot type
 */
export const type = async (page: puppeteer.Page, id: string, content: string) =>
    await page.type("#" + id, content)

export const launchURL = async () => {
    let headless = true
    if (process.env.HEADLESS) {
        headless = process.env.HEADLESS === "true"
    }

    const browser = await puppeteer.launch({
        headless: headless,
        args: [
            "incognito",
            "--disable-web-security",
            "--disable-features=IsolateOrigins,site-per-process",
        ],
    })
    const context = await browser.createIncognitoBrowserContext()

    const page = await context.newPage()
    page.setDefaultNavigationTimeout(0)
    await msWait(2000)
    await page.goto(LOCAL_URL)

    return { testBrowser: browser, testPage: page }
}

export const loadHasuraToken = () => {
    /*
    Description:
        Fetch the admin Hasura from environment (for CI)

    Arguments:
        none

    Returns:
        string: Hasura token
*/
    const env = dotenv.config()
    let hasuraToken = env.parsed ? (env.parsed.HASURA_ADMIN_KEY as string) : ""
    if (!hasuraToken) {
        hasuraToken = process.env.HASURA_ADMIN_KEY
            ? process.env.HASURA_ADMIN_KEY
            : ""
    }
    return hasuraToken
}

export const insertUserDB = async (user: TestUser, hasuraToken: string) => {
    /*
    Description:
        Insert a test user into the database

    Arguments:
        user (TestUser): TestUser object
        hasuraToken (string): Hasura admin token
*/
    if (!hasuraToken) {
        hasuraToken = loadHasuraToken()
    }

    await signupEmail(user.userID, user.password, user.name, user.feedback)

    await graphQLPost(
        UPDATE_USER,
        "UpdateUser",
        {
            userID: user.userID,
            stripeCustomerID: user.stripeCustomerID,
            verified: user.verified,
        },
        hasuraToken,
        true
    )
}

export const deleteUserDB = async (user: TestUser, hasuraToken: string) => {
    /*
    Description:
        Delete a test user from the database

    Arguments:
        user (TestUser): TestUser object
        hasuraToken (string): Hasura admin token
*/
    if (!hasuraToken) {
        hasuraToken = loadHasuraToken()
    }

    await graphQLPost(
        DELETE_USER,
        "DeleteUser",
        {
            userID: user.userID,
        },
        hasuraToken,
        true
    )
}

export const insertUserInvite = async (user: TestUser, hasuraToken: string) => {
    /*
    Description:
        Creates an invite for a user so they can access the dashboard

    Arguments:
        user (TestUser): TestUser object
        hasuraToken (string): Hasura admin token
*/
    if (!hasuraToken) {
        hasuraToken = loadHasuraToken()
    }

    const output = await graphQLPost(
        INSERT_INVITE,
        "InsertInvite",
        {
            userID: user.userID,
            typeformSubmitted: user.typeformSubmitted,
            accessGranted: user.accessGranted,
        },
        hasuraToken,
        true
    )
}

export const deleteUserInvite = async (user: TestUser, hasuraToken: string) => {
    /*
    Description:
        Deletes a test user's invite

    Arguments:
        user (TestUser): TestUser object
        hasuraToken (string): Hasura admin token
*/
    if (!hasuraToken) {
        hasuraToken = loadHasuraToken()
    }

    await graphQLPost(
        DELETE_INVITE,
        "DeleteInvite",
        {
            userID: user.userID,
        },
        hasuraToken,
        true
    )
}
