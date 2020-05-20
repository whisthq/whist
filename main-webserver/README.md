# Fractal Users & VM Webserver

Webserver to handle interfacing with Fractal cloud computers hosted on Azure, and for interfacing with a variety of website functionalities. Runs Flask with Celery for asynchronous task handling.

Production hosted on Heroku: https://cube-celery-vm.herokuapp.com

Heroku Dashboard: https://dashboard.heroku.com/apps/cube-celery-vm

Staging hosted on Heroku: https://cube-celery-staging.herokuapp.com

Heroku Dashboard: https://dashboard.heroku.com/apps/cube-celery-staging

## Setup

### Local setup (Windows/MacOS)

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

### Run on Heroku

To push to the Heroku production/staging servers, you’ll first need to set up the Heroku CLI on your computer. Make sure you are added as a collaborator to any of the Heroku apps you plan to use. You can contact Ming, Phil, or Jonathan to be added.

**Staging**

Add the heroku app as a remote: `git remote add staging https://git.heroku.com/cube-celery-staging.git`  
We have a secondary heroku staging app if two people want to deploy and test at the same time: `git remote add staging2 https://git.heroku.com/cube-celery-staging2.git`

To push to the primary staging server, first make sure you’re in the staging branch, then type `git add .`, then `git commit -m “COMMIT MESSAGE”`, then finally `git push staging staging:master`. If you are using another branch with name as {branchName}, you push via: `git push staging {branchName}:master`. If you get a git pull error, git pull by typing `git pull staging master` to pull from Heroku or `git pull origin staging` to pull from Github. To view the server logs, type `heroku logs --tail --remote staging`.

To push to the secondary staging server, the steps are all the same, except you replace the remote with `staging2`.

To push to the Github production/staging repo, run `git add .`, `git commit -m "COMMIT MESSAGE"`, and finally `git push origin staging` to push to the staging repo, and `git push origin master` to push to the production repo.

**Production**
`https://git.heroku.com/cube-celery-vm.git`

To push to the live server, git add and commit and type git push heroku master. (Obviously, DO NOT push to the production server until we’ve all agreed that staging is stable).

Test the webserver by running it on localhost and using Postman to send requests to the localhost address, and if that works, push to staging. To view the server logs, type `heroku logs --tail --app cube-celery-vm`.

**Database Scheme**
Access the SQL database here: https://pgweb-demo.herokuapp.com/

Select Scheme, and for the server URL scheme, copy the DATABASE_URL config var found on the Heroku instance.

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Python black](https://github.com/psf/black). You may find a variety of tutorial online for your personal setup. This README covers how to set it up on VSCode.

### Python Black on VSCode

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

<sub>[Source](https://medium.com/@marcobelo/setting-up-python-black-on-visual-studio-code-5318eba4cd00)</sub>
