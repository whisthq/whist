/* create-customer.js

   Each time a user logs in, determine whether or not a Stripe customer record
   is already associated with their account. If no such record exists, create
   one and start a free trial of Fractal.
 */

function createCustomer(user, context, callback) {
  const app_metadata = user.app_metadata || {}
  const stripe = require("stripe@8.126.0")(configuration.STRIPE_API_KEY)

  // Check whether or not there is already a Stripe customer associated with
  // this user.
  if (!app_metadata.stripe_customer_id) {
    // Create a new Stripe customer.
    stripe.customers
      .create({
        email: user.email,
        name: user.name,
        phone: user.phone_number,
      })
      .then((customer) => {
        // Enroll the new user in a free trial
        stripe.subscriptions
          .create({
            customer: customer.id,
            trial_period_days: 7,
            items: [{ price: configuration.STRIPE_PRICE_ID }],
          })
          .then((_subscription) => {
            // Save the new Stripe customer's customer ID.
            auth0.users
              .updateAppMetadata(user.user_id, {
                stripe_customer_id: customer.id,
              })
              .then(
                // Success! Pass the updated user object to the next rule.
                (updated_user) => callback(null, updated_user, context),
                callback
              )
          }, callback)
      }, callback)
  }
  // Nothing to do; there is already a Stripe customer associated with this
  // user.
  else callback(null, user, context)
}
