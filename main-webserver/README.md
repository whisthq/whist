# Fractal Users & VM Webserver

Webserver to handle interfacing with Fractal cloud computers hosted on Azure, and for interfacing with a variety of website functionalities. Runs Flask with Celery for asynchronous task handling.

Production hosted on Heroku: https://dashboard.heroku.com/apps/cube-celery-vm

Staging hosted on Heroku: https://dashboard.heroku.com/apps/cube-celery-staging

## Setup
### Local setup (Windows/MacOS)
1. Set up the Heroku CLI on your computer
2. Check your python version by typing `python -V`. If you have python 3.6.X, you can create a virtual environment for yourself by typing `virtualenv env` and then run the python executable listed in the install text, i.e. `source env\Scripts\activate`. If you have Python >3.6 or Python <3.0, you will need to create a Python 3.6 virtual environment. To do this, first install python 3.6.8 from the Python website. Find the directory where python.exe is installed. Make sure you are cd'ed into the vm-webserver folder, then type `virtualenv --python=[DIRECTORY PATH] venv` in your terminal. The terminal should output a "created virtual environment CPython3.6.8" message. Activate it by typing `source venv\Scripts\activate` (Windows) or `source env/bin/activate` (MacOS). You will need to type this last command every time to access your virtual environment.
3. Install everything by typing `pip install -r requirements.txt`. Make sure you're in the virtual environment when doing this.
4. Tell the local environment what the entry point is to the webserver by typing `set FLASK_APP=run.py`. 
5. Import the environment variables into your computer by typing `heroku config -s --app <APP> >> .env`. App is either `cube-celery-vm` if you are working on the production webserver, or `cube-celery-staging` if you are working on the staging webserver.
6. Type `flask run` to start the webserver on localhost.
7. [NOTE: Currently buggy] If you plan on calling endpoints that require celery, you will want to view the celery task queue locally. To do this, open a new terminal, cd into the vm-webserver folder, and type `celery -A app.tasks worker --loglevel=info`.
8. Then, in a new terminal, attach the staging branch to cube-celery-staging by typing `git checkout staging`, then `heroku git:remote --app cube-celery-staging -r staging`
9. Also in the new terminal, attach the master branch to cube-celery-vm by typing `git checkout master`, then `heroku git:remote --app cube-celery-vm -r heroku`

### Run on Heroku
**Staging**
`https://git.heroku.com/cube-celery-staging.git`

To push to the Heroku production/staging servers, you’ll first need to set up the Heroku CLI on your computer. To push to the staging server, first make sure you’re in the staging branch, then type `git add .`, then `git commit -m “COMMIT MESSAGE”`, then finally `git push staging staging:master`. If you get a git pull error, git pull by typing `git pull staging master` to pull from Heroku or `git pull origin staging` to pull from Github. To view the server logs, type `heroku logs --tail --remote staging`.

To push to the Github production/staging repo, run `git add .`, `git commit -m "COMMIT MESSAGE"`, and finally `git push origin staging` to push to the staging repo, and `git push origin master` to push to the production repo.

**Production**
`https://git.heroku.com/cube-celery-vm.git`

To push to the live server, git add and commit and type git push heroku master. (Obviously, DO NOT push to the production server until we’ve all agreed that staging is stable). 

Test the webserver by running it on localhost and using Postman to send requests to the localhost address, and if that works, push to staging. To view the server logs, type `heroku logs --tail --app cube-celery-vm`.

**Database Scheme**
Access the SQL database here: https://pgweb-demo.herokuapp.com/

Select Scheme, and for the server URL scheme, copy the DATABASE_URL config var found on the Heroku instance.
