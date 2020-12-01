import React from "react"
import { Carousel } from "react-bootstrap"
import { connect } from "react-redux"

import { openExternal } from "shared/utils/helpers"

const Banner = (props: any) => {
    const { bannerData } = props

    if (bannerData && bannerData.length > 0) {
        return (
            <Carousel
                style={{ width: "100%", height: "100%" }}
                prevIcon={<div></div>}
                nextIcon={<div></div>}
                indicators={false}
            >
                {bannerData.map((bannerItem: any) => (
                    <Carousel.Item
                        key={bannerItem.background}
                        onClick={() => openExternal(bannerItem.url)}
                    >
                        <div
                            style={{
                                backgroundImage: `url(${bannerItem.background})`,
                                backgroundSize: "cover",
                                width: "100%",
                                height: 225,
                                borderRadius: 12,
                                cursor: "pointer",
                            }}
                        ></div>
                    </Carousel.Item>
                ))}
            </Carousel>
        )
    } else {
        return (
            <div
                style={{ width: "100%", height: "100%", background: "#f4f5ff" }}
            ></div>
        )
    }
}

const mapStateToProps = (state: any) => {
    return {
        os: state.MainReducer.client.os,
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(Banner)
