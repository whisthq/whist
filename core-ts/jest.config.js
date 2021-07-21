const { pathsToModuleNameMapper } = require("ts-jest/utils");
const { compilerOptions } = require("./tsconfig");

module.exports = {
  preset: "ts-jest",
  testEnvironment: "node",
  setupFiles: ["./setupJest.js"],
  moduleNameMapper: pathsToModuleNameMapper(compilerOptions.paths, {
    prefix: "<rootDir>/",
  }),
  automock: false,
  clearMocks: true,
  coverageDirectory: "coverage",
  moduleFileExtensions: ["ts", "tsx", "js", "jsx", "json", "node"],
  modulePathIgnorePatterns: ["dist"],
  testPathIgnorePatterns: ["mocks.ts"],
};
