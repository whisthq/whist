const { StripeClient } = require('@fractal/core-ts')

module.exports = async function(user, context, cb) {
  const client = new StripeClient(context.webtask.secrets.STRIPE_KEY);
  const priceId = context.webtask.secrets.STRIPE_PRICE_ID;
  
  client.registerWithTrial(
    priceId,
    7,
    user.email,
    user.phoneNumber,
    user.id,
    user.username
  );
};
