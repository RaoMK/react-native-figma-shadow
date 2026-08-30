import { computeBleed, parseBoxShadow } from '../parseShadow';

describe('parseBoxShadow', () => {
  it('parses a single Figma "Copy as CSS" value', () => {
    const layers = parseBoxShadow('0px 4px 20px 0px rgba(0, 0, 0, 0.15)');
    expect(layers).toEqual([
      { offsetX: 0, offsetY: 4, blur: 20, spread: 0, inset: false },
    ]);
  });

  it('strips the declaration prefix and trailing semicolon', () => {
    const layers = parseBoxShadow('box-shadow: 0 2px 4px rgba(0,0,0,.1);');
    expect(layers).toHaveLength(1);
    expect(layers[0].blur).toBe(4);
  });

  it('parses multiple comma-separated shadows', () => {
    const layers = parseBoxShadow(
      '0 2px 4px rgba(0,0,0,.1), 0 12px 32px rgba(0,0,0,.14)'
    );
    expect(layers).toHaveLength(2);
    expect(layers[1]).toMatchObject({ offsetY: 12, blur: 32 });
  });

  it('handles inset in any position and a leading color', () => {
    const layers = parseBoxShadow('rgba(0,0,0,0.3) 0 4px 8px 2px inset');
    expect(layers[0]).toMatchObject({
      offsetY: 4,
      blur: 8,
      spread: 2,
      inset: true,
    });
  });

  it('supports two-value (offset only) shadows', () => {
    const layers = parseBoxShadow('2px 2px #f00');
    expect(layers[0]).toMatchObject({ offsetX: 2, offsetY: 2, blur: 0 });
  });

  it('returns nothing for empty / none / garbage', () => {
    expect(parseBoxShadow('')).toEqual([]);
    expect(parseBoxShadow('none')).toEqual([]);
    expect(parseBoxShadow('not a shadow')).toEqual([]);
    expect(parseBoxShadow(undefined)).toEqual([]);
  });
});

describe('computeBleed', () => {
  it('is zero when there is no shadow', () => {
    expect(computeBleed([])).toEqual({ left: 0, top: 0, right: 0, bottom: 0 });
  });

  it('covers 3 sigma of blur plus spread', () => {
    const bleed = computeBleed(parseBoxShadow('0 0 20px 0 rgba(0,0,0,.5)'));
    // 1.5 * 20 = 30
    expect(bleed).toEqual({ left: 30, top: 30, right: 30, bottom: 30 });
  });

  it('adds positive offset on the side it points to', () => {
    const bleed = computeBleed(parseBoxShadow('8px 10px 16px 0 rgba(0,0,0,.5)'));
    expect(bleed.right).toBe(Math.ceil(1.5 * 16 + 8));
    expect(bleed.bottom).toBe(Math.ceil(1.5 * 16 + 10));
    expect(bleed.left).toBe(24);
    expect(bleed.top).toBe(24);
  });

  it('ignores inset shadows', () => {
    expect(computeBleed(parseBoxShadow('inset 0 8px 12px rgba(0,0,0,.5)'))).toEqual({
      left: 0,
      top: 0,
      right: 0,
      bottom: 0,
    });
  });

  it('takes the max across layers', () => {
    const bleed = computeBleed(
      parseBoxShadow('0 1px 2px rgba(0,0,0,.2), 0 8px 40px rgba(0,0,0,.2)')
    );
    expect(bleed.bottom).toBe(Math.ceil(1.5 * 40 + 8));
  });
});
