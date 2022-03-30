# Whist Website

The directory contains the source code for the Whist website, hosted on Netlify.

## Development

### Setting Up for Development

1. Download the AWS CLI and run `aws configure`. You'll be prompted to enter your AWS Access Key
   and Secret Key, which you should have received during your onboarding.

2. Run `yarn -i` to install package dependencies and launch localhost via `yarn start`.

### How to Contribute

Before making a pull request, ensure that the following steps are taken:

1. Make sure the application is fully working via `yarn start`.

2. Make sure that your code follows the guidelines outlined in our [React coding philsophy](https://www.notion.so/whisthq/Typescript-Coding-Philosophy-984288f157fa47f7894c886c6a95e289).

3. Lint your code by running `yarn lint:fix`. If this does not pass, your code will fail Github CI.

4. Add docstrings to every new component and function that you created.

5. Rebase against `dev` by pulling `dev` and running `git rebase dev`.

Finally, you can open PR to `dev`.

### Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Prettier](https://prettier.io) before making any PRs. We have [pre-commit hooks](https://pre-commit.com/) with Prettier support installed on the Whist monorepository, which you can initialize by first installing pre-commit via `pip install pre-commit` from the root level, and then running `pre-commit install` to instantiate the hooks for Prettier.

Additional specific checks are done by ESLint. Please run `yarn lint:check` or `yarn lint:fix` (the latter if you want to auto-fix all possible issues) and address all raised issues. If any issues seem incompatible or irrelevant to this project, add them to `.eslintrc` and either demote to warnings or mute entirely.

## Deployment

The website auto-deploys from GitHub directly to Netlify, which is our web hosting provider. For every `push` to our main branches (dev/staging/prod), the code in that branch will be automatically built and deployed on Netlify. The Netlify dashboard is managed by the codeowners.

On all PRs to `dev`, basic linting and building checks will be performed, and a deploy preview from Netlify will be generated.

- The branch `dev` deploys to https://dev.whist.com.
- The branch `staging` deploys to https://staging.whist.com.
- The branch `prod` deploys to https://whist.com.

For `staging` the password is `><mc?@,>YF?v&p,e`. For `dev` and all deploy previews to `dev`, the password is `Mandelbox2021!`.
