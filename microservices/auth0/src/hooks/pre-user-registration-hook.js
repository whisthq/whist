const { StripeClient } = require('@fractal/core-ts')

const TRIAL_LENGTH_DAYS = 7

module.exports = function(user, context, cb) {
  const client = new StripeClient(context.webtask.secrets.STRIPE_KEY);
  const priceId = context.webtask.secrets.STRIPE_PRICE_ID;

  const { email, phoneNumber, id, username } = user;

  // Create Stripe customer corresponding to the newly-registered user and grant them a free trial
  client.registerWithTrial({
    priceId,
    trialLength: TRIAL_LENGTH_DAYS,
    email,
    phone: user.phoneNumber,
    id,
    username
  }).then((customer) => {
    cb(null, {
      "user": {
        "app_metadata": {
          "stripe_customer_id": customer.id
        }
      }
    })
  }, cb);
};
