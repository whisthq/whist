import { StripeClient } from './'

export async function registerWithTrial(
  this: StripeClient,
  priceId: string,
  trialLength: number,
  email: string,
  phone: string,
  id: string,
  username: string
) {
  const customer = await this._stripe.customers.create({
    email,
    phone,
    metadata: {
      id,
      username
    }
  });

  await this._stripe.subscriptions.create({
    customer: customer.id,
    trial_period_days: trialLength,
    items: [
      {price: priceId}
    ]
  });
}
