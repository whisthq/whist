import React from "react"
import { Row, Col } from "react-bootstrap"
import { Link } from "react-router-dom"

import PriceBox from "pages/homepage/products/chrome/components/pricing/components/priceBox/priceBox"
import { PLANS } from "shared/constants/stripe"
import { FractalPlan } from "shared/types/payment"

export const Pricing = (props: { dark: boolean }) => {
    /*
        Pricing section of chrome product page
 
        Arguments: 
            dark (boolean): True/false dark mode
    */

    const { dark } = props

    return (
        <div className="text-center mt-52 relative">
            <div className="flex justify-center text-5xl dark:text-gray-300">
                <div>
                    Try Fractal for{" "}
                    <span className="text-blue dark:text-mint">free</span>
                </div>
            </div>
            <div className="dark:text-gray-400 tracking-wider mt-4">
                No credit card required, free for seven days.
            </div>
            <Link to="/auth">
                <button
                    type="button"
                    className="text-gray hover:text-black rounded bg-transparent hover:bg-mint border border-black hover:border-mint dark:hover:border-mint dark:hover:text-black dark:border-white dark:text-white px-12 py-3 my-10 duration-500 tracking-wide"
                >
                    TRY NOW
                </button>
            </Link>
            <Row className="m-auto pt-8 max-w-screen-sm">
                <Col md={6} style={{ marginBottom: 25 }}>
                    <PriceBox
                        {...PLANS[FractalPlan.STARTER]}
                        background="#553de9"
                        textColor="white"
                    />
                </Col>
                <Col md={6} style={{ marginBottom: 25 }}>
                    <PriceBox
                        {...PLANS[FractalPlan.PRO]}
                        background={dark ? "#00FFA2" : "#f6f9ff"}
                        textColor="black"
                    />
                </Col>
            </Row>
        </div>
    )
}

export default Pricing
