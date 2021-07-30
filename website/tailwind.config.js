module.exports = {
    purge: ["./src/**/*.{js,jsx,ts,tsx}", "./public/index.html"],
    experimental: {
        darkModeVariant: true,
    },
    darkMode: "class",
    theme: {
        extend: {
            fontFamily: {
                sans: ["Josefin Sans"],
                body: ["Myriad-Pro"],
            },
            colors: {
                transparent: "transparent",
                current: "currentColor",
                blue: {
                    DEFAULT: "#4F35DE",
                    light: "#e3defa",
                    lightest: "#f6f9ff",
                    darker: "#151636",
                    darkest: "#0d0f21",
                },
                mint: {
                    DEFAULT: "#00FFA2",
                    light: "#c9ffeb",
                },
                red: {
                    DEFAULT: "#B91C1C",
                },
            },
            boxShadow: {
                bright: "0px 4px 50px rgba(255,255,255,0.25)",
            },
            top: {
                "100px": "100px",
                "150px": "150px",
                "200px": "200px",
                "250px": "250px",
                "300px": "300px",
            },
            left: {
                "100px": "100px",
                "150px": "150px",
                "200px": "200px",
                "250px": "250px",
                "300px": "300px",
            },
            bottom: {
                "100px": "100px",
                "150px": "150px",
                "200px": "200px",
                "250px": "250px",
                "300px": "300px",
            },
            right: {
                "100px": "100px",
                "150px": "150px",
                "200px": "200px",
                "250px": "250px",
                "300px": "300px",
            },
            keyframes: {
                bounce: {
                    "0%, 20%, 50%, 80%, 100%": { transform: "translateY(0)" },
                    "30%": { transform: "translateY(-5px)" },
                    "60%": { transform: "translateY(-8px)" },
                },
                blink: {
                    "0%, 100%": { opacity: 1 },
                    "50%": { opacity: 0 },
                },
            },
            animation: {
                bounce: "bounce 5s infinite",
                blink: "blink 1s step-end infinite",
            },
            letterSpacing: {
                fractal: "0.5em",
            },
        },
    },
}
