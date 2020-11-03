const loadingMessages = [
    "Remember to save your files to cloud storage. Your cookies are erased at logoff to preserve your privacy.",
    "Fractal never tracks your streaming session and all streamed information is encrypted.",
    "Don't forget to join our Discord here: https://discord.gg/PDNpHjy.",
    "If you make cool artwork on Fractal, tweet us @tryfractal and we'll share it!",
    "Launch Fractal directly from any browser by typing fractal:// followed by a URL.",
]

export const generateMessage = (): string => {
    return _.shuffle(loadingMessages)[0]
}
