import { withStatusCheck } from "../withStatusCheck"
import { ServerRequest, ServerResponse } from "../../types/api"
import { decorate } from "../../utilities"
import * as mock from "./mocks"

// We're skipping this test for now, as withStatusCheck isn't
// finished yet. It may be uneeded and deteleted soon.
// Once exported by core-ts, these tests should be finished.
describe.skip("withStatusCheck", () => {
  const badStatus = async (req: ServerRequest) =>
    // @ts-ignore
    ({
      response: {
        ...mock.RESPONSE_SUCCESS,
        status: 400,
      },
      request: req,
    } as ServerResponse)
  const apiFn = decorate(badStatus, withStatusCheck)

  test("return value has ok: false if failure status", async () => {
    const result = await apiFn(mock.REQUEST_SUCCESS)
    expect(result.response?.ok).toBe(false)
  })
})
