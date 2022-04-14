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

export { cyclingArray }
