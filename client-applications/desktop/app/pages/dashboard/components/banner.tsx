import React from "react"
import { Carousel } from "react-bootstrap"
import { connect } from "react-redux"
import { useQuery } from "@apollo/client"

import { GET_BANNERS } from "shared/constants/graphql"

const Banner = (props: any) => {
    const { data } = useQuery(GET_BANNERS, {
        context: {
            headers: {
                Authorization: `Bearer ${props.accessToken}`,
            },
        },
    })

    const bannerData = data
        ? data.hardware_banners.filter(
              (bannerData: any) => bannerData.category === "News"
          )
        : []

    const handleClick = (url: string) => {
        if (url) {
            const { shell } = require("electron")
            shell.openExternal(url)
        }
    }

    if (bannerData && bannerData.length > 0) {
        return (
            <Carousel style={{ width: "100%", height: "100%" }}>
                {bannerData.map((bannerItem: any) => (
                    <Carousel.Item
                        key={bannerItem.background}
                        onClick={() => handleClick(bannerItem.url)}
                    >
                        <div
                            style={{
                                backgroundImage: `url(${bannerItem.background})`,
                                backgroundSize: "cover",
                                width: "100%",
                                height: 225,
                                borderRadius: 10,
                                borderLeft: "none",
                                boxShadow: "0px 4px 10px rgba(0, 0, 0, 0.1)",
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
