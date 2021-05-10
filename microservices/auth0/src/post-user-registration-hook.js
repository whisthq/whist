const { StripeClient } = require('@fractal/core-ts')

const TRIAL_LENGTH_DAYS = 7

module.exports = async function(user, context, cb) {
  const client = new StripeClient(context.webtask.secrets.STRIPE_KEY);
  const priceId = context.webtask.secrets.STRIPE_PRICE_ID;
  
  // Create Stripe customer corresponding to the newly-registered user and grant them a free trial
  client.registerWithTrial(
    priceId,
    TRIAL_LENGTH_DAYS,
    user.email,
    user.phoneNumber,
    user.id,
    user.username
  );
};
