# Payments

Take a look at [this spec](https://www.notion.so/tryfractal/The-Fractal-Payments-Interface-eaa14ff337f94dd68ce9f812f748217f) for our thoughts as this was being written. The main takeaway is that we don't want to have to maintain our own payments service, so we use Stripe to process payments. One of our focuses is to have Stripe do most of the work for us; for example, all subscription status information is stored on the Stripe side, and if we need that information we pull it from Stripe directly.

This folder in the webserver repo contains all the server-side code involving user payments (all except the tests). This mini-"library" for payments is called by the webserver (with the helper function `get_customer_status`) as well as in client-applications (to create the billing portal).

```
.
├── README.md
├── __init__.py <- Fractal's Stripe utility library
├── stripe_blueprint.py <- contains HTTP endpoints for the payments interface
└── stripe_helpers.py <- helper functions that help process the information coming through the HTTP endpoints
```

Note that each file has a very specific role:

`__init__.py` is responsible for authenticating HTTP requests that come through. It looks for and retrieves the user's customer id from the jwt token's metadata, and confirms that the Stripe customer associated with the customer id has or does not have access to our payments API.

`stripe_blueprint.py` is responsible for processing HTTP requests that come through, and returns `400 BAD REQUEST` is the body is incorrectly formed.

`stripe_helpers.py` is responsible for performing any business logic on the Fractal side. It interacts with the Stripe API, and is responsible for returning a correctly formatted response to `stripe_blueprint.py`.

## Stripe Access

The Stripe dashboard contains a lot of useful information and tools for developers. If you're working on the payments library, first make sure you're added to the Fractal account on Stripe (ask ming@fractal.co or phil@fractal.co for access). This'll give you access to the [dashboard](https://dashboard.stripe.com/dashboard), which you can then use to see Fractal customers, change prices/products, run things in test mode, etc.

## How To Contribute

Before making a pull request, follow the same guidelines as when making a pull request to the `webserver` subrepo.

## How To Test

To test, we have unit tests written in `tests/payment` that test our endpoints + outward facing helper functions. For documentation about these tests, go to `webserver/tests/README.md` and `webserver/README.md`.

Stripe also has a command line interface, which, when installed, can be used to do anything the library can do. For more information, click [here](https://stripe.com/docs/stripe-cli).
