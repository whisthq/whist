#Payments

The spec as this interface was being written: https://www.notion.so/tryfractal/The-Fractal-Payments-Interface-eaa14ff337f94dd68ce9f812f748217f

This folder in the webserver repo contains all the code involving user payments (all except the tests).

```
.
├── README.md
├── __init__.py
├── stripe_blueprint.py <- contains HTTP endpoints for the payments interface
├── stripe_client.py <- a Stripe client class, to consolidate our stripe actions into one file
└── stripe_helpers.py <- helper functions that help process the information coming through the HTTP endpoints
```

Note that each file has a very specific role:

`stripe_blueprint.py` is responsible for processing HTTP requests that come through, and returns `400 BAD REQUEST` is the body is incorrectly formed.

`stripe_helpers.py` is responsible for performing any business logic on the Fractal side, and other than a malformed body, responsible for formatting the response that the interface returns.

`stripe_client.py` is the only part of our interface that interacts with the Stripe API, and any exceptions that are thrown here we `raise`. This is so that they are passed along to `stripe_helpers.py`, and can be dealt with there instead.
