import React from "react"
import { Col } from "react-bootstrap"
import { FaGlobeEurope, FaBook, FaPalette, FaLayerGroup } from "react-icons/fa"

import CategoryIcon from "pages/dashboard/components/categoryIcon"
import styles from "styles/dashboard.css"

const LeftColumn = (props: {
    callback: (category: string) => void
    selectedCategory: string
}) => {
    const { callback, selectedCategory } = props

    return (
        <>
            <Col xs={1}>
                <div style={{ position: "sticky", top: 25 }}>
                    <CategoryIcon
                        icon={
                            <FaLayerGroup
                                style={{
                                    color:
                                        selectedCategory === "All"
                                            ? "#3930b8"
                                            : "#EFEFEF",
                                    fontSize: 18,
                                }}
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category="All"
                    />
                    <CategoryIcon
                        icon={
                            <FaGlobeEurope
                                style={{
                                    color:
                                        selectedCategory === "Browser"
                                            ? "#3930b8"
                                            : "#EFEFEF",
                                    fontSize: 18,
                                }}
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category="Browser"
                    />
                    <CategoryIcon
                        icon={
                            <FaPalette
                                style={{
                                    color:
                                        selectedCategory === "Creative"
                                            ? "#3930b8"
                                            : "#EFEFEF",
                                    fontSize: 18,
                                }}
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category="Creative"
                    />
                    <CategoryIcon
                        icon={
                            <FaBook
                                style={{
                                    color:
                                        selectedCategory === "Productivity"
                                            ? "#3930b8"
                                            : "#EFEFEF",
                                    fontSize: 18,
                                }}
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category="Productivity"
                    />
                </div>
            </Col>
        </>
    )
}

export default LeftColumn
