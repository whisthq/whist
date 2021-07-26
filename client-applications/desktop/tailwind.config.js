const colors = require("tailwindcss/colors")

module.exports = {
    // This "purge" list should reference any files that call our .css files by
    // name. This includes html, and any js files that 'import' css.
    // This will allow tailwind to analyze and "purge" unused css.
    mode: "jit",
    purge: ["./src/**/*.{js,jsx,ts,tsx}", "./public/index.html"],
    // We may be able to remove "darkModeVariant" soon with tailwind JIT.
    experimental: {
        darkModeVariant: true,
    },
    darkMode: "class",
    // The theme section is where we define anything related to visual design.
    theme: {
        // The extend sections lets us add extra "variants" to utilities. We also
        // override some of the default utilities.
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
                    darkest: "#060217",
                },
                mint: {
                    DEFAULT: "#00FFA2",
                    light: "#c9ffeb",
                },
                gray: colors.blueGray,
                red: {
                    DEFAULT: "#B91C1C",
                },
            },
            boxShadow: {
                bright: "0px 4px 50px rgba(255,255,255,0.25)",
            },
            // These "literals" below can be removed soom with tailwind JIT.
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
            // We can add custom animation utilities using similiar syntax to
            // CSS animation definitions. First we definite custom keyframe ranges,
            // and we can reference these later in the actual animation utiltities.
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
            // These are the utilities that we'll actually call in the program.
            // Note how they reference the classes defined in the keyframes section.
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
