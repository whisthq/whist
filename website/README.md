# Fractal Website

[![Netlify Status](https://api.netlify.com/api/v1/badges/f65a863e-37d0-4407-babd-09b2b4802661/deploy-status)](https://app.netlify.com/sites/fractal-prod/deploys) ![Website: Node.js Build and Test](https://github.com/fractal/website/workflows/Website:%20Node.js%20Build%20and%20Test/badge.svg) ![Website: Linting](https://github.com/fractal/website/workflows/Website:%20Linting/badge.svg)

This repository contains the code for the Fractal website.

## Setting Up for Development

Note: Steps 1, 2, and 3 only need to be done once.

1. Download the AWS CLI and run `aws configure`. You'll be prompted to enter your AWS Access Key
   and Secret Key, which you received in an onboarding email titled "AWS Credentials."

2. `cd` into the `scripts` folder and run `python3 retrieve.py` to download the necessary environment variables. Depending on your compuer, you may need to replace `python3` with `py`, `py3`, `python`, etc.

3. Authenticate to GitHub Packages with `npm login --scope=@fractal --registry=https://npm.pkg.github.com`. Your `username` is your GitHub username, your `password` is a GitHub personal authentication token, and your email is your `fractal.co` email.

4. Run `npm install` to install package dependencies and launch localhost via `npm start`. If you need to update dependencies, you can run `npm upgrade`, followed by `npm prune` to remove unnecessary dependencies. NOTE: If you are on an M1 Mac, make sure you are running your terminal in Rosetta and are on Node 14, which you can install by running `nvm install 14.14.0 && nvm use 14.14.0`.

## How to Contribute

Before making a pull request, ensure that the following steps are taken:

1. Make sure the application is fully working via `npm start`.

2. Make sure that your code follows the guidelines outlined in our [React coding philsophy](https://www.notion.so/tryfractal/Typescript-Coding-Philosophy-984288f157fa47f7894c886c6a95e289).

3. Lint your code by running `npm run lint-fix`. If this does not pass, your code will fail Github CI.

4. Add docstrings to every new component and function that you created.

5. Run all test files by running `npm run test` and `jest`. If this does not pass, your code will fail Github CI. If you've changed React code, your code will likely fail the corresponding snapshot test, so you'll need to update snapshots. NOTE: Any new functions or React components created must be accompanied by unit/snapshot tests, or code reviewers will request changes on your PR.

6. Rebase against `dev` by pulling `dev` and running `git rebase dev`.

Finally, you can open PR to `dev`.

## Running tests

If you've created new React components, please write unit and/or integration tests as necessary. Tests are currently written in `Jest`, `react-testing-library`, and `jest-puppeteer`. Unit and integration test files can be found in each page under the `__tests__` folder in the format `<TESTNAME>.test.tsx`, while e2e tests can be found under `testing/e2e` in the format `<TESTNAME>.e2e.ts`. Running unit/integration tests can be done by calling `npm run test <FILENAME>` or just `npm run test` for the whole suite, while e2e tests are called by `jest <FILENAME>` or `jest`. To leanr more about writing tests, go to the engineering wiki for documentation: https://www.notion.so/tryfractal/Testing-Website-d95465a920784670838f130620cd5d87

## Publishing the Website

The website auto-deploys from GitHub directly to Netlify, which is our web hosting provider. For every `push` to our main branches (dev/staging/master), the code in that branch will be automatically built and deployed on Netlify. The Netlify dashboard is managed by the code owners.

-   The branch `dev` deploys to https://dev.fractal.co.
-   The branch `staging` deploys to https://staging.fractal.co.
-   The branch `master` deploys to https://fractal.co.

For `dev` and `staging`, the password is `><mc?@,>YF?v&p,e`.

Basic continuous integration is set up for this project. For every PR, basic Node.js tests will be compiled and run within GitHub Actions, including linting. Your code needs to pass all linting and tests to be approved for merge. If you create new functions, make sure to create tests for them and add them to the continuous integration, when relevant.

## Google Analytics, A/B Testing, and Tracking

To view our Google Analytics dashboard, follow the tutorial on the [Notion Engineering Wiki](https://www.notion.so/tryfractal/Setting-up-Your-Google-Analytics-Dashboard-d5bcc39ee6c1433fa2006945d4469615).

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Prettier](https://prettier.io) before making any PRs. We have [pre-commit hooks](https://pre-commit.com/) with Prettier support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Prettier.

Additional specific checks are done by ESLint. Please run `npm run lint-check` or `npm run lint-fix` (the latter if you want to auto-fix all possible issues) and address all raised issues. If any issues seem incompatible or irrelevant to this project, add them to .eslintrc and either demote to warnings or mute entirely.

You can always run Prettier directly from a terminal by typing `npm run format`, or you can install it directly within your IDE by via the following instructions:

### [VSCode](https://marketplace.visualstudio.com/items?itemName=esbenp.prettier-vscode)

Launch VS Code Quick Open (Ctrl+P), paste the following command, and press enter.

```
ext install esbenp.prettier-vscode
```

To ensure that this extension is used over other extensions you may have installed, be sure to set it as the default formatter in your VS Code settings. This setting can be set for all languages or by a specific language.

```
{
  "editor.defaultFormatter": "esbenp.prettier-vscode",
  "[javascript]": {
    "editor.defaultFormatter": "esbenp.prettier-vscode"
  }
}
```
