"""
A central store of all metric keys to support easier auditability of potential
metrics being recorded.
"""

CELERY__TASKS_PROCESSING__COUNT = "celery.tasks_processing.count"
CELERY__TASK_DURATION__MS = "celery.task_duration.ms"
CELERY__TASK_QUEUE_DURATION__MS = "celery.task_queue_duration.ms"
