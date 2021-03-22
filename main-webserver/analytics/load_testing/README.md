1. Create branch with "LOAD_TESTING" env var
2. Make review app (see steps to making a review app) with eph dbs.
3. Add hirefire to review app. See the hirefire [dashboard](https://manager.hirefire.io/). You'll need to configure it for the review app. Copy the configurations for dev. You should get a command of the form `heroku config:add -a <HEROKU_REVIEW_APP_NAME> HIREFIRE_TOKEN=<HIREFIRE_TOKEN>` to run.
4. Set web dyno as performance-M webserver with autoscaling with 500ms desired response time.
5. Set celery dyno as Standard 1-X.
6. Setup the load test by running `python load_test_setup.py --action create_cluster --web_url $WEB_URL --admin_token $ADMIN_TOKEN`. You can get the web URL of the deployed app from the Heroku UI and the admin token from Postman. Then run `python load_test_setup.py --action make_users --web_url $WEB_URL --admin_token $ADMIN_TOKEN --num_users 100`.
7. For each load test, first set load test configurations in the AWS UI. Specifically, this would be number of processors, number of requests, the webserver URL (this changes per review app), and the admin token. Get the redis URL of the review app from the Heroku UI. Run `export REDIS_URL=<REDIS_URL>`. Then, run `celery --app entry.celery flower` in the root directory. Then, in another terminal run `bash run_distributed_load_test.sh `. Currently, the lambda function is configured in the bash script to run 4 processors and 4 requests. You can also run it from just your laptop with `python load_test.py --num_procs 1 --num_reqs 1 --web_url $WEB_URL --admin_token $ADMIN_TOKEN --outfile local_dump/out_1.json`.
8. Analyze the results on server and client ends on `analyze_load_test.ipynb`.
9. To run another load test, first make sure things are reset. Any created resources like containers should be deleted. Make sure only 1 celery and web dynos are running.
10. To make changes to the load test source, change `load_test.py` (and make sure it works with `lambda_function..py`). Install dependencies with `pip install --target ./package <packages`. Then do `make lambda-func`. This will package and push the new lambda function code to AWS.
11. Teardown with `python load_test_setup.py --action delete_cluster --web_url $WEB_URL --admin_token $ADMIN_TOKEN`

Results: https://docs.google.com/spreadsheets/u/2/d/1yGook9y4vc6leG_uGub8ft-AN9T7ilYvbxrcRPXZqSY/edit#gid=0
