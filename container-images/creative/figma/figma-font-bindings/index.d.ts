export namespace Fonts {
  interface IFontsFigmaItem {
    postscript: string;
    family: string;
    id: string;
    style?: string;
    weight?: number;
    stretch?: number;
    italic?: boolean;
  }

  interface IFonts {
    [path: string]: Array<IFontsFigmaItem>
  }
}

declare module 'figma-linux-rust-binding' {
  function getFonts(dirs: string[], callback: (err: Error, fonts: Fonts.IFonts) => void): void;
}
