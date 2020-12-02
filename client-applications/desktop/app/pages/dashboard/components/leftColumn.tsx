import React from "react"
import { Col } from "react-bootstrap"
import { FaGlobeEurope, FaBook, FaPalette, FaLayerGroup } from "react-icons/fa"

import CategoryIcon from "pages/dashboard/components/categoryIcon"
import { FractalAppCategory } from "shared/enums/navigation"

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
                                        selectedCategory ===
                                        FractalAppCategory.ALL
                                            ? "#3930b8"
                                            : "#EFEFEF",
                                    fontSize: 18,
                                }}
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category={FractalAppCategory.ALL}
                    />
                    <CategoryIcon
                        icon={
                            <FaGlobeEurope
                                style={{
                                    color:
                                        selectedCategory ===
                                        FractalAppCategory.BROWSER
                                            ? "#3930b8"
                                            : "#EFEFEF",
                                    fontSize: 18,
                                }}
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category={FractalAppCategory.BROWSER}
                    />
                    <CategoryIcon
                        icon={
                            <FaPalette
                                style={{
                                    color:
                                        selectedCategory ===
                                        FractalAppCategory.CREATIVE
                                            ? "#3930b8"
                                            : "#EFEFEF",
                                    fontSize: 18,
                                }}
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category={FractalAppCategory.CREATIVE}
                    />
                    <CategoryIcon
                        icon={
                            <FaBook
                                style={{
                                    color:
                                        selectedCategory ===
                                        FractalAppCategory.PRODUCTIVITY
                                            ? "#3930b8"
                                            : "#EFEFEF",
                                    fontSize: 18,
                                }}
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category={FractalAppCategory.PRODUCTIVITY}
                    />
                </div>
            </Col>
        </>
    )
}

export default LeftColumn
