.. architecture.rst
   A description of the different components of the Fractal Webserver and how
   they interact with one another.

Webserver Architecture
=======================

When deployed, the so-called "Fractal Webserver" is not so much a web server as it is a group of three closely-related 
services:

* The HTTP server itself
* An asynchronous task queue
* A worker pool

Only the HTTP server is accessible to the public. The HTTP server and the worker pool, using the asynchronous task queue as 
their primary communication channel, work together to process all incoming requests. Any time the Fractal Webserver receives 
a request that would take more than a few milliseconds to process, it adds the processing of that request to the asynchronous 
task queue and immediately returns a unique ID identifying the entry in the queue to the caller (usually the Fractal Desktop 
Applications). The caller can use this unique ID to poll for the tasks' status while it waits in the queue to be picked up 
by a worker and while the task is being processed. When the tasks' processing is complete, the caller can use the unique ID 
to retrieve the result.

We have chosen to pair a worker pool with our HTTP server in this way to increase the worst-case rate at which our webserver 
can process requests. Since the HTTP server processes each request in its own thread and it is only practical to spawn so 
many threads for request processing purposes, any thread that spends too much time processing the same request effectively 
decreases the HTTP server's overall request processing capacity.

Suppose there was no worker pool and the HTTP server had to handle all computation on its own. Suppose again that it takes 
10 seconds to process a request for :code:`x`. Let's say that the HTTP server is allowed to spawn up to 50 request processing 
threads. If 50 users send a requests for :code:`x` all within 10 seconds of one another, the HTTP server will become 
completely unable to process any other requests until at least one of the threads becomes available again. In the worst case, 
in which all 50 users sent the request at the same time, it could take 10 seconds before the HTTP server is able to process 
any other requests!

Since the worker pool handles all time-intensive computation, no HTTP server thread is blocked for more than a few 
milliseconds. It would take a massive number of requests to block the webserver even for one second.


HTTP Server
-----------

Fractal's HTTP server is implemented using the Flask web framework for Python.


Redis Store
-----------

As stated above, the Redis store is the primary communication channel between the HTTP server and the Worker pool. Generally, 
it stores state that needs to be shared across multiple HTTP server or worker processes. We can approximately classify all 
pieces of state in the Redis store as either server state or task state, where a "task" is a function that the HTTP server 
wants the worker pool to compute asynchronously on some input.


Server State
^^^^^^^^^^^^

During deployment, Heroku may run Fractal's HTTP server in multiple concurrent processes. This allows Heroku to load-balance 
requests across all available server processes, improving performance at scale.

The fact that each process has its own memory makes it challenging to share information between HTTP server instances; we 
can't simply synchronize access to a global variable because each process will have an independent copy of the variable. 
Instead, we synchronize access to a global Redis store.

For example, there is a key in our Redis store that determines whether or not HTTP server processes are allowed to accept 
incoming requests. All processes running the HTTP server read the same key from the Redis instance as though it were a global 
variable.


Task State
^^^^^^^^^^

As mentioned above, a task is a function that the HTTP server wants the worker pool to compute asynchronously on some input. 
They appear in the Redis store for two reasons. First, they appear as pairs of code and input when the HTTP server adds them 
to the queue. They are moved to a different data structure in the Redis store when they are picked up by a worker. They 
remain here until they have been processed completely.


Worker Pool
-----------

The Celery package for Python provides us with a platform upon which to implement our worker pool's functionality. Celery 
defines the following roles:

* Clients add tasks to task queues and may wait for results to become available
* Brokers implement task queues
* Backends store the results of processed tasks
* Workers process tasks pending in task queues

In Fractal's case, our HTTP server is a Celery client. It enqueues long-running functions in our task queue to avoid blocking 
its own threads. Our Redis store acts both as a task broker and a result backend, meaning that it stores not only metadata 
about tasks that are yet to be processed, but also the results of tasks that have already been handled by workers. All that's
left is the Celery worker. It is launched with the :code:`celery worker` command.

An interesting thing to note is that we've decided to attach a worker pool to our deployment not because our webserver is 
responsible for large amounts of intense computation, but rather because it spends a lot of time polling external services. 
In other words, our long-running functions spend most of their time sleeping and making simple network requests.


Scaling the Worker Pool
^^^^^^^^^^^^^^^^^^^^^^^

When deployed, we use a Heroku add-on service called HireFire to automatically scale our Celery workers. Based on the number 
of pending tasks in the task queue and the number of Celery "dynos" running on our Heroku app, HireFire may decide to scale 
that number up or down to match compute needs.
