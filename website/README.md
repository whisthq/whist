# Fractal Website

![Node.js CI](https://github.com/fractalcomputers/website/workflows/Node.js%20CI/badge.svg)
[![Netlify Status](https://api.netlify.com/api/v1/badges/6bf95ae3-d9ee-4e92-99e2-fc67c52f540f/deploy-status)](https://app.netlify.com/sites/tryfractal/deploys)

This repository contains the code for the new Fractal website for single app streaming.

## Setting Up Development

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

## Code Design Philosophy

### Pages

The website is divided into pages, where each page is defined as a component with its own React Router endpoint. Each page gets its own folder in the `pages` folder; within each page is a single base `.tsx` component, which is component rendered by the React Router, and two folders: `components` and `views`. A view is defined as a collection of components; for example, the landing page has a `TopView`, `MiddleView`, and `BottomView`, which are groups of components in the top, middle, and bottom sections of the landing page, respectively. A component is defined as any standalone React Component which is imported into a view, which is then imported into the base page component.

### Components

We choose to use functional React components instead of class components because of their simplicity and readability. For instance, whereas a traditional class component may look like 

```
export default class ExampleComponent<MyProps, MyState> extends React.Component
```

a functional component looks like

```
function ExampleComponent(props: any) 
```

For consistency, the use of functional React components is enforced across the repo.

### Inline vs. Separate Styling

We do not have a preference for inline vs. separate CSS styling, with the exception of repeated CSS code, in which case we strongly encourage that a CSS class be created. All modifications to default HTML tags (e.g. `div`, `h1`, etc.) should be done in `styles/shared.css`. Every page has its own corresponding `.css` file; all other CSS should go there.

### Naming

For consistency, we enforce all folder, variable, and file names to start with lowercase letters. If a name has multiple words (e.g. "landing page"), we format it as one word, where the first word is lowercased and the subsequent words are capitalized (e.g. `landingPage`). The one exception to the lowercase rule are component names, which are all uppercased; for instance, `import ExampleComponent from pages/ExamplePage/components/exampleComponent`.

### Warnings

To minimize the risk of bugs, we enforce that all PR's be warning-free. You can see warnings in the terminal where the website is running locally; if a PR has warnings, the CI will fail.

### State Management

We currently use Redux for state management, and try to split actions and reducers into related groups as best as possible. We are currently TBD on whether to use Redux sagas or React hooks for side effects (i.e. API calls).

### Redux State Naming

We encourage that redux state variables be grouped as nested JSON objects for improved readability. For example, instead of a Redux state that looks like 

```
EXAMPLE_STATE = {
  username: null,
  name: null,
  access_token: null,
  location: null,
  stripe_data: {...}
}
```

we can organize this state as

```
EXAMPLE_STATE = {
  auth: {
    username: null,
    name: null,
    access_token: null
  },
  account_data: {
    location: null,
    stripe_data: {...}
  }
}
```

### API Calls

We currently use Firebase, so there is no need for many API calls.

When we switch to SQL, whenever possible, we encourage the use of GraphQL instead of writing new server endpoints for the sake of simplicity and coding speed. The only time when we should be writing and calling server endpoints is if we are performing server-side operations that involve more than database operations; for example, an API call to create an ECS container.

## UI Design Philosophy

### Screen Compatibility

We require all front-end code pushed to staging be tested for compatability on both mobile and normal laptop (13-15 inch) screens, and that all code pushed into production be tested for compatability on large screens (up to 27 inch monitors) and ultrawide panels (1440p 21:9 aspect ratios). We use two types of conditional CSS styling: inline conditional styling

```
<div style = {{fontSize: state.width > 720 ? 30 : 20}}>
  Conditional font size
</div>
```

and separate conditional CSS styling

```
.myClass {
  font-size: 30px;
}

@media only screen and (max-width: 720px) {
  .myClass {
    font-size: 20px;
  }
}
```

### Images

For the best image quality, we require that all images and icons be `.svg` files. You can easily create SVG or convert images to `.svg` in Figma; simply highlight the asset(s) you wish to export, right click, and Select Export > Copy to SVG.

## Contributing

Unless otherwise specified, contributors should branch off `staging` and PR back into `staging`. Because both `staging` and `master` auto-deploy the their respective Netlify sites, pushing to `staging` and `master` is blocked by non-code owners.
