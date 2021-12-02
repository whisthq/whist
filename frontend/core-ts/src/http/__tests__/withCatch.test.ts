import { withCatch } from "../withCatch"
import { decorate } from "../../utilities"
import * as mock from "./mocks"

describe("withCatch", () => {
  // Create a function to decorate that will
  // throw an error
  const message = "Something Exploded!"
  const errorFn = (x: any) => {
    if (x) {
      throw new Error(message)
    } else {
      return x
    }
  }
  // Create the decorated get function,
  // which we've set up to error
  const get = decorate(errorFn, withCatch)
  const mockedRequest = mock.REQUEST_SUCCESS
  const result = get(mockedRequest)

  test("return value has error obj", async () => {
    const { error } = await result
    expect(error.message).toStrictEqual(message)
  })

  test("return value has request obj", async () => {
    expect(await result).toHaveProperty("request", mockedRequest)
  })

  test("catches thrown exception", async () => {
    expect(get).not.toThrow()
  })
})
