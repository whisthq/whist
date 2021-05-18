import { mapValues, forEach, mergeWith, merge, values, flatten } from "lodash"

export const nestForEach = <
  T,
  C extends { [key: string]: T },
  P extends { [key: string]: C }
>(
  obj: P,
  fn: (value: T, path: [keyof P, keyof C]) => any
) =>
  forEach(obj, (parent, parentKey) =>
    forEach(parent, (child, childKey) => {
      fn(child, [parentKey, childKey])
    })
  )

export const nestMapValues = <
  T,
  C extends { [key: string]: T },
  P extends { [key: string]: C }
>(
  obj: P,
  fn: (value: T, path: [keyof P, keyof C]) => any
) =>
  mapValues(obj, (parent, parentKey) =>
    mapValues(parent, (child, childKey) => fn(child, [parentKey, childKey]))
  )

export const nestValues = <
  T,
  C extends { [key: string]: T },
  P extends { [key: string]: C }
>(
  obj: P
) => flatten(values(mapValues(obj, (parent) => values(parent))))

export const nestMerge = <A, B>(objA: object, objB: object) =>
  mergeWith({ ...objA }, objB, (a: A, b: B) => merge(a, b))
