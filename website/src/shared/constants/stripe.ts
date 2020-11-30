import AmericanExpress from "assets/cards/americanExpress.svg"
import DinersClub from "assets/cards/dinersClub.svg"
import Discover from "assets/cards/discover.svg"
import JCB from "assets/cards/jcb.svg"
import MasterCard from "assets/cards/masterCard.svg"
import UnionPay from "assets/cards/unionPay.svg"
import Visa from "assets/cards/visa.svg"

export const cards: { [key: string]: any } = {
    "American Express": AmericanExpress,
    "Diners Club": DinersClub,
    Discover: Discover,
    JCB: JCB,
    MasterCard: MasterCard,
    UnionPay: UnionPay,
    Visa: Visa,
}

export const STRIPE_OPTIONS = {
    fonts: [
        {
            cssSrc:
                "https://fonts.googleapis.com/css2?family=Maven+Pro&display=swap",
        },
    ],
}

export const TAX_RATES: { [key: string]: number } = {
    AL: 4,
    AK: 0,
    AZ: 5.6,
    AR: 6.5,
    CA: 7.25,
    CO: 2.9,
    CT: 6.35,
    DE: 0,
    DC: 5.75,
    FL: 6,
    GA: 4,
    ID: 6,
    IL: 6.25,
    IN: 7,
    IA: 6,
    KS: 6.5,
    KY: 6,
    LA: 4.45,
    ME: 5.5,
    MD: 6,
    MA: 6.25,
    MI: 6,
    MN: 6.875,
    MS: 7,
    MO: 4.225,
    MT: 0,
    NE: 5.5,
    NV: 6.85,
    NH: 0,
    NJ: 6.625,
    NM: 5.125,
    NY: 4,
    NC: 4.75,
    ND: 5,
    OH: 5.75,
    OK: 4.5,
    OR: 0,
    PA: 6,
    RI: 7,
    SC: 6,
    SD: 5,
    TN: 7,
    TX: 6.25,
    UT: 5.95,
    VT: 6,
    VA: 5.3,
    WA: 6.5,
    WV: 6,
    WI: 5,
    WY: 4,
}
