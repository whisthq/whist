import React from "react"
import { Button } from "react-bootstrap"

import "styles/landing.css"

function BottomView(props: any) {
    const scrollToTop = () => {
        window.scrollTo({
            top: 0,
            behavior: "smooth",
        })
    }

    return (
        <div style={{ padding: 30 }}>
            <div
                style={{
                    borderRadius: 5,
                    boxShadow: "0px 4px 30px rgba(0, 0, 0, 0.1)",
                    padding: "60px 100px",
                    background: "white",
                }}
            >
                <div
                    style={{
                        maxWidth: 650,
                        margin: "auto",
                        textAlign: "left",
                    }}
                >
                    <h2
                        style={{
                            fontSize: 40,
                            fontWeight: "bold",
                            lineHeight: 1.4,
                        }}
                    >
                        Give my applications superpowers.
                    </h2>
                    <p style={{ marginTop: 20 }}>
                        Run the most demanding applications from my device using
                        10x less RAM and processing power, all on gigabyte
                        datacenter Internet.
                    </p>
                    <Button className="access-button" onClick={scrollToTop}>
                        REQUEST ACCESS
                    </Button>
                </div>
            </div>
        </div>
    )
}

export default BottomView
