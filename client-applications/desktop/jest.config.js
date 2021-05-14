/*
 * For a detailed explanation regarding each configuration property and type check, visit:
 * https://jestjs.io/docs/en/configuration.html
 */

module.exports = {
  // Indicates which provider should be used to instrument code for coverage
  coverageProvider: "babel",
  // An array of file extensions your modules use
  moduleFileExtensions: ["js", "json", "jsx", "ts", "tsx", "node"],
  // A map from regular expressions to module names or to arrays of module names that allow to stub out resources with a single module
  moduleNameMapper: {
    "@app/(.*)": "<rootDir>/src/$1",
  },
  // A preset that is used as a base for Jest's configuration
  preset: "ts-jest",
  // A list of paths to directories that Jest should use to search for files in
  roots: ["<rootDir>/src"],
  // Allows you to use a custom runner instead of Jest's default test runner
  runner: "@jest-runner/electron/main",
  // The test environment that will be used for testing
  testEnvironment: "node",
  // A map from regular expressions to paths to transformers
  transform: {
    "^.+\\.tsx?$": "babel-jest",
    "^.+\\.ts?$": "babel-jest",
  },
}
