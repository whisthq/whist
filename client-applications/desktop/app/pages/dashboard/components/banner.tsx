import React, { useState } from "react"
import { Carousel } from "react-bootstrap"
import { connect } from "react-redux"
import { useQuery } from "@apollo/client"

import { GET_BANNERS } from "shared/constants/graphql"

import styles from "styles/dashboard.css"

const BannerItem = (props: any) => {
    const { bannerItem } = props

    const handleClick = () => {
        if (bannerItem.url) {
            const { shell } = require("electron")
            shell.openExternal(bannerItem.url)
        }
    }

    return (
        <div
            onClick={handleClick}
            className={bannerItem.url ? styles.bannerLink : ""}
        >
            <div className={styles.bannerText}>
                <div className={styles.category}>{bannerItem.category}</div>
                <h2 className={styles.heading}>{bannerItem.heading}</h2>
                <div className={styles.subheading}>{bannerItem.subheading}</div>
            </div>
        </div>
    )
}

const Banner = (props: any) => {
    const { data, error } = useQuery(GET_BANNERS, {
        context: {
            headers: {
                Authorization: `Bearer ${props.accessToken}`,
            },
        },
    })

    console.log(error)

    const bannerData = data ? data.hardware_banners : []
    const bannerBackgrounds = bannerData.map(
        (bannerItem: any) => bannerItem.background
    )

    const [index, setIndex] = useState(0)

    const handleSelect = (selectedIndex: number) => {
        setIndex(selectedIndex)
    }

    return (
        <>
            <Carousel
                controls={false}
                interval={8000}
                style={{ height: 335, zIndex: 1 }}
                onSelect={handleSelect}
            >
                {bannerData.map((bannerItem: any) => (
                    <Carousel.Item key={bannerItem.heading}>
                        <BannerItem bannerItem={bannerItem} />
                    </Carousel.Item>
                ))}
            </Carousel>
            <Carousel
                activeIndex={index}
                controls={false}
                indicators={false}
                style={{
                    position: "fixed",
                    top: 0,
                    left: 0,
                    zIndex: 0,
                    height: "100vh",
                }}
            >
                {bannerBackgrounds.map((background: any) => (
                    <Carousel.Item key={background}>
                        <div
                            style={{
                                zIndex: 0,
                                height: "100vh",
                            }}
                        >
                            <img src={background} style={{ width: "100%" }} />
                        </div>
                    </Carousel.Item>
                ))}
            </Carousel>
        </>
    )
}

const mapStateToProps = (state: any) => {
    return {
        os: state.MainReducer.client.os,
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(Banner)
