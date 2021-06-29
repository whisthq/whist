.. responsibilities.rst
   An description of each of the Fractal Webserver's main responsibilities.

Webserver Responsibilities
===========================

The Fractal Webserver performs three main functions:

* Controls which users have access to which compute resources
* Makes sure we have enough compute for the userload we're seeing, by starting and stopping cloud instances
* Allows users to initiate application streams -- i.e. to start using Fractal


Access Control
--------------

For each access token that the Fractal Webserver receives, it must verify that the authenticated user is authorized to access
the resources to which they are requesting access. For example, some Fractal Webserver resources are only accessible to 
Fractal subscribers, while others are only accessible to Fractal administrators. Access tokens are JWTs issued by an Auth0 
tenant that takes care of user management and authentication. Each time the Fractal Webserver receives one of these access 
tokens, it uses the Auth0 tenant's public signing key to validate the token's signature. Validating the signature allows the 
Fractal Webserver to trust that the token was really issued by the Auth0 tenant. Once the signature has been validated, the 
Webserver can use the data contained in the token's payload to determine whether or not the authenticated is authorized to 
access the resource to which they are requesting access. In order to enforce access control for resources that are only 
accessible to Fractal subscribers, the Fractal Webserver reads the the authenticated user's Stripe-issued customer ID from 
the token's payload and uses it to query Stripe's API for the authenticated user's Fractal subscription status. If the 
subscription is in one of the "active" or "trialing" states, they are granted access to the resource. In order to enforce 
access control for resources that are only accessible to Fractal administrators, the Webserver reads from the token's payload 
the API scopes that the user is authorized to access. If one of those scopes is "admin", then the user is allowed to access 
the resource.


Compute Scaling
---------------

The Fractal Webserver continuously keeps track of the amount of load on our system, both in terms of the number of active 
users and their actual activity level. Based on this, it starts and stops cloud instances dynamically in order to make sure 
everyone who wants a session can have one, ideally without any wait.


Application Streaming
---------------------

By far the most significant and complicated of the Fractal Webserver's responsibilities is that of allocating Fractal 
mandelboxes to users. When the Webserver receives a request to start streaming an application, the web server must quickly 
procure connection information for a running instance of the Fractal Protocol Server and return it to the the Fractal 
Desktop application that requested the streaming session.


Starting Streams
^^^^^^^^^^^^^^^^

At a high level, when the Fractal Webserver receives a request to allocate a Fractal mandelbox to a user, from which an
application stream will begin. It does the following in order:

1. Check for any idle Fractal mandelbox that may have been created in advance in order to minimize the amount of time taken 
to return an application stream connection information to the client. If any idle Fractal mandelbox exist, choose one, 
return its connection information to the client, and skip to step 3.
2. Start a new Fractal mandelbox on a cloud instance with availability, and return its connection information to the user.
3. Scale the pool of idle Fractal mandelboxes based on current idle Fractal mandelboxes supply and application stream 
demand metrics, creating new cloud instances as necessary.

In order to reduce the amount of time it takes for the Fractal Webserver to associate an instance of the Fractal client with 
an application stream, we "cache," as it were, some information about the cloud resources that are provisioned and available 
to serve Fractal mandelboxes. Although it is redundant for us to take it upon ourselves to store some of this information 
because we could just as easily obtain it with a query to AWS, in practice it is advantageous to do so because querying 
our own database is quicker than querying AWS.


Ending Streams
^^^^^^^^^^^^^^

When a user closes a streamed application at the end of their streaming session, the Fractal mandelbox running this
application notifies the Fractal Webserver that the user has disconnected. At this point, the web server instructs AWS to 
delete the mandelbox. All data except for any application-specific configuration vanishes. In other words, every streamed 
application (i.e. Fractal mandelbox) has an ephemeral hard drive. All data that is persisted between streaming sessions is 
encrypted with a user-specific encryption key and uploaded to in AWS S3 before the Fractal mandelbox destroys itself. The 
encryption key is known only to the end-user, so neither AWS nor Fractal employees are able to decrypt and read the users' data.
