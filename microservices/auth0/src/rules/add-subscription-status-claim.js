/* add-subscription-status-claim.js

   Add the status of the authenticated user's Fractal Stripe subscription as the
   value of the custom https://api.fractal.co/subscription_status access token
   claim.

   The subscription status is recomputed every time a new access token is
   issued. In a worst case scenario, it is possible for the user's subscription
   to expire before their access token does. This would allow them to continue
   using Fractal for up to one access token lifetime, which is twenty-four
   hours.
 */

function addSubscriptionStatusClaim(user, context, callback) {
  const app_metadata = user.app_metadata || {}
  const stripe_customer_id = app_metadata.stripe_customer_id || null
  const stripe = require("stripe@8.126.0")(configuration.STRIPE_API_KEY)

  if (app_metadata.stripe_customer_id) {
    stripe.subscriptions
      .list({
        customer: stripe_customer_id,
      })
      .then((subscriptions) => {
        // Get the subscription that was created most recently by sorting the
        // array of subscriptions in ascending order by creation date and then
        // choosing the last element in the sorted array. If the list is empty,
        // pop() will return undefined.
        const subscription = subscriptions.data
          .sort((left, right) => left.created - right.created)
          .pop()

        context.accessToken[
          "https://api.fractal.co/subscription_status"
        ] = subscription ? subscription.status : null

        return callback(null, user, context)
      }, callback)
  } else return callback(new Error("Missing Stripe customer ID"), user, context)
}
