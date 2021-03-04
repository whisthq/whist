import { deepCopyObject } from "shared/utils/general/reducer"
import { cloneDeep } from "lodash"
import { backOff } from "exponential-backoff"

export const searchArrayByKey = (
    arr: Record<string, any>[],
    key: string,
    target: string | number | boolean
): {
    index: number
    value: Record<string, any> | null
} => {
    /*
        Description:
            Takes an array of JSON objects and searches for a JSON object with a matching key/value pair. Will return
            the first match that is found.

        Arguments:
            arr (Record<string, any>[]): Array of JSON objects
            key (string): The key to search for
            target (string | number | boolean): The desired value of the key

        Returns:
            map: { value: Record<string, any>, index: number }
    */

    for (let i = 0; i < arr.length; i += 1) {
        const value = arr[i]
        if (!Object.prototype.hasOwnProperty.call(value, key)) {
            throw ReferenceError
        }
        if (value[key] === target) {
            return { value: value, index: i }
        }
    }
    return { value: null, index: -1 }
}

export const updateArrayByKey = (
    arr: Record<string, any>[],
    key: string,
    target: string | number | boolean,
    body: Record<string, any>
) => {
    /*
        Description:
            Takes an array of JSON objects, searches for a JSON object with a matching key/value pair, and then
            replaces desired fields of that JSON object with new fields. For instance, if my array looks like
                [
                    {a: 1, b: 2}
                ]
            I can replace "b" with 3 where "a" equals 1.

        Arguments:
            arr (Record<string, any>[]): Array of JSON objects
            key (string): The key to search for
            target (string | number | boolean): The desired value of the key
            body: (Record<string, any>): New JSON object.
                NOTE: Does not need to contain all fields of JSON object, only the ones you want to replace.

        Returns:
            map: { array: Record<string, any>[], index: number }
    */

    const clonedArr: Record<string, any>[] = cloneDeep(arr)
    const { value, index } = searchArrayByKey(clonedArr, key, target)
    if (index !== -1 && value) {
        const valueCopy = deepCopyObject(value)
        clonedArr[index] = Object.assign(valueCopy, body)
        return { array: clonedArr, index: index }
    }
    return { array: clonedArr, index: -1 }
}

export const fractalBackoff = <T>(fn: () => Promise<T>) => {
    /*
        Description:
            Wrapper function that implements exponential backoff with jitter for any promise.
            This is good for promises like fetch(), where if it fails the first time,
            we retry a few more times (w/ exponential backoff + jitter) in case it works
            the second or third time around.

            Defaults to five max retries.

        Arguments:
            fn (() => Promise<T>): Function that returns a promise

        Returns:
            Promise<T>

        Usage:
            fractalBackoff(() => fetch(...))
    */

    return backOff(() => fn(), {
        jitter: "full",
        numOfAttempts: 5,
    })
}

export const generateToken = async () => {
    /*
        Generate a unique one-time use login token
 
        Arguments:
            none     
    */
    const crypto = require("crypto")

    const buffer: Buffer = await new Promise((resolve, reject) => {
        crypto.randomBytes(128, (err: Error, buf: Buffer) => {
            if (err) {
                reject(err)
            }
            resolve(buf)
        })
    })
    return buffer.toString("hex")
}
