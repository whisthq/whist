import React from "react";

import "./animatedLine.css";

export const AnimatedLineA = (props: { scale?: number }) => {
  /*
        Animated background SVG

        Arguments:
            scale (number): How big the SVG would be, default size is 1
    */

  const { scale } = props;

  return (
    <div>
      <svg
        width="574"
        height="69"
        viewBox="0 0 574 69"
        fill="none"
        xmlns="http://www.w3.org/2000/svg"
        style={{
          transform: scale !== undefined ? `scale(${scale})` : "scale(1.0)",
        }}
      >
        <path
          d="M22.9713 45.5004C22.9713 45.5004 68 28.0002 82.5 26.0001C97 24 97.5 10.5 106 10.5001C114.5 10.5001 126 23.0002 135 26.0001C144 29 181.5 17.5001 193.5 18.5001C205.5 19.5 206 35.3858 219 35.3857C232 35.3856 241 14 259 14C277 14 286.5 35.3856 303 35.3856C319.5 35.3857 316 43.5008 330 45.5004C344 47.5 372.5 49.0008 394.5 45.5004C416.5 42 428 29.0003 448.5 26.0001C469 23 488.5 31.7713 504.5 35.3856C520.5 39 551.969 35.3857 551.969 35.3857"
          stroke="#69F5B2"
          strokeWidth="3"
          className="animatedLine"
        />
        <g filter="url(#filter0_d)">
          <circle
            cx="552"
            cy="35"
            r="5"
            fill="#69F5B2"
            className="animatedDot"
          />
        </g>
        <g filter="url(#filter1_d)">
          <circle cx="22" cy="47" r="5" fill="#69F5B2" />
        </g>
        <defs>
          <filter
            id="filter0_d"
            x="530"
            y="13"
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
            id="filter1_d"
            x="0"
            y="25"
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
    </div>
  );
};

export default AnimatedLineA;
