import { withServer } from "../withServer";
import { decorate } from "../../utilities";
import * as mock from "./mocks";

describe("withServer", () => {
  const input = mock.REQUEST_SUCCESS;
  // Pass identity function to avoid mocking fetch
  const apiFn = decorate(mock.FETCH_IDENTITY, withServer(mock.SERVER));
  const result = apiFn(input);

  test("return value has get method", async () => {
    expect(await result).toHaveProperty("server", mock.SERVER);
  });
  test("return value has input unchanged", async () => {
    expect(await result).toMatchObject(input);
  });
});
