import { withTokenRefresh } from "../withTokenRefresh";
import { ServerRequest } from "../../types/api";
import { decorate } from "../../utilities";
import * as mock from "./mocks";

// Mock a "fetch" function taht retunrs a 400 status
const status400 = async (_: ServerRequest) => ({
  ...mock.RESPONSE_SUCCESS,
  response: {
    ...mock.MOCK_RESPONSE,
    status: 400,
  },
});

// Mock a "fetch" function that returns a 401 status
const status401 = async (_: ServerRequest) => ({
  ...mock.RESPONSE_SUCCESS,
  response: {
    ...mock.MOCK_RESPONSE,
    status: 401,
  },
});

describe("withAuth", () => {
  // A "control" function (like "control group")
  // We'll compare with this to make sure that
  // withAuth is not altering its return value
  const get400Control = decorate(status400);
  // @ts-ignore
  const get400 = decorate(status400, withTokenRefresh(mock.ENDPOINT));
  // @ts-ignore
  const get401 = decorate(status401, withTokenRefresh(mock.ENDPOINT));

  test("returns response unchanged if not 401", async () => {
    const control = await get400Control(mock.REQUEST_SUCCESS);
    const result = await get400(mock.REQUEST_SUCCESS);
    expect(result).toEqual(control);
  });

  test("return error on 401 + no refresh", async () => {
    const result = await get401(mock.REQUEST_SUCCESS);
    expect(result).toHaveProperty("error");
  });
});
