import AmericanExpress from "assets/cards/americanExpress.svg"
import DinersClub from "assets/cards/dinersClub.svg"
import Discover from "assets/cards/discover.svg"
import JCB from "assets/cards/jcb.svg"
import MasterCard from "assets/cards/masterCard.svg"
import UnionPay from "assets/cards/unionPay.svg"
import Visa from "assets/cards/visa.svg"

type cardsType = { [key: string]: any }

export const cards: cardsType = {
    "American Express": AmericanExpress,
    "Diners Club": DinersClub,
    Discover: Discover,
    JCB: JCB,
    MasterCard: MasterCard,
    UnionPay: UnionPay,
    Visa: Visa,
}
