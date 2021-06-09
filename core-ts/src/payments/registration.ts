import { StripeClient } from './'

/*
 * Registers a new customer and grants them a trial
 * @param this - Stripe Client that owns this function
 * @param priceId - ID of Stripe price object
 * @param trialLength - Length of the trial in days
 * @param email - Customer email
 * @param phone - Customer phone number
 * @param id - Customer userID
 * @param username  - Customer username
 * @return An object representing a Stripe customer
 */
export async function registerWithTrial(
  this: StripeClient,
  args: {
    priceId: string,
    trialLength: number,
    email: string,
    phone: string,
    id: string,
    username: string
  }
) {
  const { email, phone, id, username, trialLength, priceId } = args;
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

  return customer;
}
