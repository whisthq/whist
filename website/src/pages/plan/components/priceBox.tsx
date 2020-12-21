import React from "react"

import "styles/plan.css"

const PriceBox = (props: any) => {
    const { name, subText, price, details, checked } = props

    return (
        <div
            className="priceBox-container"
            style={{
                boxShadow: checked ? "0px 4px 10px rgba(0, 0, 0, 0.1)" : "none",
                //  border: checked ? "solid 1px #93f1d9" : "solid .8px #cccccc",
            }}
        >
            <div style={{ display: "flex", justifyContent: "space-between" }}>
                <div className="priceBox-name">{name}</div>
                <div
                    className="priceBox-checkbox"
                    style={{
                        background: checked ? "#93f1d9" : "none",
                    }}
                />
            </div>
            <div className="priceBox-subText">{subText}</div>
            <div className="priceBox-priceContainer">
                <div className="priceBox-dollarSign">$</div>
                <div className="priceBox-price">{price}</div>
                <div className="priceBox-mo">/ mo</div>
            </div>
            <div className="priceBox-details">{details}</div>
        </div>
    )
}

export default PriceBox
