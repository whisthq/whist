from subprocess import call

rc = call(
    'bash poll_task.sh "localhost:7730/" "bd86d98c-259c-40ed-bd11-e892272d2536"',
    shell=True,
)