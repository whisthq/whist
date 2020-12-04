import { deepCopyObject } from "shared/utils/reducerHelpers"
import { cloneDeep } from "lodash"

export const openExternal = (url: string) => {
    /*
    Description:
        Opens a link in a browser from Electron.

    Arguments:
        url (string): Link URL to open
    
    Returns:
        void
    */

    const { shell } = require("electron")
    shell.openExternal(url)
}

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

    try {
        let clonedArr: Record<string, any>[] = cloneDeep(arr)
        let { value, index } = searchArrayByKey(clonedArr, key, target)
        if (index !== -1 && value) {
            let valueCopy = deepCopyObject(value)
            ;(valueCopy = Object.assign(valueCopy, body)),
                (clonedArr[index] = valueCopy)
            return { array: clonedArr, index: index }
        } else {
            return { array: clonedArr, index: -1 }
        }
    } catch (err) {
        throw err
    }
}
