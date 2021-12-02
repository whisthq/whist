/*
 * Returns a function that extras the element of an array
 * at the given index.
 *
 * @param index - a number
 * @returns a function to select the element by index
 */
const index = (idx: number) => {
  return function getIndex(arr: any[]) {
    return arr[idx]
  }
}

/*
 * A utility functon to get the length of an array.
 *
 * @param arr - arr to measure
 * @returns a number, the length of the array
 */
const lengths = (arr: any[]) => arr.length

/*
 * A utility function to alternate items from arrays.
 *
 * Takes any number of arrays a, b, c, ... and returns an array
 * which contains each of their arguments in alternated order, such
 * as [a1, b1, c1, ..., a2, b2, c2,...].
 *
 * @param { ...arrays } - arrays to interleave
 * @returns an array with the interleaved elements
 */
const interleave = (...arrays: any[]) => {
  let length = Math.min.apply(Math, arrays.map(lengths))
  let result: Array<any> = []

  for (var idx = 0; idx < length; ++idx) {
    result = result.concat(arrays.map(index(idx)))
  }

  return result
}

/*
 * A logging utility for decorated function pipelines.
 *
 * Can be used exactly like decorate, and will log to the console
 * the result at each step.
 *
 * The first log is always the initial arguments, and the last log
 * is the final result.
 *
 * @param func - a function to be wrapped by decorators
 * @param { ...decs } - decorators to wrap base function
 * @returns a function that pipelines func and { ...decs }
 */
const logDecorator = (
  logPrefix: (...args: any[]) => any,
  inputFn: (...args: any[]) => any,
  outputFn: (...args: any[]) => any
) => {
  return async (fn: (...args: any[]) => any, ...args: any[]) => {
    console.log(logPrefix(...args), "in: ", inputFn(...args))
    const response = await fn(...args)
    console.log(logPrefix(response), "out:", outputFn(response))
    return response
  }
}

/*
 * A logging utility for decorated function pipelines.
 *
 * Can be used exactly like decorate, and will log to the console
 * the result at each step.
 *
 * The first log is always the initial arguments, and the last log
 * is the final result.
 *
 * @param func - a function to be wrapped by decorators
 * @param { ...decs } - decorators to wrap base function
 * @returns a function that pipelines func and { ...decs }
 */
export const decorateDebug = (
  fn: (...args: any[]) => any,
  ...decs: ((...args: any[]) => any)[]
) => {
  let counter = 0
  const prefix = () => {
    const msg = "log " + counter
    counter++
    return msg
  }
  const inputFn = (x: any) => x
  const outputFn = (x: any) => x

  const initDbg = logDecorator(prefix, inputFn, outputFn)
  const decsDbg = logDecorator(prefix, inputFn, outputFn)
  interleave([], [])
  return decorate(
    fn,
    initDbg,
    ...interleave(
      decs,
      decs.map(() => decsDbg)
    )
  )
}

/*
 * Composes higher-order decorator functions onto a base function.
 *
 * The resulting function pipelines the decorators in the order they
 * are passed, with the base function on the end. Input flows
 * "downstream" from left to right through the decorators and is then
 * passed to the base function. The return value of the base function
 * flows "upstream" from right to left back through the decorators.
 *
 * Each decorator function must accept a "continuation" as its first
 * argument, followed by the arguments to be passed to the base
 * function. The continuation function represents the "next" step in
 * the decorator pipeline, and must be called to continue the flow of
 * data downstream.
 *
 * A value must be returned by a decorator function to continue the
 * flow of data upstream.
 *
 * @param func - a function to be wrapped by decorators
 * @param { ...decs } - decorators to wrap base function
 * @returns a function that pipelines func and { ...decs }
 */
export function decorate<
  F extends (...args: any[]) => any,
  D extends (f: F, ...args: Parameters<F>) => ReturnType<F>
>(func: F, ...decs: D[]): (...xs: Parameters<F>) => ReturnType<F> {
  return decs.reduceRight(
    (d2, d1) => ((...args: Parameters<F>) => d1(d2, ...args)) as F,
    func
  )
}
