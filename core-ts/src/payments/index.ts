import Stripe from "stripe";

import { registerWithTrial } from "./registration"

export class StripeClient {
    _key: string;
    _stripe: Stripe;

    constructor(key: string) {    
        this._key = key;
        this._stripe = new Stripe(key, {
            apiVersion: '2020-08-27',
        })
    }

    registerWithTrial = registerWithTrial.bind(this);
}
