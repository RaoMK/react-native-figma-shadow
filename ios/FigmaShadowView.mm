#import "FigmaShadowView.h"

#import <react/renderer/components/RNFigmaShadowSpec/ComponentDescriptors.h>
#import <react/renderer/components/RNFigmaShadowSpec/EventEmitters.h>
#import <react/renderer/components/RNFigmaShadowSpec/Props.h>
#import <react/renderer/components/RNFigmaShadowSpec/RCTComponentViewHelpers.h>

#import "figmashadow/FigmaShadow.h"

using namespace facebook::react;

@interface FigmaShadowView () <RCTFigmaShadowViewViewProtocol>
@end

@implementation FigmaShadowView {
  CALayer *_shadowLayer;

  std::string _shadow;
  std::string _fillColor;
  float _radiusTL;
  float _radiusTR;
  float _radiusBR;
  float _radiusBL;
  float _bleedLeft;
  float _bleedTop;
  float _bleedRight;
  float _bleedBottom;
  float _pixelRatio;
  bool _highQuality;
  uint64_t _renderGeneration;
}

+ (ComponentDescriptorProvider)componentDescriptorProvider
{
  return concreteComponentDescriptorProvider<FigmaShadowViewComponentDescriptor>();
}

- (instancetype)initWithFrame:(CGRect)frame
{
  if (self = [super initWithFrame:frame]) {
    static const auto defaultProps = std::make_shared<const FigmaShadowViewProps>();
    _props = defaultProps;
    _pixelRatio = (float)[UIScreen mainScreen].scale;

    _shadowLayer = [CALayer layer];
    _shadowLayer.actions = @{
      @"contents" : [NSNull null],
      @"position" : [NSNull null],
      @"bounds" : [NSNull null],
    };
    // Behind the React-managed child views.
    [self.layer insertSublayer:_shadowLayer atIndex:0];
  }
  return self;
}

#pragma mark - Props

- (void)updateProps:(Props::Shared const &)props oldProps:(Props::Shared const &)oldProps
{
  const auto &newProps = *std::static_pointer_cast<FigmaShadowViewProps const>(props);

  _shadow = newProps.shadow;
  _fillColor = newProps.fillColor;
  _radiusTL = newProps.borderTopLeftRadius;
  _radiusTR = newProps.borderTopRightRadius;
  _radiusBR = newProps.borderBottomRightRadius;
  _radiusBL = newProps.borderBottomLeftRadius;
  _bleedLeft = newProps.bleedLeft;
  _bleedTop = newProps.bleedTop;
  _bleedRight = newProps.bleedRight;
  _bleedBottom = newProps.bleedBottom;
  _highQuality = newProps.highQuality;
  if (newProps.pixelRatio > 0) {
    _pixelRatio = newProps.pixelRatio;
  }

  [super updateProps:props oldProps:oldProps];
  [self setNeedsShadowRender];
}

- (void)updateLayoutMetrics:(const LayoutMetrics &)layoutMetrics
           oldLayoutMetrics:(const LayoutMetrics &)oldLayoutMetrics
{
  [super updateLayoutMetrics:layoutMetrics oldLayoutMetrics:oldLayoutMetrics];
  [self setNeedsShadowRender];
}

#pragma mark - Rendering

- (void)setNeedsShadowRender
{
  // Coalesce prop + layout updates that arrive in the same commit.
  [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(renderShadow) object:nil];
  [self performSelector:@selector(renderShadow) withObject:nil afterDelay:0];
}

+ (dispatch_queue_t)renderQueue
{
  static dispatch_queue_t queue;
  static dispatch_once_t once;
  dispatch_once(&once, ^{
    queue = dispatch_queue_create("com.figmashadow.raster", DISPATCH_QUEUE_SERIAL);
  });
  return queue;
}

- (void)renderShadow
{
  const CGRect bounds = self.bounds;
  const CGFloat viewW = CGRectGetWidth(bounds);
  const CGFloat viewH = CGRectGetHeight(bounds);
  if (viewW <= 0 || viewH <= 0) {
    return;
  }

  const float contentW = (float)viewW - _bleedLeft - _bleedRight;
  const float contentH = (float)viewH - _bleedTop - _bleedBottom;
  if (contentW <= 0 || contentH <= 0) {
    _shadowLayer.contents = nil;
    return;
  }

  const uint64_t generation = ++_renderGeneration;
  const std::string shadow = _shadow;
  const std::string fillColor = _fillColor;
  const float radiusTL = _radiusTL, radiusTR = _radiusTR, radiusBR = _radiusBR, radiusBL = _radiusBL;
  const float bleedLeft = _bleedLeft, bleedTop = _bleedTop, bleedRight = _bleedRight, bleedBottom = _bleedBottom;
  const float pixelRatio = _pixelRatio;
  const bool highQuality = _highQuality;

  __weak FigmaShadowView *weakSelf = self;
  dispatch_async([FigmaShadowView renderQueue], ^{
    figmashadow::Bitmap bmp = figmashadow::render(
        contentW, contentH, radiusTL, radiusTR, radiusBR, radiusBL, shadow, fillColor,
        bleedLeft, bleedTop, bleedRight, bleedBottom, pixelRatio, highQuality);

    CGImageRef image = bmp.empty() ? nullptr : [FigmaShadowView makeImageFromBitmap:bmp];

    dispatch_async(dispatch_get_main_queue(), ^{
      FigmaShadowView *strongSelf = weakSelf;
      if (strongSelf == nil || generation != strongSelf->_renderGeneration) {
        if (image) CGImageRelease(image);
        return;
      }
      strongSelf->_shadowLayer.frame = strongSelf.bounds;
      strongSelf->_shadowLayer.contentsScale = 1.0;
      strongSelf->_shadowLayer.contentsGravity = kCAGravityResize;
      strongSelf->_shadowLayer.contents = (__bridge_transfer id)image;
    });
  });
}

+ (CGImageRef)makeImageFromBitmap:(const figmashadow::Bitmap &)bmp CF_RETURNS_RETAINED
{
  const size_t bytesPerRow = (size_t)bmp.width * 4;
  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  CGContextRef ctx = CGBitmapContextCreate(
      (void *)bmp.pixels.data(), bmp.width, bmp.height, 8, bytesPerRow, colorSpace,
      kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
  CGImageRef image = CGBitmapContextCreateImage(ctx);
  CGContextRelease(ctx);
  CGColorSpaceRelease(colorSpace);
  return image;
}

- (void)prepareForRecycle
{
  [super prepareForRecycle];
  _shadowLayer.contents = nil;
  _shadow.clear();
  _fillColor.clear();
  _renderGeneration++;
}

@end

Class<RCTComponentViewProtocol> FigmaShadowViewCls(void)
{
  return FigmaShadowView.class;
}
