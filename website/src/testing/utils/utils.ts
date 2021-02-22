import { is_basic_type } from "shared/utils/reducerHelpers"
import fs from "fs"

// read notion for more information regarding this
// these are used to clean up our snapshots so that they will be deterministic basically
const PRUNE_STRING = "PRUNED FOR NONDETERMINISM"
const RISKY_PROPS = ["aria-describedby", "src"]

/**
 * Get the child node of an element and throw an error if there are > 1 children and
 * you request for it to be checked with checks (param). Otherwise return the first child.
 * @param node the (hopefully) singleton node to get the child for.
 * @param checks whether to check if there are more than one children.
 *
 * Returns null for no children.
 */
export const child = (node: { childNodes: any }, checks: boolean = true) => {
    // this is meant to help you get children of singleton elements that you might get
    // for example by screen.getByTestId('something') you'll get a
    // <div data-testid={'something'}>
    //  <fancy component that I want>
    // </div>
    // but you actually want the child inside (fancy component that you want)
    // sometimes these are not actually singletons but usually they are
    if (checks && node.childNodes.length > 1) {
        throw new Error("Too many children")
    } else if (node.childNodes.length >= 1) {
        node.childNodes[0]
    } else {
        return null
    }
}

/**
 * Wait for a given number of seconds. Potentially useful for when tests need to wait for something to
 * complete and we need a quick hack to make them work for now. Used for zeroMsWait which has a
 * more tangible use.
 * @param ms the number of milliseconds to wait for.
 */
export const msWait = async (ms: number) =>
    await new Promise((resolve) => setTimeout(resolve, ms))

/**
 * Wait for 0 seconds. Necessary for GraphQL mocks to go from loading to having data. This is
 * an intentional behavior from ApolloClient, it's used to allow us to test behavior easily when
 * "loading".
 */
export const zeroMsWait = async () => msWait(0)

/**
 * Remove all non-determinism that is known from a render(...) snapshot. Currently removes determinism from
 * history and the html in baseElement and container. These are the ones that we currently know of, though
 * there may be more in the future.
 * @param tree the render tree (the full one).
 * @param risky whether to manipulate the DOM that react manipulates. Doing so can cause react to throw
 * horrendous errors that are unreadable and ruin your testing experience. For that we prefer custom
 * snapshots for now. (Or just move to enzyme and shallow render).
 */
export const removeNondeterminism = (
    tree: {
        history: any
        baseElement: any
        container: any
    },
    risky: boolean = false
) => {
    if (tree && tree.history) {
        if (tree.history) {
            const badKeys = ["key", "pathname", "action"]
            badKeys.forEach((key: string) =>
                removeAll(tree.history, key, PRUNE_STRING)
            )
        }

        // risky because sometimes throws an unhelpful error because "dom changed by not react oh no!"
        const elements: string[] = [tree.baseElement, tree.container]
        const badProps: string[] = risky ? ["aria-describedby", "src"] : [] // "src=ming.svg" for example
        elements.forEach((element: any) => {
            if (element) {
                badProps.forEach((prop: string) =>
                    setPropertyAll(element, prop, PRUNE_STRING)
                )
            }
        })
    }
    return tree
}

/**
 * Return whether an object is of a basic type (primitive). Functions count.
 * @param obj the object to check.
 */
const isBase = (obj: any) => is_basic_type(obj) || isFunction(obj)

/**
 * Checks whether when a object is console.logged it will print the given string. Thise
 * is used to check types of fancy elements that are not defined in an obvious place (for example
 * check if something is a function, or an HTMLButtonElement).
 * @param obj the object we want to simluate console.log for.
 * @param str the actual string we want to check for equality.
 */
const printStringIs = (obj: any, str: string) => {
    return obj && {}.toString.call(obj) === str
}

/**
 * Check if an object is a function.
 * @param functionToCheck the object to check for whether it's a function.
 */
const isFunction = (functionToCheck: any) => {
    return printStringIs(functionToCheck, "[object Function]")
}
/**
 * Check if an element (object) is an HTMLButtonElement
 * @param elementToCheck object to check.
 */
const isHTMLButtonElement = (elementToCheck: any) => {
    return printStringIs(elementToCheck, "[object HTMLButtonElement]")
}

/**
 * Removes the values of a dictionary recursively where the key is a string and has the value given
 * by removeStr. This is primarily used to remove problematic (non-deterministic, usually due to hashing)
 * elements in history in the render(...) tree to enable snapshots to equate.
 * @param tree the render(...) tree's subtree (or just a dictionary in general) to remove from (insert dummy).
 * @param removeStr the key that is bad and we want out.
 * @param magicString the dummy to replace that with. You can also put undefined to effectively "remove"
 * it. We prefer not to actually remove it since that can make mistakes harder to find.
 */
const removeAll = (
    tree: any,
    removeStr: string,
    magicString: string | undefined = ""
) => {
    Object.keys(tree).forEach((key: any) => {
        if (isBase(tree[key])) {
            if (key == removeStr && typeof tree[key] == "string") {
                tree[key] = magicString
            }
        } else if (tree[key]) {
            removeAll(tree[key], removeStr, magicString)
        }
    })
}

/**
 * Recursively traverse the DOM tree generated by render(...) and change the innerHTML to replace prop
 * values for certain non-deterministic properties with dummies that are static. This is to make
 * snapshots pass. Currently, we avoid it since react gets angry when you modify the DOM outside react.
 * @param tree the render(...) tree that we use to
 * @param property the property that is non-deterministic that we want out.
 * @param magicString the dummy string to replace the prop with.
 */
const setPropertyAll = (
    tree: any,
    property: string,
    magicString: string = ""
) => {
    const html: string = tree.innerHTML
    if (html && html.includes(property)) {
        tree.innerHTML = removeRiskyProp(tree.innerHTML, property, magicString)
    }

    const children = tree.childNodes
    if (children && children.length > 0) {
        for (let i = 0; i < children.length; ++i) {
            const child = children[i]
            setPropertyAll(child, property, magicString)
        }
    }
}

/**
 * A most rudimentary basic snapshot function that simply does string equals for the baseElement
 * and container objects inside the render(...) tree. Stores files and if they exist compares with them.
 * Throws an error if they are not equal.
 * @param tree the render tree generated by react-testing-library.
 * @param customSnapshotPath the path to store this file to. Not yet automatic.
 * @param apply The function that we might want to use to modify the string to remove all properties
 * (or other elements) which may be non-deterministic. Right now, we simply pass the identity function
 * as a default but the recommended usage is with the removeRiskyProps function (below).
 */
export const customSnapshot = async (
    tree: any,
    customSnapshotPath: string,
    apply: (html: string) => string = (str: string) => str
) => {
    const basePath = customSnapshotPath + "-baseElement-snap"
    const containerPath = customSnapshotPath + "-container-snap"

    const htmlBase = tree.baseElement.innerHTML
    const htmlContainer = tree.container.innerHTML

    const appliedHTMLBase = apply(htmlBase)
    const appliedHTMLContainer = apply(htmlContainer)

    const checkExists = (path: string, writeData: string) => {
        return (err: any, data: string) => {
            if (!err) {
                if (data !== writeData) {
                    throw new Error(`${path} had unequal data`)
                }
            }
        }
    }

    if (fs.existsSync(basePath)) {
        fs.readFile(basePath, "utf-8", checkExists(basePath, appliedHTMLBase))
    } else {
        fs.writeFile(basePath, appliedHTMLBase, "utf-8", (err: any) => {
            // throw new Error(`err on base path write: ${err}`)
        })
    }

    if (fs.existsSync(containerPath)) {
        fs.readFile(
            containerPath,
            "utf-8",
            checkExists(containerPath, appliedHTMLContainer)
        )
    } else {
        fs.writeFile(
            containerPath,
            appliedHTMLContainer,
            "utf-8",
            (err: any) => {
                // throw new Error(`err on base path write: ${err}`)
            }
        )
    }
}

/**
 * Removes various risky props (props that are non-deterministic or might be) as in removeRiskyProp
 * (below) but for a list of props.
 * @param html the string (html) from which to remove these props (and replace with dummy values).
 * @param props the props to remove.
 */
export const removeRiskyProps = (
    html: string,
    props: string[] = RISKY_PROPS
) => {
    let newHTML: string = html
    props.forEach((property: string) => {
        newHTML = removeRiskyProp(newHTML, property)
    })

    return newHTML
}

/**
 * This function takes a string of html and removes a property by searching for property="something"
 * and replacing that something with the magicString.
 * @param html the string to replace form.
 * @param property the property which is non-deterministic for which we want to take away the values
 * or make them static.
 * @param magicString The string we use as a dummy to replace them.
 */
export const removeRiskyProp = (
    html: string,
    property: string,
    magicString: string = PRUNE_STRING
) => {
    if (html && html.includes(property)) {
        let newHTML: string[] = []
        let start = 0
        let propIndex = html.indexOf(property, start)

        while (propIndex > -1) {
            // property="remove me!"
            const removeStart = propIndex + property.length + 2 // ="[.]..
            const removeEnd = html.indexOf('"', removeStart) // ...["]
            newHTML.push(html.substring(start, removeStart) + magicString)

            start = removeEnd
            propIndex = html.indexOf(property, start)
        }
        newHTML.push(html.substring(start, html.length))

        return newHTML.join("")
    } else {
        return html
    }
}
