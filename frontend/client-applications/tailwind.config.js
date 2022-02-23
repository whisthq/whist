module.exports = {
  // This "purge" list should reference any files that call our .css files by
  // name. This includes html, and any js files that 'import' css.
  // This will allow tailwind to analyze and "purge" unused css.
  purge: ["./src/**/*.{js,jsx,ts,tsx}", "./public/index.html"],
  darkMode: "class",
  // The theme section is where we define anything related to visual design.
  theme: {
    // The extend sections lets us add extra "variants" to utilities. We also
    // override some of the default utilities.
    extend: {
      fontFamily: {
        body: ["Josefin-Sans"],
      },
      colors: {
        transparent: "transparent",
        current: "currentColor",
        blue: {
          DEFAULT: "#4f35de",
        },
        mint: {
          DEFAULT: "#00FFA2",
          light: "#c9ffeb",
        },
        gray: {
          900: "#202124",
        },
      },
      keyframes: {
        "fade-in-up": {
          "0%": {
            opacity: "0.0",
            transform: "translateY(10px)",
          },
          "100%": {
            opacity: "1.0",
            transform: "translateY(0)",
          },
        },
        "fade-in": {
          "0%": {
            opacity: "0.0",
          },
          "100%": {
            opacity: "1.0",
          },
        },
      },
      animation: {
        "fade-in-up": "fade-in-up 1s ease-out forwards",
        "fade-in": "fade-in 1s ease-out forwards",
      },
    },
  },
}
