.. authentication.rst
   A description of how we integrate with Auth0 to enforce access control on
   our Webserver endpoints.

Authentication
==============

The Webserver is an OAuth 2.0 specification compliant resource server (see
`Section 1.1 of RFC 6479`_).

Most requests sent to the Webserver must be authorized. An authorized request
must contain an valid access token in its HTTP headers::

   GET / HTTP/1.1
   Authorization: Bearer <token>
   ...

Access tokens are RS256-signed `JSON Web Tokens <https://jwt.io/introduction>`_
issued by one of our Auth0 tenants. Each Auth0 tenant is an OAuth 2.0
specification compliant authorization server.


Configuration
-------------

The Webserver must be configured to communicate with the deployment-specific
Auth0 tenant that protects access to it. The following configuration variables
must be set correctly to ensure that the Webserver is able to communicate with
the correct tenant.

* :attr:`~app.config.DeploymentConfig.AUTH0_DOMAIN`
* :attr:`~app.config.DeploymentConfig.AUTH0_WEBSERVER_CLIENT_ID`
* :attr:`~app.config.DeploymentConfig.AUTH0_WEBSERVER_CLIENT_SECRET`


Token validation
----------------

Upon receipt of an HTTP request containing an access token in its
``Authorization`` header, the Webserver attempts to determine whether or not
the token is valid. In order to be considered valid, a token's standard time-\
based claims (``iat`` and ``exp``) must be valid, the issuer must be the
deployment-specific Auth0 tenant (e.g. ``https://fractal-prod.us.auth0.com/``
in production), ``https://api.fractal.co`` must be one of the token's
audiences, and the signature must be verified.

Validating the standard time-based, issuer, and audience claims can easily be
done locally. Since tokens are signed using the asymmetric RS256 signing
algorithm, the authorization server's public key is required to verify their
signatures. A JSON web key set containing public keys can be downloaded from
each tenant's ``/.well-known/jwks.json`` endpoint. For example, our production
tenant's public keys are available at
https://fractal-prod.us.auth0.com/.well-known/jwks.json. Once the public keys
have been downloaded, signature verification can be done locally as well.


.. _custom claims:

Custom claims
-------------

Don't be alarmed to find two custom claims in each access token's payload. The
``https://api.fractal.co/stripe_customer_id`` claim contains the ID of the
Stripe customer record that is associated with the authorized user's Fractal
account. The ``https://api.fractal.co/subscription_status`` claim contains a
string indicating the user's subscription status. Note that, per Auth0's
guidelines, each of these claims is `namespaced`_ with the namespace URL
``https://api.fractal.co/``. Read :ref:`stripe optimization` for an explanation
of why we have chosen to define these two custom claims.


.. _Section 1.1 of RFC 6479: https://datatracker.ietf.org/doc/html/rfc6749#section-1.1
.. _namespaced: https://auth0.com/docs/tokens/create-namespaced-custom-claims
