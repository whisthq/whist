
echo "=== Make sure to run tests/setup/setup_tests.sh once prior to this ==="

# if in CI, run setup tests and set env vars
if [ -z "${IN_CI}" ]; then
    #TODO: get env vars
    export POSTGRES_LOCAL_HOST="will not work"
    export POSTGRES_LOCAL_PORT="idk"
    cd setup
    bash setup_tests.sh
    cd ..
    # points app to use CI db. 
    export POSTGRES_HOST=$POSTGRES_LOCAL_HOST
    export POSTGRES_PORT=$POSTGRES_LOCAL_PORT
# if not in CI (so local), use downloaded .env in docker
else; then
    # add to current env
    export $(cat ../docker/.env | xargs)
    # points app to use local db, but keep user/password/db the same
    # NOTE: these are NOT being passed to the setup_tests.sh
    export POSTGRES_HOST="localhost"
    export POSTGRES_PORT="9999"
fi

pytest --no-mock-aws
