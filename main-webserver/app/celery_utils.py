import ssl


def init_celery(celery, app):
    key_file = "./TMP_SAVE/key.pem"
    cert_file = "./TMP_SAVE/cert.pem"
    ca_file = "./TMP_SAVE/cert.pem"
    celery.conf.update(
        task_track_started=True,
        accept_content=["json"],
        result_accept_content=["json"],
        worker_hijack_root_logger=True,
        celery_redis_max_connections=40,
        # redis_backend_use_ssl={"ssl_cert_reqs": ssl.CERT_NONE},
        # broker_use_ssl={"ssl_cert_reqs": ssl.CERT_NONE},
        broker_use_ssl={
            "ssl_keyfile": key_file,
            "ssl_certfile": cert_file,
            "ssl_ca_certs": ca_file,
            "ssl_cert_reqs": ssl.CERT_REQUIRED,
        },
        redis_backend_use_ssl={
            "ssl_keyfile": key_file,
            "ssl_certfile": cert_file,
            "ssl_ca_certs": ca_file,
            "ssl_cert_reqs": ssl.CERT_REQUIRED,
        },
    )
    TaskBase = celery.Task  # pylint: disable=invalid-name

    class ContextTask(TaskBase):
        def __call__(self, *args, **kwargs):
            with app.app_context():
                return TaskBase.__call__(self, *args, **kwargs)

    celery.Task = ContextTask
