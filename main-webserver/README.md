# Fractal Users & VM Webserver

Webserver to handle interfacing with Fractal cloud computers hosted on Azure, and for interfacing with a variety of website functionalities. Runs Flask with Celery for asynchronous task handling.

Hosted on Heroku: https://dashboard.heroku.com/apps/cube-celery-vm

## Setup
### Local setup (Windows)
1. Set up the Heroku CLI on your computer
2. Ceate a virtual environment for yourself by typing `virtualenv env` and then run the python executable listed in the install text, i.e. `source env\Scripts\activate`.
3. Install everything by typing `pip install -r requirements.txt`. 
4. Tell the local environment what the entry point is to the webserver by typing `set FLASK_APP=run.py`. 
5. Import the environment variables into your computer by typing `heroku config -s >> .env`. 
6. Type `flask run` to start the webserver on localhost

### Run on Heroku
**Staging**
`https://git.heroku.com/cube-celery-staging.git`

To push to the Heroku production/staging servers, you’ll first need to set up the Heroku CLI on your computer. To push to the staging server, first make sure you’re in the staging branch, then type `git add .`, then `git commit -m “COMMIT MESSAGE”`, then finally `git push staging staging:master`.

**Production**
`https://git.heroku.com/cube-celery-vm.git`

To push to the live server, git add and commit and type git push heroku master. (Obviously, DO NOT push to the production server until we’ve all agreed that staging is stable). 

Test the webserver by running it on localhost and using Postman to send requests to the localhost address, and if that works, push to staging.
