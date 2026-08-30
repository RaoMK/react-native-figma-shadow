import type { ViewProps } from 'react-native';
import type { Float, WithDefault } from 'react-native/Libraries/Types/CodegenTypes';
import codegenNativeComponent from 'react-native/Libraries/Utilities/codegenNativeComponent';

export interface NativeProps extends ViewProps {
  /** A CSS `box-shadow` value, exactly as copied from Figma / a stylesheet. */
  shadow?: string;

  /** Optional fill painted inside the content box, below inset shadows. */
  fillColor?: string;

  borderTopLeftRadius?: WithDefault<Float, 0>;
  borderTopRightRadius?: WithDefault<Float, 0>;
  borderBottomRightRadius?: WithDefault<Float, 0>;
  borderBottomLeftRadius?: WithDefault<Float, 0>;

  /** Bleed insets (device-independent px) computed on the JS side. */
  bleedLeft?: WithDefault<Float, 0>;
  bleedTop?: WithDefault<Float, 0>;
  bleedRight?: WithDefault<Float, 0>;
  bleedBottom?: WithDefault<Float, 0>;

  /** `PixelRatio.get()` — passed so the raster matches the screen density. */
  pixelRatio?: WithDefault<Float, 1>;

  /** Use the slower, exact rounded-corner quadrature. */
  highQuality?: WithDefault<boolean, false>;
}

export default codegenNativeComponent<NativeProps>('FigmaShadowView');
