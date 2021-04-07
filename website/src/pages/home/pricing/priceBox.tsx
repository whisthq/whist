import React from "react"
import { Col } from "react-bootstrap"
import { FaCheck } from "react-icons/fa"

import "./priceBox.css"

export const PriceBox = (props: {
    name: string
    subText: string
    price: number
    details: string[]
    background: string
    textColor: string
}) => {
    /*
        Component that shows a plan tier as a checkable box
 
        Arguments:
            name (string): e.g. Starter, Pro
            subText (string): e.g. 7 day free trial
            price (number): Price in dollars
            details (string[]): e.g. ["Unlimited access", "24/7 support"]
            background (string): e.g. #FFFFFF
            textColor (string): e.g. #000000
    */

    const { name, subText, price, details, background, textColor } = props

    return (
        <Col
            className="px-12 py-8 rounded"
            style={{
                backgroundColor: `${background}`,
            }}
        >
            <div
                style={{
                    display: "flex",
                    justifyContent: "space-between",
                    color: `${textColor}`,
                }}
            >
                <div className="font-medium text-3xl mb-2">{name}</div>
            </div>
            <div
                className="text-sm text-left"
                style={{
                    color: `${textColor}`,
                }}
            >
                {subText}
            </div>
            <div
                className="priceBox-priceContainer"
                style={{ color: `${textColor}` }}
            >
                <div className="priceBox-dollarSign">$</div>
                <div className="priceBox-price">{price}</div>
                <div className="priceBox-mo">/ mo</div>
            </div>
            {details &&
                details.map((detail: string) => (
                    <div
                        style={{
                            display: "flex",
                            marginBottom: 10,
                            textAlign: "left",
                        }}
                        key={detail}
                    >
                        <FaCheck
                            style={{
                                fontSize: 12,
                                marginRight: 15,
                                color: `${textColor}`,
                            }}
                        />
                        <div
                            className="text-sm relative bottom-1"
                            style={{
                                color: `${textColor}`,
                                textAlign: "left",
                            }}
                        >
                            {detail}
                        </div>
                    </div>
                ))}
        </Col>
    )
}

export default PriceBox
