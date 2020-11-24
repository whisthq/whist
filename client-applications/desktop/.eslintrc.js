module.exports = {
    extends: "erb/typescript",
    rules: {
        // A temporary hack related to IDE not resolving correct package.json
        "import/no-extraneous-dependencies": "off",
        // camelcase is a default typescript setting
        "@typescript-eslint/camelcase": ["error", {"ignoreImports": true}],
        // disable the below checks
        "@typescript-eslint/no-explicit-any": 0,
        "global-require": 0,
        "object-shorthand": 0
    },
    settings: {
        "import/resolver": {
            // See https://github.com/benmosher/eslint-plugin-import/issues/1396#issuecomment-575727774 for line below
            node: {},
            webpack: {
                config: require.resolve("./configs/webpack.config.eslint.js"),
            },
        },
    },
}
