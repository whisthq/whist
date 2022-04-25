const makeModuleNameMapper = (srcPath, tsconfigPath) => {
  // Get paths from tsconfig
  const { paths } = require(tsconfigPath).compilerOptions

  const aliases = {}

  // Iterate over paths and convert them into moduleNameMapper format
  Object.keys(paths).forEach((item) => {
    const key = item.replace("/*", "/(.*)")
    const path = paths[item][0].replace("/*", "/$1")
    aliases[key] = srcPath + "/" + path
  })
  return aliases
}

const TS_CONFIG_PATH = "./tsconfig.json"
const SRC_PATH = "<rootDir>"

module.exports = {
  roots: ["src"],
  transform: {
    "^.+\\.tsx?$": "ts-jest",
  },
  testRegex: "(/__tests__/.*|(\\.|/)(test|spec))\\.tsx?$",
  moduleFileExtensions: ["ts", "js", "json", "node", "tsx"],
  collectCoverage: true,
  clearMocks: true,
  coverageDirectory: "coverage",
  moduleNameMapper: makeModuleNameMapper(SRC_PATH, TS_CONFIG_PATH),
  testEnvironment: "jsdom",
  setupFilesAfterEnv: ["<rootDir>/scripts/setupJest.ts"],
}
