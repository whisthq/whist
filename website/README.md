# Fractal Website

![Node.js CI](https://github.com/fractalcomputers/website/workflows/Node.js%20CI/badge.svg)

This repository contains the code for the new Fractal website for single app streaming.

## Development

The admin dashboard is developed using the `npm` package manager. You can start developing by running `npm install`, and can launch into a localhost via `npm start`.

If you need to update dependencies, you can run `npm upgrade`, followed by `npm prune` to remove unnecessary dependencies.

Basic continuous integration is set up for this project. For every push or PR, basic NodeJS tests will be compiled and run within GitHub Actions. This will also attempt to format the code via Prettier and inform you if your code is not properly formatted. You should make sure that every pull request to `master` passes the build in GitHub Actions, and that you pre-formatted the code via Prettier beforehand. 

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Prettier](https://prettier.io/) before making any PRs. We have [pre-commit hooks](https://pre-commit.com/) with Prettier support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Prettier.

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
