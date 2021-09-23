.. stripe.rst
   This page of the Webserver documentation details the way we have chosen
   to integrate Stripe's billing services into our backend.

Stripe Integration
==================

Configuration
-------------

The following configuration variables affect the Webserver's ability to
integrate with Stripe correctly:

* :attr:`~app.config.DeploymentConfig.STRIPE_CUSTOMER_ID_CLAIM`
* :attr:`~app.config.DeploymentConfig.STRIPE_PRICE_ID`
* :attr:`~app.config.DeploymentConfig.STRIPE_SECRET`
* :attr:`~app.config.DeploymentConfig.STRIPE_SUBSCRIPTION_STATUS_CLAIM`

It is also important that the same
:attr:`~app.config.DeploymentConfig.STRIPE_PRICE_ID` and
:attr:`~app.config.DeploymentConfig.STRIPE_SECRET` values that the Webserver
uses are stored in the appropriate Auth0 tenant's `Rules configuration`_ as
``STRIPE_PRICE_ID`` and ``STRIPE_API_KEY`` respectively.

.. image:: /images/rules.png


Subscription status
-------------------

Part of enforcing access control is denying service to users who lack active
Fractal subscriptions. The :func:`payments.payment_required` decorator
implements this behavior. Each time the Webserver receives a request for a
resource that is only available to paying users, the
:func:`payments.payment_required` decorator checks the authenticated user's
subscription status before deciding whether or not to allow the request to be
processed as usual.

As is mentioned in the section about :ref:`custom claims`, each access token's
payload includes includes a ``https://api.fractal.co/subscription_status``
claim whose value is a string representing the authorized user's subscription
status. More specifically, the value of the
``https://api.fractal.co/subscription_status`` claim is the value of the
``status`` attribute of the most recent non-terminal Stripe
`Subscription object`_ associated with Stripe Customer record associated with
the user's Fractal account, or ``null`` if all of a user's subscriptions are in
terminal states. Let's break that down:

1. Also mentioned in the section about :ref:`custom claims` is the
   ``https://api.fractal.co/stripe_customer_id`` claim. This claim contains the
   ID of the Stripe customer record associated with the authorized user's
   Fractal account. The customer record associated with the authorized user's
   is created the first time the user logs into Fractal.
2. When a Fractal user buys a Fractal subscription, a Subscription object
   representing their subscription is associated with the Customer record
   associated with their account. An invariant that we don't necessarily
   enforce, but we hope holds true is that each Fractal user is only paying for
   one subscription at a time (and so has only one Subscription object
   associated with their Customer record. Nevertheless, we disregard all
   Subscription objects associated with a user's Customer record other than the
   most recent one when computing the value of the
   ``https://api.fractal.co/subscription_status`` claim to be on the safe side.
3. Each subscription can be in a variety of "terminal" or "non-terminal" states.
   To be in a terminal state is to be un-renewable. There is no action a user or
   developer can take (e.g. updating payment information) to move a
   subscription that is in a terminal state into a non-terminal state. A
   subscription enters a terminal state when it is canceled either manually or
   automatically (e.g. due to a lack of payment). To be in a non-terminal state
   is for a subscription to be active or inactive but renewable. Users and
   developers alike can take action to keep a subscription in a non-terminal
   state (e.g. by updating payment information to cause subsequent payment
   attempts to succeed after a failed attempt) or move it into a terminal state
   (e.g. by canceling it). We can determine whether a subscription is in a
   "terminal" or "non-terminal" state by examining the appropriate Subscription
   object's ``status`` attribute. A subscription is in a non-terminal state if
   and only if the value of its Subscription object's ``status`` attribute is
   one of ``active``, ``past_due``, ``incomplete``, or ``trialing``. Otherwise
   (i.e. if and only if the value of the Subscription object's ``status``
   attribute is one of ``unpaid``, ``canceled``, or ``incomplete_expired``), the
   subscription is in a terminal state.
4. If and only if all of the authorized users are in terminal states, the value
   of the ``https://api.fractal.co/subscription_status`` claim is ``null``.
   This is due to the fact that once a subscription enters a terminal state, it
   may as well not exist, but as long as it is in a non-terminal state, it may
   be worth knowing the exact state in which it is.

The :func:`payments.payment_required` decorator examines the value of the
access token's ``https://api.fractal.co/subscription_status`` claim and grants
access to only those users whose subscriptions are ``active`` or ``trialing``.


.. _payment portal:

Payment portal
--------------

`Stripe Checkout Sessions`_ and the `Stripe Customer Portal`_ eliminate the
necessity of implementing our own payment and billing UI. Instead, when a user
would like to manage their subscription or billing information, the desktop app
calls the ``/payment_portal_url`` endpoint on the Webserver to obtain the URL
of either a Stripe Checkout or Customer Portal session. Opening the URL brings
up a Stripe-hosted form that allows the user to sign up for a new subscription
or manage their existing subscription and billing information. Fractal
developers can customize the form's appearance from the `Stripe dashboard`_.

The Webserver determines whether or not a Checkout or Customer Portal session
should be created based on the value of the authenticated user's access token's
``https://api.fractal.co/subscription_status`` claim. If its value indicates
that the user has a subscription in a non-terminal state (one of ``active``,
``past_due``, ``incomplete``, or ``trialing``), then the Webserver creates a
Customer Portal session and returns its URL to the client so the user can
update their existing subscription and billing information. If its value
indicates that all of the user's subscriptions are in teriminal states
(``unpaid``, ``canceled``, or ``incomplete_expired``), the Webserver creates a
Checkout session and returns its URL to the client os the user can purchase a
new subscription.


.. _stripe optimization:

Optimizing the Stripe integration
---------------------------------

We have decided to define the custom
``https://api.fractal.co/stripe_customer_id`` and
``https://api.fractal.co/subscription_status`` access token claims in order to
share information between Stripe and the rest of our backend as efficiently as
possible.

Not mentioned in the `payment portal`_ section is that in order to create a
Checkout or Customer Portal session for an authenticated user, the Webserver
needs to know the ID of the Stripe Customer associated with that user. The
Webserver could obtain the necessary information by querying Stripe or Auth0's
API, but round trips are time-consuming. By claiming the ID of the Customer
record associated a user's account in each of their access tokens, we eliminate
at least one round trip that must be made to Stripe or Auth0's API.

Each time a user sends a request to an endpoint that is protected by the
:func:`payments.payment_required` decorator, the Webserver needs to look up the
authenticated user's subscription status in order to enforce access control. It
would be possible to extract the ID of the Customer record associated with the
user's account and query the Stripe API for the subscription status, but such
queries have been observed to take as much as a quarter of a second, which is an
unacceptably long time. Furthermore, under high load, we risk exceeding our
Stripe API rate limits.

The values of each of these claims is re-computed each time a new access token
is issued. The only downside of re-computing the value of the
``https://api.fractal.co/subscription_status`` claim is that the subscription
status claimed in the user's current access token may fall out of sync with the
user's actual subscription status. In theory, an extremely meticulous and
technical user could continue to use a subscription for one extra day after it
expires. However, we don't believe that this worst-case scenario is worth
addressing, especially considering the advantages and ease of adoption of
caching a user's subscription status in their access token.


.. _Rules configuration: https://auth0.com/docs/rules/configuration
.. _Subscription object: https://stripe.com/docs/api/subscriptions/object#subscription_object-status
.. _Stripe Checkout Sessions: https://stripe.com/docs/payments/checkout
.. _Stripe Customer Portal: https://stripe.com/docs/billing/subscriptions/customer-portal
.. _Stripe dashboard: https://dashboard.stripe.com/settings/billing/portal
