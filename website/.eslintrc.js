module.exports = {
    extends: ["react-app"],
    parser: `@typescript-eslint/parser`,
    parserOptions: {
        project: `./tsconfig.json`,
        extraFileExtensions: [".snap"],
    },
    rules: {
        // A temporary hack related to IDE not resolving correct package.json
        "import/no-extraneous-dependencies": "off",
        // camelcase is a default typescript setting
        "@typescript-eslint/naming-convention": [
            "warn",
            {
                selector: "function",
                format: ["PascalCase", "camelCase"],
            },
        ],
        "@typescript-eslint/no-explicit-any": 0,
        "global-require": 0,
        "no-param-reassign": 0,
        "object-shorthand": 0,
        "prefer-destructuring": 0,
        "react/destructuring-assignment": 0,
        "react/jsx-indent": 0, // we use prettier's indent rules
        "react/jsx-indent-props": 0, // we use prettier's indent rules
        "react/jsx-key": 0,
        "vars-on-top": 0,

        // demote the below checks to warning instead of error
        "import/prefer-default-export": "off",
        "no-shadow": 1,
        "react/jsx-props-no-spreading": "off",
        "react/jsx-one-expression-per-line": "off",
        strict: 2,
        "no-console": 1,
    },
}
