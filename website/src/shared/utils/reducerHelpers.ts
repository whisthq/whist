// may wanna replace with rfdc - really fast deep copy
var _ = require("lodash") // alternatively use a recursive ... function

// deep copy an object so that there is no aliasing
export const deep_copy = (obj: any) => _.cloneDeep(obj)

// basically will return obj after obj.prop_func(prop_func_args)
// i.e. modified(a_set, add, element) = a_set after a_set.add(element)
// this way you can do a_set = modified(a_set, add, element) as if you could do a_set = a_set.added(element)

// I encourage you to just use a closure or whatever those are called (delete the params)
//if you deep copy you'll need to pass the name of the property
export const modified = (
    obj: any,
    prop_func: any,
    prop_func_args: any[] = [],
    deepcopy = false
) => {
    const y = deepcopy ? deep_copy(obj) : obj
    if (prop_func_args.length > 0) {
        if (!deepcopy) {
            if (typeof prop_func === "string") {
                y[prop_func].apply(y, prop_func_args)
            } else {
                prop_func.apply(y, prop_func_args)
            }
        } else {
            if (typeof prop_func === "string") {
                y[prop_func].apply(y, prop_func_args)
            } else {
                throw "For deep copy type must be string."
            }
        }
    } else {
        y.prop_func()
    }

    return y
}

// key values should be { k : v }
// this is meant to avoid rewriting stuff like
// new_obj : obj ? {
// ...obj,
// key: value,
// } : {
//  key: value,
// }
export const if_exists_spread = (
    obj: any,
    key_values: any,
    copy: boolean = true,
    deepcopy: boolean = false
) => {
    let key_valuefs = deep_copy(key_values)
    for (const key in key_values) {
        key_valuefs[key] = (obj: any) => key_values[key]
    }

    return if_exists_spread_f(obj, key_valuefs, copy, deepcopy)
}

// same idea as above but will apply the value_f of {key : value_f} to each of the functions
// please put your functions in closures if there are any more params than just the key
// the valuefs should take in the previous object as their value, otherwise use a closure
export const if_exists_spread_f = (
    obj: any,
    key_valuefs: any,
    copy: boolean = true,
    deepcopy: boolean = false
) => {
    const new_obj =
        deepcopy && obj
            ? deep_copy(obj)
            : copy && obj
            ? { ...obj }
            : obj
            ? obj
            : {}

    if (obj) {
        for (const key in key_valuefs) {
            new_obj[key] = key_valuefs[key](
                key in new_obj ? new_obj[key] : null
            )
        }
    }

    return new_obj
}

// this is whether we should just paste in the object, or recurse further instead (!base)
const is_basic_type = (obj: any) =>
    typeof obj === "string" ||
    typeof obj === "number" ||
    typeof obj === "boolean" ||
    typeof obj === "bigint" ||
    typeof obj === "symbol" ||
    typeof obj === "function"
const is_base = (obj: any) =>
    Array.isArray(obj) || obj instanceof Set || is_basic_type(obj)

// DOES NOT WORK IF YOU WANT TO STORE FUNCTIONS
// IF YOU HAVE COMPOSED FUNCTIONS IT WILL ONLY EVALUATE AT THE FIRST LEVEL
const merge_f = (
    new_obj: any,
    new_key_values: any,
    otherwise_in: any | undefined = undefined
) => {
    for (const key in new_key_values) {
        if (!(key in new_obj) && typeof new_key_values[key] != "function") {
            // if it's not a function we need to create recursivelly
            new_obj[key] = {}
        }

        if (key in new_obj && !is_base(new_obj[key])) {
            // if it exists we need to search
            merge_f(new_obj[key], new_key_values[key])
        } else {
            // otherwise it's a base value
            const value = new_key_values[key](
                key in new_obj ? new_obj[key] : otherwise_in
            )
            new_obj[key] = value
        }
    }
    return new_obj // this is merely here to be helpful on the final return
}

const nested_values_to_f = (new_key_values: any) => {
    for (const key in new_key_values) {
        if (is_base(new_key_values[key])) {
            const value = new_key_values[key] // necessary because arrow functions lazy??
            new_key_values[key] = (old_obj: any) => value
        } else {
            nested_values_to_f(new_key_values[key])
        }
    }
    return new_key_values
}

// this handles the nested case of if_exists_spread
// where key_values mimicks the body of the object we want to set and at every level where
// there exists then it will create otherwise it will spread and add
// both key_values and obj can be arbitrarily nested objects
//
// {x : {y : z}, w: t}, {x : {n : [m1, m2]}}
// => {x : {y: z, m: [m1, m2]}, w: t}
//
// this is necessarily a deep copy
// basically a help
export const if_exists_spread_nested = (obj: any, key_values: any) =>
    merge_f(deep_copy(obj), nested_values_to_f(deep_copy(key_values)))

// same idea but with functions
// by undefined we are basically setting it to "not existing"
export const if_exists_spread_nested_f = (
    obj: any,
    key_valuefs: any,
    otherwise_in: any | undefined = undefined
) => merge_f(deep_copy(obj), deep_copy(key_valuefs), otherwise_in)

// below here are functions that you'll want to use in every day-life
// i.e. in the mapStateToProps function

// if obj has otherwise return obj[indices[0]][indices[1]] etc... else otherwise
export const if_exists_else = (
    obj: any,
    indices: any[],
    otherwise: any = null
) => {
    let search_obj: any = obj
    for (const i in indices) {
        let index = indices[i]
        if (index in search_obj) {
            search_obj = search_obj[index]
        } else {
            return otherwise
        }
    }

    return search_obj ? search_obj : otherwise
}
