// Import with require() packages that require Node 10 experimental
const toIco = require("to-ico")
const png2icons = require("png2icons")

export class SVGConverter {
    /*
        Description:
            Converts an SVG to various image formats. Currently supports
            converting to Base64 PNG, .ico, and .icns.

        Usage: 
            const svgInput = "https://my-svg-url.svg"
            const base64 = await SVGConverter.convertToPngBase64(svgInput)

        Methods:
            base64PngToBuffer(input: string) :
                Converts a base64 encoded PNG to an ArrayBuffer and returns the ArrayBuffer
            convertToPngBase64(input: string) :
                Converts an svg (requires .svg) to base64 and returns a Promise with the base64 string
            convertToIco(input: string) : 
                Converts an svg (requires .svg) to .ico and returns a Promise with the ArrayBuffer
            convertToIcns(input: string) : 
                Converts an svg (requires .svg) to .ico and returns a Promise with the ArrayBuffer
    */
    private static canvas: HTMLCanvasElement = document.createElement("canvas")

    private static imgPreview: HTMLImageElement = document.createElement("img")

    private static canvasCtx: CanvasRenderingContext2D | null = SVGConverter.canvas.getContext(
        "2d"
    )

    private static init() {
        document.body.appendChild(this.imgPreview)
    }

    private static cleanUp() {
        document.body.removeChild(this.imgPreview)
    }

    private static waitForImageToLoad(img: HTMLImageElement) {
        return new Promise((resolve, reject) => {
            img.onload = () => resolve(img)
            img.onerror = reject
        })
    }

    static base64PngToBuffer(base64: string) {
        base64 = base64.replace("data:image/png;base64,", "")
        const buffer = Buffer.from(base64, "base64")
        return buffer
    }

    static async convertToPngBase64(
        input: string,
        width = 64,
        height = 64
    ): Promise<string> {
        // String where base64 output will be written to
        let base64 = ""

        // Load the SVG into HTML image element
        this.init()

        this.imgPreview.style.position = "absolute"
        this.imgPreview.style.top = "-9999px"
        this.imgPreview.src = input

        // Wait for SVG to load into HTML image preview
        await this.waitForImageToLoad(this.imgPreview)

        // Create final image
        const img = new Image()
        this.canvas.width = width
        this.canvas.height = height
        img.crossOrigin = "anonymous"
        img.src = this.imgPreview.src

        // Draw SVG onto canvas with resampling (to resize to 64 x 64)
        await this.waitForImageToLoad(img)

        if (this.canvasCtx) {
            this.canvasCtx.drawImage(
                img,
                0,
                0,
                img.width,
                img.height,
                0,
                0,
                this.canvas.width,
                this.canvas.height
            )
            // Encode PNG as a base64 string
            base64 = this.canvas.toDataURL("image/png")
        }
        this.cleanUp()
        return base64
    }

    static async convertToIco(
        input: string,
        width = 64,
        height = 64
    ): Promise<ArrayBuffer> {
        // SVG to base64 PNG
        const base64 = await this.convertToPngBase64(input, width, height)
        // base64 PNG to Buffer
        const convertedBuffer = this.base64PngToBuffer(base64)
        // Buffer to ICO
        const buffer = await toIco([convertedBuffer])

        return buffer
    }

    static async convertToIcns(
        input: string,
        width = 256,
        height = 256
    ): Promise<ArrayBuffer> {
        // SVG to base64 PNG
        const base64 = await this.convertToPngBase64(input, width, height)
        // base64 PNG to Buffer
        const convertedBuffer = this.base64PngToBuffer(base64)
        // Buffer to ICNS
        const buffer = png2icons.createICNS(
            convertedBuffer,
            png2icons.BICUBIC2,
            0
        )

        return buffer
    }
}

export default SVGConverter
