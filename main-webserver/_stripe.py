"""Fractal's Stripe utility library.

A Stripe API key must be set before the functions in this utility library may be used.

Example usage:

    import stripe

    from _stripe import ensure_subscribed, PaymentRequired

    customer_id = "..."
    stripe.api_key = "..."

    try:
        ensure_subscribed(customer_id)
    except PaymentRequired:
        print(f"Customer '{customer_id}' has no active subscriptions.")
    else:
        print(f"Customer '{customer_id}' is a paying user.")

"""

import stripe


class PaymentRequired(Exception):
    """Raised by ensure_subscribed() when a user does not have an active subscription."""


def ensure_subscribed(customer_id: str) -> None:
    """Make sure the specified user has an active Fractal subscription.

    The user's subscription is considered valid iff it is paid or it is still in the trial period.

    Args:
        customer_id: The Stripe customer ID of the customer whose subscription should be checked.

    Returns:
        None

    Raises:
        PaymentRequired: The customer has no active subscriptions.
    """

    customer = stripe.Customer.retrieve(customer_id, expand=("subscriptions",))

    if not any(
        subscription["status"] in ("active", "trialing")
        for subscription in customer["subscriptions"]
    ):
        raise PaymentRequired
