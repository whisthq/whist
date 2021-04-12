.. responsibilities.rst
   An description of each of the Fractal web server's main responsibilities.

Web server responsibilities
===========================

The Fractal appears to have many different responsibilities because it does so much, but none of its responsibilities are particularly well-defined. In fact, we can think about the web server having just three main responsibilities:

* user management,
* access control, and
* and streaming session management.


User management
---------------

The user management responsibilities of the web server are pretty much limited to account registration. The web server is responsible for creating rows for new users in Fractal's database.


Access control
--------------

For each user that the web server has registered, the web server must be able to control that user's access to resources that are available via the web server. For example, the web server has to ensure that only users with an active Fractal subscription receive service.


Streaming session management
----------------------------

By far the most significant and complicated of the web server's responsibilities is that of allocating application streams to users. Upon receipt of a request to start streaming an application, the web server must quickly procure connection information for a running instance of the protocol server and return it to the the desktop application that requested the streaming session. In order to fulfill this responsibility, the web server consumes information from Fractal's database and AWS account.


Starting streams
^^^^^^^^^^^^^^^^

At a high level, when the web server receives a request to allocate an application stream to a user, it does the following in order:

1. Check for any idle streaming sessions that may have been created in advance in order to minimize the amount of time taken to return application stream connection information to the client. If any idle streaming sessions exist, choose one, return its connection information to the client, and skip to step 3.
2. Start a new application stream and return its connection information to the user.
3. Scale the pool of idle streaming sessions based idle application stream supply and application stream demand metrics.

In order to reduce the amount of time it takes for the web server to associate an instance of the Fractal client with an application stream, we "cache," as it were, some information about the cloud resources that are provisioned and available to serve application streams. Although it is redundant for us to take it upon ourselves to store some of this information because we could just as easily obtain it with a query to AWS, it is theoretically advantageous to do so because querying our own database is quicker than querying AWS.

Another reason why it is useful, if not necessarily necessary, to cache some information about what cloud resources are available to serve application streams is that we have implemented our own scaling layer on top of what scaling AWS already provides. Specifically, there is code on the web server that both scales the number of ECS clusters we have made available to ourselves for the purposes of deploying containerized application streams and also load balances application streams across all of these clusters.


Ending streams
^^^^^^^^^^^^^^

When a user closes a streamed application at the end of their streaming session, the application container notifies the web server that the user has disconnected. At this point, the web server instructs AWS to delete the container. All data except for any application-specific configuration vanishes. Application-specific configuration is encrypted with a user-specific encryption key that is never stored in plaintext on Fractal's servers.
