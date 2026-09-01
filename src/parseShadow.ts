/**
 * A minimal `box-shadow` parser used on the JS side purely to compute how far
 * the shadow bleeds past the element box. The authoritative parse (colors,
 * rendering) happens in the shared C++ core; this only needs geometry.
 */

export interface ParsedShadowLayer {
  offsetX: number;
  offsetY: number;
  blur: number;
  spread: number;
  inset: boolean;
}

export interface EdgeInsets {
  left: number;
  top: number;
  right: number;
  bottom: number;
}

// The shadow is a Gaussian with sigma = blur / 2. Reserve 4 sigma (= 2 * blur)
// of bleed so it fades to nothing before the raster buffer edge; 3 sigma leaves
// a ~0.4% tail that reads as a faint hard edge on a light background.
const BLUR_EXTENT_FACTOR = 2.0;

function splitTopLevel(input: string, delimiter: string): string[] {
  const out: string[] = [];
  let depth = 0;
  let current = '';
  for (const ch of input) {
    if (ch === '(') {depth++;}
    else if (ch === ')') {depth = Math.max(0, depth - 1);}

    if (ch === delimiter && depth === 0) {
      out.push(current);
      current = '';
    } else {
      current += ch;
    }
  }
  out.push(current);
  return out;
}

function tokenize(layer: string): string[] {
  const out: string[] = [];
  let depth = 0;
  let current = '';
  for (const ch of layer) {
    if (ch === '(') {depth++;}
    else if (ch === ')') {depth = Math.max(0, depth - 1);}

    if (/\s/.test(ch) && depth === 0) {
      if (current) {
        out.push(current);
        current = '';
      }
    } else {
      current += ch;
    }
  }
  if (current) {out.push(current);}
  return out;
}

function parseLength(token: string): number | null {
  const match = /^(-?\d*\.?\d+)(px|dp|dip|pt)?$/i.exec(token.trim());
  if (!match) {return null;}
  return parseFloat(match[1]);
}

export function parseBoxShadow(input: string | undefined | null): ParsedShadowLayer[] {
  if (!input) {return [];}
  let s = input.trim();

  const colon = s.indexOf(':');
  if (colon !== -1) {
    const head = s.slice(0, colon).trim().toLowerCase();
    if (head === 'box-shadow' || head === 'boxshadow') {s = s.slice(colon + 1).trim();}
  }
  if (s.endsWith(';')) {s = s.slice(0, -1).trim();}
  if (!s || s.toLowerCase() === 'none') {return [];}

  const layers: ParsedShadowLayer[] = [];
  for (const part of splitTopLevel(s, ',')) {
    const trimmed = part.trim();
    if (!trimmed) {continue;}

    let inset = false;
    const lengths: number[] = [];
    for (const token of tokenize(trimmed)) {
      if (token.toLowerCase() === 'inset') {
        inset = true;
        continue;
      }
      const len = parseLength(token);
      if (len !== null) {lengths.push(len);}
      // anything else (a color) is irrelevant to bleed
    }
    if (lengths.length < 2) {continue;}

    layers.push({
      offsetX: lengths[0],
      offsetY: lengths[1],
      blur: Math.max(0, lengths[2] ?? 0),
      spread: lengths[3] ?? 0,
      inset,
    });
  }
  return layers;
}

export function computeBleed(layers: ParsedShadowLayer[]): EdgeInsets {
  const bleed: EdgeInsets = { left: 0, top: 0, right: 0, bottom: 0 };

  for (const layer of layers) {
    if (layer.inset) {continue;}
    const extent = layer.blur * BLUR_EXTENT_FACTOR + Math.max(0, layer.spread);
    bleed.right = Math.max(bleed.right, extent + Math.max(0, layer.offsetX));
    bleed.left = Math.max(bleed.left, extent + Math.max(0, -layer.offsetX));
    bleed.bottom = Math.max(bleed.bottom, extent + Math.max(0, layer.offsetY));
    bleed.top = Math.max(bleed.top, extent + Math.max(0, -layer.offsetY));
  }

  return {
    left: Math.ceil(bleed.left),
    top: Math.ceil(bleed.top),
    right: Math.ceil(bleed.right),
    bottom: Math.ceil(bleed.bottom),
  };
}

export function hasVisibleShadow(layers: ParsedShadowLayer[]): boolean {
  return layers.length > 0;
}
