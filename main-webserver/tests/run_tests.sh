
echo "=== Make sure to run tests/setup/setup_tests.sh once prior to this ==="

# if in CI, run setup tests and set env vars
if [ -z "${IN_CI}" ]; then
    # these are needed to migrate schema/data
    export POSTGRES_HOST=$DATABASE_URL
    export POSTGRES_LOCAL_HOST=$HEROKU_POSTGRES_URL # set in app.json
    cd setup
    bash setup_tests.sh
    cd ..
    # points app to use CI db. 
    export POSTGRES_HOST=$POSTGRES_LOCAL_HOST

# if local, setup_tests should already be run (once)
pytest --no-mock-aws
