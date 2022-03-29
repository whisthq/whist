module.exports = {
  content: ["./src/**/*.{js,jsx,ts,tsx}", "./public/index.html"],
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
          DEFAULT: "#0092FF",
          light: "#71D9FF",
          dark: "#0000CE",
        },
        gray: {
          800: "#202124",
          900: "#141517",
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
          "0%, 100%": { transform: "translateY(-25%)" },
          "50%": { transform: "translateY(0)" },
        },
        blink: {
          "0%, 100%": { opacity: 1 },
          "50%": { opacity: 0 },
        },
      },
      animation: {
        bounce: "bounce 2s infinite",
        blink: "blink 1s step-end infinite",
      },
      letterSpacing: {
        whist: "0.5em",
      },
    },
  },
}
