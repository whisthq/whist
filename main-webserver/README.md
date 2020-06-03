# Fractal User & VM Webserver

![Python App CI](https://github.com/fractalcomputers/vm-webserver/workflows/Python%20App%20CI/badge.svg)

This repo contains the code for the VM and user webserver, which handles interfacing between users and the Fractal cloud computers hosted on Azure (currently), and for interfacing with a variety of website functionalities. Runs Flask with Celery for asynchronous task handling.

Production hosted on Heroku: https://cube-celery-vm.herokuapp.com

Heroku Dashboard: https://dashboard.heroku.com/apps/cube-celery-vm

Staging hosted on Heroku: https://cube-celery-staging.herokuapp.com

Heroku Dashboard: https://dashboard.heroku.com/apps/cube-celery-staging

## Development

Here are the main setups to run this webserver locally and on Heroku. If developing mainly in Heroku, you should make sure to commit your latest code to GitHub, since this is where our development happens. We have basic continuous integration set via GitHub Actions. For every push or PR to master, the commit will be built and formatted via Python Black, see below. You should always make sure that your code passes the tests in the Actions tab.

### Local Setup (Windows/MacOS)

1. Set up the Heroku CLI on your computer
2. Check your python version by typing `python -V`.
  - If you have python 3.6.X:
    - Create a virtual environment for yourself by typing `virtualenv env` and then run the python executable listed in the install text, i.e. `source env\Scripts\activate` in Windows, or `source env/bin/activate` on Linux
  - If you have Python >3.6 or Python <3.0:
    - Create a Python 3.6 virtual environment. To do this, first install python 3.6.8 from the Python website.
    - Find the directory where python.exe is installed. Make sure you are cd'ed into the vm-webserver folder, then type `virtualenv --python=[DIRECTORY PATH] venv` in your terminal. The terminal should output a "created virtual environment CPython3.6.8" message.
    - Activate it by typing `source venv\Scripts\activate` (Windows) or `source venv/bin/activate` (MacOS/Linux). You will need to type this last command every time to access your virtual environment.
3. Install everything by typing `pip install -r requirements.txt`. Make sure you're in the virtual environment when doing this.
4. Tell the local environment what the entry point is to the webserver by typing `set FLASK_APP=run.py`.
5. Import the environment variables into your computer by typing `heroku config -s --app <APP> >> .env`. App is either `cube-celery-vm` if you are working on the production webserver, or `cube-celery-staging` if you are working on the staging webserver.
6. Type `flask run` to start the webserver on localhost.
7. [NOTE: Currently buggy] If you plan on calling endpoints that require celery, you will want to view the celery task queue locally. To do this, open a new terminal, cd into the vm-webserver folder, and type `celery -A app.tasks worker --loglevel=info`.
8. Then, in a new terminal, attach the staging branch to cube-celery-staging by typing `git checkout staging`, then `heroku git:remote --app cube-celery-staging -r staging`
9. Also in the new terminal, attach the master branch to cube-celery-vm by typing `git checkout master`, then `heroku git:remote --app cube-celery-vm -r heroku`

### Build/Run in Docker

To build the Docker image, run: `docker build -t vm-webserver`. You will require the Heroku API key as an environment variable, `HEROKU_API_KEY=<heroku-api-key>`. You can store all Docker environment variables in a file such as `.envdocker`, to save yourself time between runs. You can then run Docker with:

`docker run --env-file .envdocker -t vm-webserver:latest`

or 

`docker run --env HEROKU_API_KEY=<heroku-api-key> -t vm-webserver:latest`

### Build/Run in Vagrant

Vagrant allows you to build local VMs which can be used to run code seamlessly. It is done via the Git submodule to the `fractalcomputers/vagrant` repo.

First, make sure you initialized submodules via: `git submodule update --init --recursive`. After the submodules are initialized you will find the Vagrant configs in `vagrant/`.

You will first need to download the win10-dev.box image. To do so, run `aws s3 cp s3://fractal-private-dev/win10-dev.box win10-dev.box` as detailed in `vagrant/README.md`, or you can also manually download it from the S3 bucket. If you try to download it via the AWS CLI, you will first need to install the CLI and configure permissions, which is explained the the Vagrant repo README. You can then follow the instructions in `vagrant/README.md` for running VMs with the vm-webserver repo in them.  

Quick Commands:
```
# vagrant up commands will take some time
vagrant up
vagrant up win10 # only win10
vagrant ssh win10
vagrant ssh ubuntu

# code is located under /vagrant
vagrant destroy # destroys vms and cleans up
```

### Run on Heroku

**Staging**
`https://git.heroku.com/cube-celery-staging.git`

To push to the Heroku production/staging servers, you’ll first need to set up the Heroku CLI on your computer. To push to the staging server, first make sure you’re in the staging branch, then type `git add .`, then `git commit -m “COMMIT MESSAGE”`, then finally `git push staging staging:master`. If you are using another branch with name as {branchName}, you push via: `git push staging {branchName}:master`. If you get a git pull error, git pull by typing `git pull staging master` to pull from Heroku or `git pull origin staging` to pull from Github. To view the server logs, type `heroku logs --tail --remote staging`.

To push to the Github production/staging repo, run `git add .`, `git commit -m "COMMIT MESSAGE"`, and finally `git push origin staging` to push to the staging repo, and `git push origin master` to push to the production repo.

**Production**
`https://git.heroku.com/cube-celery-vm.git`

To push to the live server, git add and commit and type git push heroku master. (Obviously, DO NOT push to the production server until we’ve all agreed that staging is stable).

Test the webserver by running it on localhost and using Postman to send requests to the localhost address, and if that works, push to staging. To view the server logs, type `heroku logs --tail --app cube-celery-vm`.

**SQL Database Scheme**
`https://pgweb-demo.herokuapp.com/`

Select Scheme, and for the server URL scheme, copy the DATABASE_URL config var found on the Heroku instance.

## Publishing

Once you are ready to deploy to production, you can merge your code into master and then run `./update.sh`. The script will push your local code to Heroku on the master branch, and notify the team via Slack.

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Python black](https://github.com/psf/black). You may find a variety of tutorial online for your personal setup. This README covers how to set it up on VSCode.

#### [VSCode](https://medium.com/@marcobelo/setting-up-python-black-on-visual-studio-code-5318eba4cd00)

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

#### [Sublime](https://github.com/jgirardet/sublack)

#### [CLI](https://github.com/psf/black)

Installation:  
Black can be installed by running `pip install black`. It requires Python 3.6.0+ to run but you can reformat Python 2 code with it, too.

Usage:  
To get started right away with sensible defaults:

```
black {source_file_or_directory}
```

Black doesn't provide many options. You can list them by running `black --help`:
