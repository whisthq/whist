This folder contains code to profile celery task execution. The main questions are:

1. What "type" of pooling should we use? The options are prefork (uses `fork` syscall)
   and coroutines (userspace threads; `gevent` library is used by celery). This link
   has a good description of the differences: https://www.distributedpython.com/2018/10/26/celery-execution-pool/
2. What "type" of tasks are we running on our webserver? Specifically, are our tasks IO
   or CPU bound? We suspect IO because most of the time is spent doing blocking API calls to AWS.

Results and more detail can be found [here](https://docs.google.com/spreadsheets/d/1ykcQvhhCdNhCl0IvZ7LQGtpNFIPMi8BS1Lls3xmZrtk/edit?usp=sharing).

To do this yourself, on a multi-core laptop run each of the following on a unique terminal. You should close out of as many applications as possible to not waste any CPU resources. If you choose to run Redis anywhere other than `redis://localhost:6379` please set the environment variable `REDIS_URL` to the appropriate value.

```bash
$ docker run -p 6379:6379 --name redis redis
$ # export REDIS_URL=...
$ celery --app tasks worker --pool prefork --concurrency NUM_WORKERS
$ python profiler.py
```

Set NUM_WORKERS to whatever level of parallelism you want (use `nproc` to see how many cores you have).
By default, `profiler.py` runs 100 tasks with 10% of a task's time spent using CPU
(the other 90% is blocking IO, aka sleeping). Each task takes 100ms and results are polled at
0.1 second intervals. The following (optional), hopefully self-explanatory flags can be passed to try different tasks:

```
--num_tasks
--frac_cpu
--task_time_ms
--poll_freq
```

If you want to try the `gevent` pooling option, kill celery then run:

```
celery --app tasks worker --pool gevent --concurrency NUM_WORKERS
python profiler.py
```

To understand the CPU/IO profile of our webserver, use `time.time()` and `time.process_time()` inside a function of choice. The former checks wall time, the latter checks process CPU time. For example, you could profile a function by adding the following near the start and end of the function:

```
time_start = time.time()
cpu_start = time.process_time()

...
(main function body)
...

time_end = time.time()
cpu_end = time.process_time()

fractal_logger(f"TOTAL TIME: {time_end - time_start}, TOTAL CPU TIME: {cpu_end - cpu_start}")

return <function returns>
```

Then, you could use our testing framework to execute this function. See the webserver README on how to run tests. Note testing will actually report a higher CPU ratio than when deployed because the `_poll` function is mocked.

## Conclusions

We should use gevent as our pooling method. Our tasks are heavily IO-bound (for example, an old request handler was using CPU only 0.3% of the time). Profiling showed that gevent outperforms prefork when the fraction of time spent doing CPU work is roughly under 15%. Also, we are using NUM_WORKERS = 50 in the docker-compose and production because that outperformed other options under the following parameters:

```
num_tasks: 500
frac_cpu: 0.1
task_time_ms: 100
poll_freq: 0.1
```
