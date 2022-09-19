module.exports = {
  root: true,
  env: {
    browser: true,
    es2021: true,
  },
  extends: [
    "plugin:react/recommended",
    "standard-with-typescript",
    "plugin:prettier/recommended",
  ],
  parser: "@typescript-eslint/parser",
  parserOptions: {
    ecmaFeatures: {
      jsx: true,
    },
    ecmaVersion: 12,
    sourceType: "module",
    project: "./tsconfig.json",
  },
  plugins: ["react", "@typescript-eslint", "prettier"],
  rules: {
    "@typescript-eslint/explicit-function-return-type": "off",
    "@typescript-eslint/consistent-type-assertions": "off",
    "prettier/prettier": "error",
  },
  settings: {
    react: {
      version: "detect",
    },
  },
  overrides: [
    // This override hides incorrect usage of async/await in rxjs subscriptions. Instead of
    // doing that, we should be doing anything asynchronous in the observable pipe chain.
    // TODO: Fix this after Suriya's major rewrite for spotlighting tabs.
    {
      files: ["*.ts"],
      rules: {
        "@typescript-eslint/no-misused-promises": "off",
      },
    },
  ],
}
