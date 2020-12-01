import React from "react"
import { Col } from "react-bootstrap"
import { FaGlobeEurope, FaBook } from "react-icons/fa"

import styles from "styles/dashboard.css"

const LeftColumn = (props: {
    callback: (category: string) => void
    selectedCategory: string
}) => {
    const { callback, selectedCategory } = props

    return (
        <>
            <Col xs={1}>
                <div
                    className={styles.categoryIcon}
                    style={{
                        boxShadow:
                            selectedCategory === "All"
                                ? "0px 4px 10px rgba(0,0,0,0.25)"
                                : "0px 4px 10px rgba(0,0,0,0.1)",
                    }}
                    onClick={() => callback("All")}
                >
                    <FaGlobeEurope
                        style={{
                            color:
                                selectedCategory === "All"
                                    ? "#111111"
                                    : "#EFEFEF",
                            fontSize: 18,
                        }}
                    />
                </div>
                <div
                    className={styles.categoryIcon}
                    style={{
                        boxShadow:
                            selectedCategory === "Productivity"
                                ? "0px 4px 10px rgba(0,0,0,0.25)"
                                : "0px 4px 10px rgba(0,0,0,0.1)",
                    }}
                    onClick={() => callback("Productivity")}
                >
                    <FaBook
                        style={{
                            color:
                                selectedCategory === "Productivity"
                                    ? "#111111"
                                    : "#EFEFEF",
                            fontSize: 18,
                        }}
                    />
                </div>
            </Col>
        </>
    )
}

export default LeftColumn
