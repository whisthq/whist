/* eslint-disable */
import {
    deep_copy,
    modified,
    if_exists_spread,
    if_exists_spread_f,
    if_exists_spread_nested,
    if_exists_spread_nested_f,
    if_exists_else,
} from "shared/utils/reducerHelpers"

describe("reducerHelpers", () => {
    // these are used mainly for state modification
    test("Testing deep_copy", () => {
        let obj = {
            a: {
                b: "c",
            },
            d: "e",
        }

        let copy = deep_copy(obj)

        obj.d = "f"

        expect(copy.d).toEqual("e")

        obj.a.b = "g"

        expect(copy.a.b).toEqual("c")
    })

    test("Testing modified", () => {
        // without deepcopy
        let _set: Set<string> = new Set()
        let new_set = modified(_set, _set.add, ["hello"])

        expect(_set.has("hello")).toEqual(true)
        expect(new_set.has("hello")).toEqual(true)

        new_set = modified(_set, "add", ["goodbye"])

        expect(_set.has("goodbye")).toEqual(true)
        expect(new_set.has("goodbye")).toEqual(true)

        // with deepcopy
        new_set = modified(_set, "add", ["hello again"], true)

        expect(_set.has("hello again")).toEqual(false)
        expect(new_set.has("hello again")).toEqual(true)

        let obj: any = {
            a: [0, 0, 0, 0],
            f(element: any) {
                if (this && this.a) {
                    this.a.push(element)
                }
            },
            g(elements: any[]) {
                for (let i = 0; i < elements.length; i++) {
                    this.f(elements[i])
                }
            },
        }

        let new_obj: any = modified(obj, "f", [4], true)

        expect(obj.a.length).toEqual(4)
        expect(new_obj.a.length).toEqual(5)

        for (let i = 0; i < 5; i++) {
            if (i < 4) {
                expect(obj.a[i]).toEqual(new_obj.a[i])
            } else {
                expect(new_obj.a[i]).toEqual(i)
            }
        }

        obj = new_obj
        new_obj = modified(obj, "g", [[5, 6]], true)

        for (let i = 0; i < 7; i++) {
            if (i < 5) {
                expect(obj.a[i]).toEqual(new_obj.a[i])
            } else {
                expect(new_obj.a[i]).toEqual(i)
            }
        }
    })

    test("Testing if_exists_spread", () => {
        // no deepcopy or copy
        let obj: any = {
            a: [
                null,
                {
                    b: "c",
                },
            ],
            d: "e",
        }

        let new_obj: any = if_exists_spread(obj, { d: "f", g: "h" }, false)

        expect(obj).toEqual(new_obj)
        expect(obj.d).toEqual("f")
        expect(obj.g).toEqual("h")

        // test with shallow copy

        let old_obj = obj

        new_obj = if_exists_spread(obj, { d: "e", i: "j" })

        expect(old_obj).toEqual(obj)
        expect(new_obj === obj).toEqual(false)
        expect(obj.i).toEqual(undefined)
        expect(obj.d).toEqual("f")

        expect(new_obj.d).toEqual("e")
        expect(new_obj.i).toEqual("j")

        // test with deep copy not meaningful at one level of recursion
    })

    test("Testing if_exists_spread_f", () => {
        // try first with no copy
        let obj: any = {
            a: 1,
            b: 2,
        }

        let f = (x: number) => x + 1

        let incrementor = { a: f, b: f }
        let new_obj: any = if_exists_spread_f(obj, incrementor, false)

        expect(obj.a).toEqual(2)
        expect(obj.b).toEqual(3)
        expect(new_obj).toEqual(obj)

        // shallow copy

        let old_obj: any = obj
        new_obj = if_exists_spread_f(obj, incrementor)

        expect(new_obj === obj).toEqual(false)
        expect(obj).toEqual(old_obj)
        expect(new_obj.a).toEqual(3)
        expect(new_obj.b).toEqual(4)

        // as before deep copy is not meaningful without recursion
    })

    test("Testing if_exists_spread_nested", () => {
        let obj: any = {
            x: {
                y: ["z1", "z2"],
            },
            w: "t",
        }

        let to_merge: any = {
            x: {
                n: ["m1", "m2"],
            },
        }

        let merged: any = if_exists_spread_nested(obj, to_merge)
        let merged_x_expected: any = {
            y: ["z1", "z2"],
            n: ["m1", "m2"],
        }

        expect(merged.x).toEqual(merged_x_expected)
        expect(merged.w).toEqual("t")
        expect(Object.keys(merged).length).toEqual(2)

        obj = {
            x: {
                y: {
                    z: "a",
                },
            },
        }

        to_merge = {
            x: {
                y: {
                    w: "w",
                },
                f: {
                    t: "a",
                },
            },
        }

        merged = if_exists_spread_nested(obj, to_merge)
        merged_x_expected = {
            y: {
                w: "w",
                z: "a",
            },
            f: {
                t: "a",
            },
        }

        let merged_xf_expected: any = {
            t: "a",
        }

        expect(merged.x).toEqual(merged_x_expected)
        expect(merged.x.f).toEqual(merged_xf_expected)
        expect(Object.keys(merged).length).toEqual(1)
    })

    test("Testing if_exists_spread_nested_f", () => {
        let obj: any = {
            x: {
                y: [1, "not null"],
            },
            w: new Set(),
        }

        let to_merge_f: any = {
            x: {
                y: (arr: any[]) =>
                    arr.map((element: any) =>
                        typeof element === "number" ? element + 1 : null
                    ),
            },
            w: (_set: Set<any>) => modified(_set, "add", ["hello world"], true),
            r: (old_r: any) =>
                typeof old_r === "number" ? old_r * old_r : "not a number",
        }

        let merged = if_exists_spread_nested_f(obj, to_merge_f, "hey there")

        expect(merged.x.y[0]).toEqual(2)
        expect(merged.x.y[1]).toEqual(null)
        expect(merged.w.has("hello world")).toEqual(true)
        expect(merged.w.size).toEqual(1)
        expect(merged.r).toEqual("not a number")

        // test the default parameter passed

        to_merge_f = {
            n: (z: any | undefined) =>
                typeof z === "undefined" ? null : "not null",
        }

        merged = if_exists_spread_nested_f(obj, to_merge_f)

        expect(merged.n).toEqual(null)
    })

    // these are meant usually for state accessing
    test("Testing if_exists_else", () => {
        let obj: any = {
            a: {
                b: {
                    c: ["a", "b", "c"],
                },
            },
            d: "d",
            e: {
                f: ["g, h"],
            },
        }

        let query_abc: any[] = ["a", "b", "c"]
        let abc: any[] | null = if_exists_else(obj, query_abc)

        expect(abc).toEqual(query_abc)

        let query_bad: any[] = ["e", "f", "g"]
        let badpath: any[] | null = if_exists_else(obj, query_bad)

        expect(badpath).toEqual(null)

        let query_bad2: any = ["h"]
        let badpath2: any[] | string = if_exists_else(obj, query_bad2, "hello")

        expect(badpath2).toEqual("hello")
    })
})
