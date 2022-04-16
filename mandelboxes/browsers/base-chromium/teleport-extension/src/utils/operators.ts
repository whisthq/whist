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
  if (lower < 0 || upper < 0)
    throw new Error("Lower/upper bounds must be positive")
  if (lower >= upper)
    throw new Error("Lower bound must be less than upper bound")

  let trimmed = Math.min(Math.abs(value), upper)
  trimmed = Math.max(trimmed, lower)
  return value < 0 ? -1 * trimmed : trimmed
}

const sameSign = (a: number, b: number) => a * b > 0

export { cyclingArray, trim, sameSign }
