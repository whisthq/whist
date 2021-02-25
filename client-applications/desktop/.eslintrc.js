module.exports = {
    extends: ['erb/typescript', 'prettier', 'prettier/react'],
    rules: {
        'no-console': 'off',
        // A temporary hack related to IDE not resolving correct package.json
        'import/no-extraneous-dependencies': 'off',
        // camelcase is a default typescript setting
        '@typescript-eslint/camelcase': ['error', { ignoreImports: true }],
        // disable the below checks
        '@typescript-eslint/no-explicit-any': 0,
        'global-require': 0,
        'no-param-reassign': 0,
        'object-shorthand': 0,
        'prefer-destructuring': 0,
        'react/destructuring-assignment': 0,
        'react/jsx-indent': 0, // we use prettier's indent rules
        'react/jsx-indent-props': 0, // we use prettier's indent rules
        'react/jsx-key': 0,
        'vars-on-top': 0,
        'import/prefer-default-export': 0,

        // demote the below checks to warning instead of error
        'no-shadow': 1,
        'react/jsx-props-no-spreading': 1,
        'import/no-named-as-default': 'off',
        // anything beginning with an underscore is excused
        '@typescript-eslint/no-unused-vars': [1, { argsIgnorePattern: "^_" }],
    },
    settings: {
        'import/resolver': {
            // See https://github.com/benmosher/eslint-plugin-import/issues/1396#issuecomment-575727774 for line below
            node: {},
            webpack: {
                config: require.resolve('./configs/webpack.config.eslint.js'),
            },
        },
    },
}
