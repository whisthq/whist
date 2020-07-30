# Fractal Main Webserver

![Python Webserver CI](https://github.com/fractalcomputers/main-webserver/workflows/Python%20Webserver%20CI/badge.svg)

This repository contains the code for the VM and user webserver, which handles interfacing between users, the Fractal cloud computers hosted on Azure (currently), and a variety of website & client applications functionalities. Runs Flask with Celery for asynchronous task handling.

Our webservers are hosted on Heroku:

- [Production](https://cube-celery-vm.herokuapp.com)
- [Staging](https://cube-celery-staging.herokuapp.com)

Our production database is attached as an Heroku Add-On PostgresSQL to the associated webserver in Heroku, `main-webserver`, and has automated backups in place daily at 2 AM PST. See [here](https://devcenter.heroku.com/articles/heroku-postgres-backups#creating-a-backup) for further information.

Our webserver logs are hosted on Datadog [here](https://app.datadoghq.com/logs?cols=core_host%2Ccore_service&from_ts=1593977274176&index=&live=true&messageDisplay=inline&stream_sort=desc&to_ts=1593978174176).

## Development

### Local Setup

Docker is being leveraged to create a partial-stack (TBD on full) deployment of the `main-webserver` components, `web` and `celery`. To do so, it packages the application into an image with all necessary dependencies and then launches the application with the appropriate configurations, depending on if it's `web` or `celery` using `stem-cell.sh`.

Currently, the full environment is only partially replicated, so `retrieve_config.py` exists for collecting the appropriate environment variables needed to connect to the non-replicated portions of the environment (they are pulled from Heroku).

#### 1. Retrieve Environment Variables

Use `retrieve_config.py`. It provides a `-h` help menu for understanding parameters. On Mac/Linux, run

```sh
./retrieve_config.py [NAME OF BRANCH]
```

On Windows, run
```sh
py retrieve_config.py [NAME OF BRANCH]
```

To see the possible NAME_OF_BRANCH options, see Lines 34-38 of `retrieve_config.py`. This command will pull the config variables of NAME_OF_BRANCH and write them to `docker/.env`.

You can review `dev-base-config.json` to see which values will be overriden for local development. For example, the `REDIS_URL` will be changed to use the local Docker version.

This command requires the CLI tool `heroku` to be installed and logged in.

#### 2. Spin Up Local Servers

Use `docker-compose` to run the stack locally. First, `cd` into the `docker/` folder. Then, run the `up` command. If you are on Windows, you should run this from a command prompt in Administrator mode.

```sh
docker-compose up --build
```

If you encounter a "daemon not running" error, this likely means that Docker is not actually running. To fix this, try restarting your computer and opening the Docker desktop app; if the app opens successfully, then the issue should go away.

Review `docker-compose.yml` to see which ports the various services are hosted on. For example, `"7810:6379"` means that the Redis service, running on port 6379 internally, will be available on `localhost:7810` from the host machine. Line 25 of `docker-compose.yml` will tell you where the web server itself is running.

If you make a change to the webserver, you'll need to restart docker by first killing the server (Ctrl-C) and re-running `docker-compose up --build`.

### Heroku Setup

To push to the Heroku production/staging servers, you’ll first need to set up the Heroku CLI on your computer. Make sure you are added as a collaborator to any of the Heroku apps you plan to use. You can contact Ming, Phil, or Jonathan to be added.

#### Create new Heroku server

1. Inside your virtual environment, run the command `heroku create -a [DESIRED HEROKU SERVER NAME]`.
2. `git checkout` to the branch you want to connect the new server to, and run the command `heroku git:remote -a [HEROKU SERVER NAME] -r [DESIRED LOCAL NICKNAME]`. For instance, if the app you created is called `cube-celery-staging5`, you could run `heroku git:remote -a cube-celery-staging5 -r staging5`.
3. To transfer the environment variables over automatically, run the command `heroku config -s -a [EXISTING SERVER] > config.txt` and then `cat config.txt | tr '\n' ' ' | xargs heroku config:set -a [HEROKU SERVER NAME]`. Note that you need to be on a Mac or Linux computer to run the second command; I could not find a suitable workaround for Windows.
4. Copy the environment variables locally by running `heroku config -s -a [HEROKU SERVER NAME]` >> .env`
5. Install a Redis task Queue by going to the Heroku dashboard, finding the app you created, and navigating to Resources > Find More Addons > Heroku Redis. Follow the Heroku setup instructions and select the free plan.
6. Under the Resources tab, make sure that the "web" and "celery" workers are both toggled on.
7. All good to go! To push live, commit your branch and type `git push [LOCAL NICKNAME] [BRANCH NAME]:master`. For instance, if my nickname from step 2 is `staging5` and my branch name is `test-branch`, I will do `git push staging5 test-branch:master`.

#### [Staging](https://cube-celery-staging.herokuapp.com)

Add the heroku app as a remote: `git remote add staging https://git.heroku.com/cube-celery-staging.git`  
We have a secondary heroku staging app if two people want to deploy and test at the same time: `git remote add staging2 https://git.heroku.com/cube-celery-staging2.git`. We also have a tertiary staging app.

To push to the primary staging server, first make sure you’re in the staging branch, then type `git add .`, then `git commit -m “COMMIT MESSAGE”`, then finally `git push staging staging:master`. If you are using another branch with name as {branchName}, you push via: `git push staging {branchName}:master`. If you get a git pull error, git pull by typing `git pull staging master` to pull from Heroku or `git pull origin staging` to pull from Github. To view the server logs, type `heroku logs --tail --remote staging`.

To push to the secondary staging server, the steps are all the same, except you replace the remote with `staging2`.

To push to the Github production/staging repo, run `git add .`, `git commit -m "COMMIT MESSAGE"`, and finally `git push origin staging` to push to the staging repo, and `git push origin master` to push to the production repo.

#### [Production](https://git.heroku.com/cube-celery-vm.git)

To push to the live server, git add and commit and type git push heroku master. (Obviously, DO NOT push to the production server until we’ve all agreed that staging is stable).

Test the webserver by running it on localhost and using Postman to send requests to the localhost address, and if that works, push to staging. To view the server logs, type `heroku logs --tail --app cube-celery-vm`.

#### [SQL Database Scheme](https://pgweb-demo.herokuapp.com/)

Select Scheme, and for the server URL scheme, copy the DATABASE_URL config var found on the Heroku instance.

## Testing

We have created a Postman workspace for a variety of API endpionts in vm-webserver, which can be used for testing in the Staging and Staging2 environments.

Postman Team link: https://app.getpostman.com/join-team?invite_code=29d49d2365850ccfb50fc09723a45a93

**Pytest**
We have pytest tests in the `/tests` folder. To run tests, just run `pytest -o log_cli=true -s` in terminal. To run tests in parallel, run `pytest -o log_cli=true -s -n <num>`, with `<num>` as the # of workers in parallel.

## Publishing

Once you are ready to deploy to production, you can merge your code into master and then run `./update.sh`. The script will push your local code to Heroku on the master branch, and notify the team via Slack.

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Python black](https://github.com/psf/black) before making any PRs. You may find a variety of tutorial online for your personal setup. This README covers how to set it up on VSCode, Sublime Text and running it from the CLI. GitHub Actions will also automatically lint your code via Python Black for every push.

### [VSCode](https://medium.com/@marcobelo/setting-up-python-black-on-visual-studio-code-5318eba4cd00)

1. Install it on your virtual env or in your local python with the command:

```
$ pip install black
```

2. Now install the python extension for VS-Code, open your VS-Code and type “Ctrl + p”, paste the line bellow and hit enter:

```
ext install ms-python.python
```

3. Go to the settings in your VS-Code typing “Ctrl + ,” or clicking at the gear on the bottom left and selecting “Settings [Ctrl+,]” option.
4. Type “format on save” at the search bar on top of the Settings tab and check the box.
5. Search for “python formatting provider” and select “black”.
6. Now open/create a python file, write some code and save(Ctrl+s) it to see the magic happen!

### [CLI](https://github.com/psf/black)

Installation:  
Black can be installed by running `pip install black`. It requires Python 3.6.0+ to run but you can reformat Python 2 code with it, too.

Usage:  
To get started right away with sensible defaults:

```
black {source_file_or_directory}
```

Black doesn't provide many options. You can list them by running `black --help`:
