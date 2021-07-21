module.exports = {
    env: {
        browser: true,
        es2021: true,
        node: true
    },
    extends: [
        "standard",
        "prettier",
        "prettier/prettier",
    ],
    parser: "@typescript-eslint/parser",
    parserOptions: {
        ecmaFeatures: {
            jsx: true
        },
        ecmaVersion: 12,
        sourceType: "module"
    },
    plugins: [
        ["@typescript-eslint", "prettier"]
    ],
    rules: {
        "@typescript-eslint/explicit-function-return-type": "off",
        "@typescript-eslint/consistent-type-assertions": "off",
        "prettier/prettier": "error",
    },
}
