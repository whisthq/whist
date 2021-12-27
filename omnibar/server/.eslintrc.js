module.exports = {
  env: {
    browser: true,
    es2021: true,
  },
  extends: ["standard-with-typescript", "plugin:prettier/recommended"],
  parser: "@typescript-eslint/parser",
  parserOptions: {
    ecmaFeatures: {
      jsx: true,
    },
    ecmaVersion: 12,
    sourceType: "module",
    project: "./tsconfig.json",
  },
  plugins: ["@typescript-eslint", "prettier"],
  rules: {
    "@typescript-eslint/explicit-function-return-type": "off",
    "@typescript-eslint/consistent-type-assertions": "off",
    "prettier/prettier": "error",
  },
};
