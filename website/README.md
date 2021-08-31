# Fractal Website

[![Netlify Status](https://api.netlify.com/api/v1/badges/f65a863e-37d0-4407-babd-09b2b4802661/deploy-status)](https://app.netlify.com/sites/fractal-prod/deploys) ![Website: Node.js Build and Test](https://github.com/fractal/website/workflows/Website:%20Node.js%20Build%20and%20Test/badge.svg) ![Website: Linting](https://github.com/fractal/website/workflows/Website:%20Linting/badge.svg)

This repository contains the code for the Fractal website.

## Setting Up for Development

Note: Steps 1, 2, and 3 only need to be done once.

1. Download the AWS CLI and run `aws configure`. You'll be prompted to enter your AWS Access Key
   and Secret Key, which you received in an onboarding email titled "AWS Credentials."

2. `cd` into the `scripts` folder and run `python3 retrieve.py` to download the necessary environment variables. Depending on your computer, you may need to replace `python3` with `py`, `py3`, `python`, etc.

3. Run `yarn` to install package dependencies and launch localhost via `yarn start`.

## How to Contribute

Before making a pull request, ensure that the following steps are taken:

1. Make sure the application is fully working via `yarn start`.

2. Make sure that your code follows the guidelines outlined in our [React coding philsophy](https://www.notion.so/tryfractal/Typescript-Coding-Philosophy-984288f157fa47f7894c886c6a95e289).

3. Lint your code by running `yarn lint:fix`. If this does not pass, your code will fail Github CI.

4. Add docstrings to every new component and function that you created.

5. Rebase against `dev` by pulling `dev` and running `git rebase dev`.

Finally, you can open PR to `dev`.

## Publishing the Website

The website auto-deploys from GitHub directly to Netlify, which is our web hosting provider. For every `push` to our main branches (dev/staging/prod), the code in that branch will be automatically built and deployed on Netlify. The Netlify dashboard is managed by the code owners.

- The branch `dev` deploys to https://dev.fractal.co.
- The branch `staging` deploys to https://staging.fractal.co.
- The branch `prod` deploys to https://fractal.co.

For `staging` the password is `><mc?@,>YF?v&p,e`. For `dev` the password is `Mandelbox2021!`.

Basic continuous integration is set up for this project. For every PR, basic Node.js tests will be compiled and run within GitHub Actions, including linting. Your code needs to pass all linting and tests to be approved for merge. If you create new functions, make sure to create tests for them and add them to the continuous integration, when relevant.

## Google Analytics, A/B Testing, and Tracking

To view our Google Analytics dashboard, follow the tutorial on the [Notion Engineering Wiki](https://www.notion.so/tryfractal/Setting-up-Your-Google-Analytics-Dashboard-d5bcc39ee6c1433fa2006945d4469615).

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Prettier](https://prettier.io) before making any PRs. We have [pre-commit hooks](https://pre-commit.com/) with Prettier support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Prettier.

Additional specific checks are done by ESLint. Please run `yarn lint:check` or `yarn lint:fix` (the latter if you want to auto-fix all possible issues) and address all raised issues. If any issues seem incompatible or irrelevant to this project, add them to .eslintrc and either demote to warnings or mute entirely.

You can always run Prettier directly within your IDE by via the following instructions:

### [VSCode](https://marketplace.visualstudio.com/items?itemName=esbenp.prettier-vscode)

Launch VS Code Quick Open (Ctrl+P), paste the following command, and press enter.

```bash
ext install esbenp.prettier-vscode
```

To ensure that this extension is used over other extensions you may have installed, be sure to set it as the default formatter in your VS Code settings. This setting can be set for all languages or by a specific language.

```json
{
  "editor.defaultFormatter": "esbenp.prettier-vscode",
  "[javascript]": {
    "editor.defaultFormatter": "esbenp.prettier-vscode"
  }
}
```
