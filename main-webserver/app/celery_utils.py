def init_celery(celery, app):
    """
    Adds some configs to Celery instance (`celery`) and
    adds flask request context to all celery tasks.
    Note specifically the serialization strategy now
    uses pickle instead of json, which binds us
    to python data types but means we can handle any python
    object.

    Args:
        celery: a Celery instance
        app: a Flask app
    """
    celery.conf.update(
        task_track_started=True,
        accept_content=["json", "pickle"],
        result_accept_content=["json", "pickle"],
        worker_hijack_root_logger=True,
        celery_redis_max_connections=40,
        task_serializer="pickle",
        result_serializer="pickle",
        event_serializer="pickle",
    )
    TaskBase = celery.Task  # pylint: disable=invalid-name

    class ContextTask(TaskBase):
        def __call__(self, *args, **kwargs):
            with app.app_context():
                return TaskBase.__call__(self, *args, **kwargs)

    celery.Task = ContextTask
