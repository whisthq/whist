# Fractal Website

![Node.js CI](https://github.com/fractal/website/workflows/Node.js%20CI/badge.svg)
[![Netlify Status](https://api.netlify.com/api/v1/badges/6bf95ae3-d9ee-4e92-99e2-fc67c52f540f/deploy-status)](https://app.netlify.com/sites/tryfractal/deploys)

This repository contains the code for the new Fractal website for single app streaming.

## Setting Up Development

The admin dashboard is developed using the `npm` package manager. You can start developing by running `npm install`, and can launch into a localhost via `npm start`.

If you need to update dependencies, you can run `npm upgrade`, followed by `npm prune` to remove unnecessary dependencies.

Basic continuous integration is set up for this project. For every push or PR, basic NodeJS tests will be compiled and run within GitHub Actions. This will also attempt to format the code via Prettier and inform you if your code is not properly formatted. You should make sure that every pull request to `master` passes the build in GitHub Actions, and that you pre-formatted the code via Prettier beforehand.

To ensure that no lingering `console.log()` statements make it to production and can be inspected by users, please use `debugLog()` to print to the console. This custom logging function gets automatically hidden in production.

If you do not have the `.env` environment folder. Run `python retrieve.py` in the `scripts` folder. It will download the environment secrets necessary to run the site locally. You will need to export your aws secret access key and access key id for it to work with `export AWS_SECRET_ACCESS_KEY=...`/`set AWS_SECRET_ACCESS_KEY` and `export AWS_ACCESS_KEY_ID=...`/`set AWS_ACCESS_KEY_ID` (replace `...` with each of the two values respectively). You should be able to see these keys in the onboarding email from Phil or your AWS account. Ask someone with root user access if you do not know what your credentials are (probably Ming or Phil). If the code is not working and you do not have time to fix it, consider using the CLI or AWS Console per the instructions in the Notion Engineering Wiki doc.

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Prettier](https://prettier.io/) before making any PRs. We have [pre-commit hooks](https://pre-commit.com/) with Prettier support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Prettier.

You can always run Prettier directly from a terminal by typing `npm run format`, or you can install it directly within your IDE by via the following instructions:

### [VSCode](https://marketplace.visualstudio.com/items?itemName=esbenp.prettier-vscode)

Additional specific checks are done by ESLint. Please run `npm run lint-check` or `npm run lint-fix` (the latter if you want to auto-fix all possible issues) and address all raised issues. If any issues seem incompatible or irrelevant to this project, add them to .eslintrc and either demote to warnings or mute entirely.

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

Before contributing, please familiarize yourself with your [coding philosophy](https://www.notion.so/tryfractal/Setting-up-Your-Google-Analytics-Dashboard-d5bcc39ee6c1433fa2006945d4469615). Unless otherwise specified, contributors should branch off `staging` and PR back into `staging`. Because both `staging` and `master` auto-deploy the their respective Netlify sites, pushing to `staging` and `master` is blocked by non-code owners.

## Google Analytics, A/B Testing, and Tracking

To improve our retention rates and scientifically approach the question of "what makes a user stick in our website?" we use basic pageview and event tracking with Google analytics. When users click a button a google analytics (GA) event will trigger that will inform GA of the fact that they clicked, and it will also keep track of their rough geographical locations and page views (which pages they are viewing).

The GA code is in the `withTracker.tsx` component as well as in gaEvents. The GA ids that you need to connect this to your own GA account are included in config. There can be multiple since GA allows us to connect multiple GA accounts to one website, which is good, since that means multiple team members working on A/B testing and analytics can link their GA account to Fractal and avoid having a shared account.

To set up your GA dashboard follow the tutorial on the [Notion engineering wiki](https://www.notion.so/tryfractal/Setting-up-Your-Google-Analytics-Dashboard-d5bcc39ee6c1433fa2006945d4469615).
