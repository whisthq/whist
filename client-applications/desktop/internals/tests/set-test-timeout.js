// 60 seconds was selected to balance two competing interests (in priority order):
// 1. % of test failures due to arbitrary factors, such as intermittent network problems, is ideally 0%.
// 2. Tests shouldn't hang CI forever.
jest.setTimeout(60 * 1000)

// This file should be included by the jest config under the key `setupFilesAfterEnv` which will
// ensure it runs before any tests but after the test environment has loaded.
