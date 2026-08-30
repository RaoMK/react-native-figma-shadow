import React, { useMemo } from 'react';
import {
  PixelRatio,
  StyleSheet,
  View,
  type StyleProp,
  type ViewProps,
  type ViewStyle,
} from 'react-native';

import FigmaShadowView from './FigmaShadowViewNativeComponent';
import { computeBleed, parseBoxShadow, type EdgeInsets } from './parseShadow';

export type { EdgeInsets, ParsedShadowLayer } from './parseShadow';
export { parseBoxShadow, computeBleed } from './parseShadow';

export interface ShadowProps extends Omit<ViewProps, 'style'> {
  /**
   * A CSS `box-shadow` value — the exact string Figma's "Copy as CSS" produces,
   * or what you already use on the web. Supports multiple comma-separated
   * shadows, `inset`, spread, and any CSS color. Renders identically on iOS and
   * Android.
   */
  shadow?: string;

  /** Uniform corner radius for the shadow shape. Match your child's radius. */
  borderRadius?: number;
  borderTopLeftRadius?: number;
  borderTopRightRadius?: number;
  borderBottomRightRadius?: number;
  borderBottomLeftRadius?: number;

  /**
   * Fill painted inside the content box, below any inset shadow and below the
   * children. Use this (instead of a background on the child) when you want an
   * inset shadow to sit above the fill, like CSS.
   */
  backgroundColor?: string;

  /** Opt into the slower, exact rounded-corner convolution. */
  highQuality?: boolean;

  /** Applied to the outer layout box. */
  style?: StyleProp<ViewStyle>;

  children?: React.ReactNode;
}

function resolveRadii(props: ShadowProps) {
  const base = props.borderRadius ?? 0;
  return {
    tl: props.borderTopLeftRadius ?? base,
    tr: props.borderTopRightRadius ?? base,
    br: props.borderBottomRightRadius ?? base,
    bl: props.borderBottomLeftRadius ?? base,
  };
}

/**
 * Wraps its children and paints a pixel-consistent CSS-style shadow behind them
 * on both platforms.
 *
 * Layout is neutral: the shadow bleeds into space the component reserves with
 * negative margins, so siblings are laid out as if only the children were there.
 */
export function Shadow(props: ShadowProps) {
  const {
    shadow = '',
    backgroundColor,
    highQuality = false,
    style,
    children,
    // Pulled out so they are not spread onto the native view (which only takes
    // per-corner radii); `backgroundColor` is forwarded below as `fillColor` to
    // avoid colliding with RN's built-in view prop.
    borderRadius: _borderRadius,
    borderTopLeftRadius: _btl,
    borderTopRightRadius: _btr,
    borderBottomRightRadius: _bbr,
    borderBottomLeftRadius: _bbl,
    ...rest
  } = props;

  const layers = useMemo(() => parseBoxShadow(shadow), [shadow]);
  const bleed: EdgeInsets = useMemo(() => computeBleed(layers), [layers]);
  const radii = resolveRadii(props);

  const geometry: ViewStyle = {
    marginLeft: -bleed.left,
    marginTop: -bleed.top,
    marginRight: -bleed.right,
    marginBottom: -bleed.bottom,
    paddingLeft: bleed.left,
    paddingTop: bleed.top,
    paddingRight: bleed.right,
    paddingBottom: bleed.bottom,
  };

  return (
    <View style={[styles.outer, style]}>
      <FigmaShadowView
        {...rest}
        shadow={shadow}
        fillColor={backgroundColor}
        highQuality={highQuality}
        borderTopLeftRadius={radii.tl}
        borderTopRightRadius={radii.tr}
        borderBottomRightRadius={radii.br}
        borderBottomLeftRadius={radii.bl}
        bleedLeft={bleed.left}
        bleedTop={bleed.top}
        bleedRight={bleed.right}
        bleedBottom={bleed.bottom}
        pixelRatio={PixelRatio.get()}
        style={[styles.native, geometry]}
      >
        {children}
      </FigmaShadowView>
    </View>
  );
}

const styles = StyleSheet.create({
  outer: {
    // The shadow bleeds past this layout box; an ancestor with `overflow:
    // hidden` will clip it, exactly as it would in CSS.
    overflow: 'visible',
  },
  native: {
    overflow: 'visible',
  },
});

export default Shadow;
