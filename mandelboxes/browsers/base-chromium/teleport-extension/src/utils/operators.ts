const cyclingArray = <T>(limit: number, array: Array<T>) => {
  if (array.length > limit) array.splice(0, array.length - limit)
  const fluent = {
    add: (value: T) => {
      if (array.length >= limit) array.splice(0, array.length - limit + 1)
      array.push(value)
      return fluent
    },
    get: () => array,
  }
  return fluent
}

const trim = (value: number, lower: number, upper: number) => {
  if (value > upper) return upper
  if (value < lower) return lower
  return value
}

const sameSign = (a: number, b: number) => a * b > 0

export { cyclingArray, trim, sameSign }
