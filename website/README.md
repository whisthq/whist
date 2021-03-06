# Fractal Website

![Website: Node.js Build and Test](https://github.com/fractal/website/workflows/Website:%20Node.js%20Build%20and%20Test/badge.svg)
[![Netlify Status](https://api.netlify.com/api/v1/badges/f65a863e-37d0-4407-babd-09b2b4802661/deploy-status)](https://app.netlify.com/sites/fractal-prod/deploys)

This repository contains the code for the Fractal website for application streaming. The Fractal website is used to advertise the product, and handle account management and authentication.

## Development

This project is developed using the `npm` package manager. You can start developing by running `npm install`, and can launch into a localhost via `npm start`. If you need to update dependencies, you can run `npm upgrade`, followed by `npm prune` to remove unnecessary dependencies.

### Retrieving Environment Variables

If you do not have the `.env` environment folder, run `python retrieve.py` in the `scripts` folder. This will download the environment secrets necessary to fully run the site locally. You will need to have configured your AWS Access Key and Secret Access Key locally for the script to work properly. If you haven't done so already, you can do it by running `aws configure`.

### Logging

To ensure that no lingering `console.log()` statements make it to production and can be inspected by users, please use `debugLog()` to print to the console. This custom logging function gets automatically hidden in production.

### Continous Integration and Netlify Deployments

Basic continuous integration is set up for this project. For every PR, basic Node.js tests will be compiled and run within GitHub Actions, including linting. Your code needs to pass all linting and tests to be approved for merge. If you create new functions, make sure to create tests for them and add them to the continuous integration, when relevant.

The website auto-deploys from GitHub directly to Netlify, which is our web hosting provider. For every `push` to our main branches, the code in that branch will be automatically built and deployed on Netlify. You will also see deploy previews for your branches when you make a PR - they *need* to pass for merging to be approved. You most likely won't need to access Netlify directly, but if you do ask one of the code owners.

- The branch `dev` deploys to https://fractal-dev.netlify.app.
- The branch `staging` deploys to https://fractal-staging.netlify.app.
- The branch `master` deploys to https://fractal.co.

## Google Analytics, A/B Testing, and Tracking

To improve our retention rates and scientifically approach the question of *"what makes a user stick in our website?"*, we use basic pageview and event tracking with Google Analytics. When users click a button, a Google Analytics (GA) event will trigger that will inform GA of the fact that they clicked, and it will also keep track of their rough geographical location and of which page(s) they are viewing.

The Google Analytics code is in the `withTracker.tsx` component as well as in `gaEvents`. The Google Analytics IDs that you need to connect this to your own GA account are included in `shared/constants/config`. There can be multiple IDs since Google Analytics allows us to connect multiple GA accounts to one website.

To set up your GA dashboard follow the tutorial on the [Notion Engineering Wiki](https://www.notion.so/tryfractal/Setting-up-Your-Google-Analytics-Dashboard-d5bcc39ee6c1433fa2006945d4469615).

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

## Contributing

Before contributing, please familiarize yourself with our [Coding Philosophy](https://www.notion.so/tryfractal/Engineering-Guidelines-d8a1d5ff06074ddeb8e5510b4412033b). Unless otherwise specified, contributors should branch off `dev` and PR back into `dev`. Because all three of `dev`, `staging` and `master` auto-deploy the their respective Netlify sites, pushing to `staging` and `master` is blocked by non-code owners.
