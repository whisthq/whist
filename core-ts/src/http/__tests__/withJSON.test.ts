import { withJSON } from "../withJSON";
import { fetchBase } from "../fetchBase";
import { decorate } from "../../utilities";
import * as mock from "./mocks";
import fetchMock from "jest-fetch-mock";

describe("withJSON", () => {
  // We'll create a toy body to pass to the mock fetch
  const mockedBody = { user: "John Doe" };
  const mockedRequest = mock.GET_REQUEST_SUCCESS;
  // The mocked fetch returns the argument we pass here
  fetchMock.mockResponse(JSON.stringify(mockedBody));
  const get = decorate(fetchBase, withJSON);
  // Here we make a call to fetch for a "reference" response
  // which we'll compare in a test
  const fetchResponse = fetch(mockedRequest.url, mockedRequest);
  // Here we make our actual API call, which should received
  // a JSON response from the mocked fetch
  const result = get(mockedRequest);

  test("return value has json", async () => {
    const { json } = await result;
    expect(json).toStrictEqual(mockedBody);
  });
  test("return value has input unchanged", async () => {
    const { response } = await result;
    expect(response).toMatchObject(await fetchResponse);
  });
});
