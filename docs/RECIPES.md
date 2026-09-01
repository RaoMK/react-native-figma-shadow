# Recipes

Every example assumes:

```tsx
import { Shadow } from 'react-native-figma-shadow';
```

## Card

```tsx
<Shadow shadow="0 4px 20px rgba(0,0,0,0.12)" borderRadius={16}>
  <View style={{ width: 320, height: 180, borderRadius: 16, backgroundColor: '#fff' }} />
</Shadow>
```

The child paints its own white background. `<Shadow>` only draws the shadow.

## Layered / "elevation" shadow

Real product shadows are usually two or three layers — a tight contact shadow
plus a soft ambient one. Paste the whole comma-separated value:

```tsx
<Shadow
  shadow="0 1px 2px rgba(0,0,0,0.06), 0 8px 16px rgba(0,0,0,0.08), 0 16px 32px rgba(0,0,0,0.06)"
  borderRadius={20}
>
  <Card />
</Shadow>
```

## Inner shadow (input well, pressed state)

For `inset`, pass the fill to `<Shadow>` (not the child) so the shadow paints
above the fill, matching CSS:

```tsx
<Shadow
  shadow="inset 0 2px 6px rgba(0,0,0,0.2)"
  borderRadius={10}
  backgroundColor="#f2f2f5"
>
  <TextInput style={{ height: 44, paddingHorizontal: 12 }} />
</Shadow>
```

## Bottom sheet (shadow only on the top edges)

```tsx
<Shadow
  shadow="0 -6px 24px rgba(0,0,0,0.16)"
  borderTopLeftRadius={24}
  borderTopRightRadius={24}
>
  <View style={{ borderTopLeftRadius: 24, borderTopRightRadius: 24, backgroundColor: '#fff' }}>
    {/* sheet content */}
  </View>
</Shadow>
```

## Floating action button (colored shadow)

```tsx
<Shadow shadow="0 8px 20px rgba(37,99,235,0.45)" borderRadius={28}>
  <Pressable style={{ width: 56, height: 56, borderRadius: 28, backgroundColor: '#2563eb' }}>
    <PlusIcon />
  </Pressable>
</Shadow>
```

## Press-to-lift

```tsx
function LiftCard({ children }: { children: React.ReactNode }) {
  const [pressed, setPressed] = useState(false);
  return (
    <Pressable onPressIn={() => setPressed(true)} onPressOut={() => setPressed(false)}>
      <Shadow
        shadow={
          pressed
            ? '0 2px 6px rgba(0,0,0,0.18)'
            : '0 12px 28px rgba(0,0,0,0.18)'
        }
        borderRadius={16}
      >
        <View style={{ borderRadius: 16, backgroundColor: '#fff' }}>{children}</View>
      </Shadow>
    </Pressable>
  );
}
```

There's no built-in animation — this snaps between two states. For a smooth
tween you'd currently cross-fade two `<Shadow>` layers; native animated shadow
props are on the roadmap.

## In a FlatList

```tsx
<FlatList
  data={items}
  renderItem={({ item }) => (
    <Shadow shadow={CARD_SHADOW} borderRadius={14}>
      <Row item={item} />
    </Shadow>
  )}
/>;

// Keep the shadow string a module constant so every row hits the raster cache.
const CARD_SHADOW = '0 3px 12px rgba(0,0,0,0.10)';
```

Because every row's shadow is byte-identical, the bitmap is rasterized once and
shared.

## Design tokens

There's no token layer built in, but a plain object works:

```tsx
export const shadows = {
  sm: '0 1px 3px rgba(0,0,0,0.10)',
  md: '0 4px 12px rgba(0,0,0,0.10), 0 2px 4px rgba(0,0,0,0.06)',
  lg: '0 12px 32px rgba(0,0,0,0.14)',
  xl: '0 24px 48px rgba(0,0,0,0.18)',
} as const;

<Shadow shadow={shadows.md} borderRadius={16}>
  <Card />
</Shadow>;
```

## Gotchas

- **Match `borderRadius`** on `<Shadow>` and the child, or the shadow shape won't
  line up with the card.
- **An ancestor with `overflow: 'hidden'` clips the shadow** — same as CSS.
- **Don't rebuild the `shadow` string inline every render** (`` `0 ${y}px ...` ``
  in JSX) unless it actually changes — you'll miss the cache.
- For `inset`, the child should be transparent and `<Shadow backgroundColor>`
  provides the fill.
