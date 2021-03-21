"""
A central store of all dimensions keys to support easier auditability of potential
metric dimensions being recorded.
"""

ENVIRONMENT = "environment"
HOST_SERVER = "host_server"
APP_GIT_COMMIT = "app_git_commit"
APP_NAME = "app_name"

CELERY_WORKER_ID = "celery_worker_id"
CELERY_TASK_NAME = "celery_task_name"
CELERY_TASK_ID = "celery_task_id"
CELERY_TASK_STATUS = "celery_task_status"

WEB_REQ_ENDPOINT = "web_req_endpoint"
WEB_REQ_STATUS_CODE = "web_req_status_code"
