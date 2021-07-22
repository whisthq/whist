/* add-customer-id-claim.js

   Add the authenticated user's Stripe customer ID as the value of the custom
   https://api.fractal.co/stripe_customer_id access token claim. By adding the
   user's customer ID as a claim in the access token, the Webserver can read
   the authenticated user's customer ID from the access token once it verifies
   the token's signature.
 */

function addCustomerIdClaim(user, context, callback) {
  const app_metadata = user.app_metadata || {};
  const stripe_customer_id = app_metadata.stripe_customer_id || null;

  context.accessToken[
    "https://api.fractal.co/stripe_customer_id"
  ] = stripe_customer_id;

  return callback(null, user, context);
}
