def init_celery(celery, app):
    celery.conf.update(task_track_started = True, accept_content = ['pickle', 'json'],
    	result_accept_content = ['json'])
    TaskBase = celery.Task
    class ContextTask(TaskBase):
        def __call__(self, *args, **kwargs):
            with app.app_context():
                return TaskBase.__call__(self, *args, **kwargs)
    celery.Task = ContextTask
