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
      },
    },
  },
}
