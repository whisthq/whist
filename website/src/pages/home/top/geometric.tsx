import React from "react"
import "./geometric.css"

export const Geometric = (props: {
    scale: number
    flip: boolean
    className?: string
}) => {
    /*
        Animated geometric SVG background

        Arguments:
            color (string): SVG color
            scale (number): How big the SVG should be, 1 is the original size, 2 is double the size
            flip (boolean): Flip the SVG
    */

    const { scale, flip } = props

    const width = scale * 210
    const height = scale * 264

    return (
        <div
            className={props.className}
            style={{
                transform: `scale(${scale})`,
                maxHeight: 300,
                overflowY: "hidden",
                pointerEvents: "none",
            }}
        >
            <svg
                width={width}
                height={height}
                viewBox={`0 0 ${width} ${height}`}
                fill="none"
                xmlns="http://www.w3.org/2000/svg"
            >
                {flip ? (
                    <path
                        d="M138.784 0.311959L80.127 65.7281M26.2533 128.099L80.127 65.7281M26.2533 128.099L142.284 129.01L143.107 233.453L86.7494 233.011L141.281 169.984L69.0104 169.416L26.2533 128.099ZM80.127 65.7281L141.789 66.2121L80.6116 127.204L80.127 65.7281Z"
                        strokeWidth="0.35"
                        className="geometricOne stroke-current text-blue-light dark:text-gray-800"
                    />
                ) : (
                    <path
                        d="M69.2158 1.31215L127.837 66.7603M181.677 129.162L127.837 66.7603M181.677 129.162L65.7096 130.081L64.8799 234.582L121.207 234.136L66.7094 171.077L138.94 170.505L181.677 129.162ZM127.837 66.7603L66.2085 67.2487L127.348 128.27L127.837 66.7603Z"
                        strokeWidth="0.35"
                        className="geometricOne stroke-current text-blue-light dark:text-gray-800"
                    />
                )}
            </svg>
        </div>
    )
}

export default Geometric
