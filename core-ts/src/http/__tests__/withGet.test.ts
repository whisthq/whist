import { withGet } from "../withGet";
import { decorate } from "../../utilities";
import * as mock from "./mocks";

describe("withGet", () => {
  // Passing a "identity" function to decorate
  // means that we can test its behavior while
  // keeping it pure
  const input = mock.REQUEST_SUCCESS;
  const get = decorate(mock.FETCH_IDENTITY, withGet);
  const result = get(input);

  test("return value has get method", async () => {
    expect(await result).toHaveProperty("method", "GET");
  });
  test("return value has input unchanged", async () => {
    expect(await result).toMatchObject(input);
  });
});
