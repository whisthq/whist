import React from "react"
import {
    FaArrowLeft,
    FaArrowRight,
    FaRedo,
    FaEllipsisV,
    FaPlus,
} from "react-icons/fa"

export const ChromeBackground = () => {
    return (
        <>
            <div
                style={{
                    position: "absolute",
                    left: 0,
                    top: -1,
                    width: "100%",
                    height: 35,
                    background: "#dfe1e6",
                }}
            ></div>
            <div style={{ position: "relative", top: 38 }}>
                <div
                    style={{
                        position: "absolute",
                        background: "white",
                        left: 10,
                        top: -36,
                        width: 250,
                        height: 35,
                        borderRadius: "8px 8px 0px 0px",
                    }}
                ></div>
                <FaPlus
                    style={{
                        position: "absolute",
                        top: -26,
                        left: 280,
                        fontSize: 12.5,
                        color: "#888888",
                    }}
                />
                <div
                    style={{
                        background: "white",
                        padding: "0px 20px 4px 20px",
                        display: "flex",
                    }}
                >
                    <div>
                        <FaArrowLeft
                            style={{
                                fontSize: 15,
                                position: "relative",
                                marginRight: 20,
                                color: "#333333",
                                top: 8,
                            }}
                        />
                    </div>
                    <div>
                        <FaArrowRight
                            style={{
                                fontSize: 15,
                                position: "relative",
                                marginRight: 20,
                                color: "#eeeeee",
                                top: 8,
                            }}
                        />
                    </div>
                    <div>
                        <FaRedo
                            style={{
                                fontSize: 12,
                                position: "relative",
                                marginRight: 20,
                                color: "#333333",
                                top: 7,
                            }}
                        />
                    </div>
                    <div
                        style={{
                            borderRadius: 15,
                            height: 30,
                            background: "#f3f3f3",
                            flexGrow: 1,
                        }}
                    ></div>
                    <div>
                        <FaEllipsisV
                            style={{
                                fontSize: 13,
                                position: "relative",
                                paddingLeft: 20,
                                color: "#777777",
                                top: 8,
                            }}
                        />
                    </div>
                </div>
            </div>
        </>
    )
}

export default ChromeBackground
