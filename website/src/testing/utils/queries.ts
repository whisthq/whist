import puppeteer from "puppeteer"
// Puppeteer queries return ElementHandle objects. These have a .click()
// method that can be used to perform a click. However, there are problems with
// performing clicks with this method. It's unreliable on complex targets, like
// modal windows or targets with animation.
//
// Because of this inconsistency, we perform click events using Puppeteer's
// 'evaluate' method, which actually executes a function within the browser.
// 'evaluate' is a little more complex to use, because the function passed
// to it must be serializable. Fortunately, 'evaluate' accepts a ElementHandle
// as an argument. We can call a ElementHandle like this:
// page.evaluate((el) => el.click(), element)

const formatElementError = (
    url: string,
    error: Error,
    thrower: Function,
    func: Function,
    message?: string
) => {
    const helpMessage = message || ""
    const funcMessage = func.name
        ? "Couldn't find element: " + func.name + ". "
        : ""
    const pageMessage = "Thrown on page: " + url

    error.message =
        helpMessage +
        "\n" +
        funcMessage +
        "\n" +
        pageMessage +
        "\n" +
        error.message

    Error.captureStackTrace(error, thrower)
    throw error
}

export const sleep = (ms: number) =>
    new Promise((resolve) => setTimeout(resolve, ms))

type ElementTest = <T extends () => Promise<any>>(
    page: puppeteer.Page,
    func: T,
    thrower?: Function,
    msg?: string
) => Promise<ReturnType<T> | never>

type ElementAction = <T extends () => Promise<any>>(
    page: puppeteer.Page,
    func: T,
    msg?: string
) => Promise<void | never>

export const expectElement: ElementTest = async (
    page,
    func,
    thrower,
    message
) => {
    const element = await func()
    try {
        expect(element).toBeTruthy()
    } catch (error) {
        throw formatElementError(
            page.url(),
            error,
            thrower || expectElement,
            func,
            message
        )
    }
    return element!
}

export const waitForElement: ElementTest = async (
    page,
    func,
    thrower,
    message
) => {
    let start = Date.now()
    let tick
    while (true) {
        tick = Date.now()
        if (await func()) return await func()
        try {
            if (tick - start > 10000) expect(await func()).toBeTruthy()
        } catch (error) {
            throw formatElementError(
                page.url(),
                error,
                thrower || waitForElement,
                func,
                message
            )
        }
        await sleep(100)
    }
}

export const maybeClick: ElementAction = async (page, func) => {
    const element = await func()
    if (element) page.evaluate((el) => el.click(), element)
}

export const expectClick: ElementAction = async (page, func, message) => {
    const element = await expectElement(page, func, expectClick, message)
    if (element) page.evaluate((el) => el.click(), element)
}

export const expectNavigate: ElementAction = async (page, func, message) => {
    await waitForElement(page, func, expectNavigate, message)
}

export const maybeClickNavigate: ElementAction = async (page, func) => {
    const element = await func()
    if (element)
        await Promise.all([
            page.evaluate((el) => el.click(), element),
            page.waitForNavigation({ waitUntil: "domcontentloaded" }),
            page.waitForNavigation({ waitUntil: "networkidle0" }),
        ])
}

export const expectClickNavigate: ElementAction = async (
    page,
    func,
    message
) => {
    const element = await expectElement(
        page,
        func,
        expectClickNavigate,
        message
    )
    await Promise.all([
        page.evaluate((el) => el.click(), element),
        page.waitForNavigation({ waitUntil: "domcontentloaded" }),
        page.waitForNavigation({ waitUntil: "networkidle0" }),
    ])
}
