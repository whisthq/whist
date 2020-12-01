import React from "react"
import { Col, Carousel } from "react-bootstrap"
import { connect } from "react-redux"

import { openExternal } from "shared/utils/helpers"

const News = (props: any) => {
    const { mediaData } = props

    if (mediaData && mediaData.length > 0) {
        return (
            <Col
                xs={5}
                style={{
                    width: "100%",
                    height: 225,
                }}
            >
                <div
                    style={{
                        padding: 15,
                        background: "white",
                        borderRadius: 10,
                        boxShadow: "0px 4px 10px rgba(0, 0, 0, 0.1)",
                        cursor: "pointer",
                    }}
                >
                    <Carousel
                        style={{ width: "100%", height: "100%" }}
                        prevIcon={<div></div>}
                        nextIcon={<div></div>}
                        indicators={false}
                    >
                        {mediaData.map((mediaItem: any) => (
                            <Carousel.Item
                                key={mediaItem.background}
                                onClick={() => openExternal(mediaItem.url)}
                            >
                                <div
                                    style={{
                                        maxHeight: 195,
                                        overflowY: "scroll",
                                    }}
                                >
                                    <div
                                        style={{
                                            backgroundImage: `url(${mediaItem.background})`,
                                            backgroundSize: "cover",
                                            width: "100%",
                                            height: 125,
                                        }}
                                    ></div>
                                    <div
                                        style={{
                                            marginTop: 30,
                                            fontWeight: "bold",
                                            fontSize: 14,
                                        }}
                                    >
                                        {mediaItem.heading}
                                    </div>
                                    <div
                                        style={{
                                            marginTop: 10,
                                            color: "#333333",
                                            fontSize: 12,
                                        }}
                                    >
                                        {mediaItem.subheading}
                                    </div>
                                </div>
                            </Carousel.Item>
                        ))}
                    </Carousel>
                </div>
            </Col>
        )
    } else {
        return (
            <Col
                xs={4}
                style={{
                    width: "100%",
                    height: 225,
                    background: "white",
                    borderRadius: 10,
                    boxShadow: "0px 4px 10px rgba(0, 0, 0, 0.1)",
                }}
            ></Col>
        )
    }
}

const mapStateToProps = (state: any) => {
    return {
        os: state.MainReducer.client.os,
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(News)
