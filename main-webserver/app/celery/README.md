##Celery README

###Basics of celery
A celery task is one way to asynchronously execute some code — this is useful for e.g. code that pings an API or touches one of our databases so it doesn't block our server while it's running. By design, Heroku fails all API requests that take more than 30 seconds to execute, so celery tasks are necessary for long-running API calls. A high-level rule of thumb to follow is to outsource an endpoint to a celery task if the endpoint can be expected to take more than 5-10 seconds to run (most SQL operations take less than a second).

Celery tasks should never make their own API calls or direct SQL statements, instead wrapping those calls in one of our helpers. Helpers for APIs live in the helpers directory, and for SQL use fractalSQLCommit, fractalSQLUpdate, and fractalSQLDelete. Any exceptions raised by helpers should be directly reraised to aid debugging (that means that you should not wrap helpers in a try-catch block unless you anticipate some exception, and even then you should try to rewrite the code or the helper to detect that exception-causing condition without raising it, exceptions as control flow are an antipattern). For helpers that do not raise exceptions and instead have special error return values, check for those values and fail immediately after those helpers return a failing error code, again to aid debugging. All failures should both change the state of the celery worker and use fractalLog to log the error.

###Specifics of our celery tasks
Our two primary celery task classes are 'creation and deletion of AWS containers', in the aws_ecs files, and 'saving/deletion of logs to s3', in the aws_s3 files.
The final file, dummy, is used for manual testing of celery instances (and shouldn't really be needed). If you're trying to add additional steps to container spinup and already have client methods
corresponding to those steps, you'll put calls of those methods into the relevant celery tasks in the above files.
