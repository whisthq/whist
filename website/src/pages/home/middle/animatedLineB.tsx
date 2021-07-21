import React from "react"

import "./animatedLine.css"

export const AnimatedLineB = (props: { scale?: number }) => {
    /*
        Animated background SVG

        Arguments:
            scale (number): How big the SVG would be, default size is 1
    */

    const { scale } = props
    return (
        <svg
            width="569"
            height="250"
            viewBox="0 0 569 250"
            fill="none"
            xmlns="http://www.w3.org/2000/svg"
            style={{
                transform:
                    scale !== undefined ? `scale(${scale})` : "scale(1.0)",
            }}
        >
            <path
                d="M20.4162 228.392C20.4162 228.392 57.3912 189.833 70.9913 182.284C84.5913 174.735 91.0913 162.206 99.61 159.351C108.129 156.495 143.504 158.618 153.922 159.612C164.339 160.605 166.249 137.646 178.741 134.954C191.233 132.261 199.135 153.365 212.164 148.998C225.192 144.631 241.07 118.606 259.11 112.559C277.149 106.513 279.812 126.323 296.348 120.78C312.884 115.237 313.157 127.279 328.119 125.254C343.082 123.228 372.344 115.664 392.761 103.587C413.179 91.5092 418.648 70.2391 437.795 59.3354C456.943 48.4318 480.572 53.6261 498.291 53.091C516.01 52.556 545.864 37.1451 545.864 37.1451"
                stroke="#7967ED"
                strokeWidth="3"
                className="animatedLine"
            />
            <g filter="url(#filter_2_d)">
                <circle cx="22" cy="228" r="5" fill="#7967ED" />
            </g>
            <g filter="url(#filter_3_d)">
                <circle
                    cx="547"
                    cy="37"
                    r="5"
                    fill="#7967ED"
                    className="animatedDotB"
                />
            </g>
            <defs>
                <filter
                    id="filter_2_d"
                    x="0"
                    y="206"
                    width="44"
                    height="44"
                    filterUnits="userSpaceOnUse"
                    colorInterpolationFilters="sRGB"
                >
                    <feFlood floodOpacity="0" result="BackgroundImageFix" />
                    <feColorMatrix
                        in="SourceAlpha"
                        type="matrix"
                        values="0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 127 0"
                    />
                    <feMorphology
                        radius="2"
                        operator="dilate"
                        in="SourceAlpha"
                        result="effect1_dropShadow"
                    />
                    <feOffset />
                    <feGaussianBlur stdDeviation="7.5" />
                    <feColorMatrix
                        type="matrix"
                        values="0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 0 0 0 1 0"
                    />
                    <feBlend
                        mode="normal"
                        in2="BackgroundImageFix"
                        result="effect1_dropShadow"
                    />
                    <feBlend
                        mode="normal"
                        in="SourceGraphic"
                        in2="effect1_dropShadow"
                        result="shape"
                    />
                </filter>
                <filter
                    id="filter_3_d"
                    x="525"
                    y="15"
                    width="44"
                    height="44"
                    filterUnits="userSpaceOnUse"
                    colorInterpolationFilters="sRGB"
                >
                    <feFlood floodOpacity="0" result="BackgroundImageFix" />
                    <feColorMatrix
                        in="SourceAlpha"
                        type="matrix"
                        values="0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 127 0"
                    />
                    <feMorphology
                        radius="2"
                        operator="dilate"
                        in="SourceAlpha"
                        result="effect1_dropShadow"
                    />
                    <feOffset />
                    <feGaussianBlur stdDeviation="7.5" />
                    <feColorMatrix
                        type="matrix"
                        values="0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 0 0 0 1 0"
                    />
                    <feBlend
                        mode="normal"
                        in2="BackgroundImageFix"
                        result="effect1_dropShadow"
                    />
                    <feBlend
                        mode="normal"
                        in="SourceGraphic"
                        in2="effect1_dropShadow"
                        result="shape"
                    />
                </filter>
            </defs>
        </svg>
    )
}

export default AnimatedLineB
