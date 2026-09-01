/**
 * react-native-figma-shadow — example / manual-test screen.
 *
 * Every card renders the SAME box-shadow string two ways:
 *   left  = <Shadow> from this package (one C++ rasterizer, both platforms)
 *   right = core React Native  style={{ boxShadow }}
 *
 * The left column should look identical on iOS and Android; on a light
 * background it should also read as a clean, true Gaussian.
 */

import React, { useState } from 'react';
import {
  Pressable,
  SafeAreaView,
  ScrollView,
  StyleSheet,
  Text,
  View,
  type ViewStyle,
} from 'react-native';
import { Shadow } from 'react-native-figma-shadow';

type Sample = {
  label: string;
  shadow: string;
  radius?: number;
  perCorner?: { tl: number; tr: number; br: number; bl: number };
  fill?: string;
};

const SAMPLES: Sample[] = [
  { label: 'soft drop\n0 0 20px rgba(0,0,0,.5)', shadow: '0px 0px 20px 0px rgba(0,0,0,0.5)', radius: 12 },
  { label: 'card\n0 4px 20px rgba(0,0,0,.15)', shadow: '0px 4px 20px 0px rgba(0,0,0,0.15)', radius: 16 },
  { label: 'offset\n8px 10px 16px rgba(0,0,0,.6)', shadow: '8px 10px 16px 0px rgba(0,0,0,0.6)', radius: 16 },
  {
    label: 'layered\n0 1px 2px + 0 8px 16px + 0 16px 32px',
    shadow:
      '0 1px 2px rgba(0,0,0,0.06), 0 8px 16px rgba(0,0,0,0.08), 0 16px 32px rgba(0,0,0,0.06)',
    radius: 20,
  },
  { label: 'spread\n0 6px 14px 4px rgba(0,0,0,.25)', shadow: '0px 6px 14px 4px rgba(0,0,0,0.25)', radius: 20 },
  { label: 'colored glow\n0 0 24px 2px #3b82f6', shadow: '0px 0px 24px 2px rgba(59,130,246,0.9)', radius: 24 },
  {
    label: 'per-corner\ntop 20 / bottom 0',
    shadow: '0 8px 24px rgba(0,0,0,0.16)',
    perCorner: { tl: 20, tr: 20, br: 0, bl: 0 },
  },
  {
    label: 'inset\ninset 0 8px 12px rgba(0,0,0,.7)',
    shadow: 'inset 0px 8px 12px 0px rgba(0,0,0,0.7)',
    radius: 10,
    fill: '#ffffff',
  },
];

const CARD: ViewStyle = { width: 132, height: 84 };

function radiusProps(s: Sample) {
  if (s.perCorner) {
    return {
      borderTopLeftRadius: s.perCorner.tl,
      borderTopRightRadius: s.perCorner.tr,
      borderBottomRightRadius: s.perCorner.br,
      borderBottomLeftRadius: s.perCorner.bl,
    };
  }
  return { borderRadius: s.radius };
}

function childRadiusStyle(s: Sample): ViewStyle {
  if (s.perCorner) {
    return {
      borderTopLeftRadius: s.perCorner.tl,
      borderTopRightRadius: s.perCorner.tr,
      borderBottomRightRadius: s.perCorner.br,
      borderBottomLeftRadius: s.perCorner.bl,
    };
  }
  return { borderRadius: s.radius };
}

function Row({ item }: { item: Sample }) {
  const isInset = !!item.fill;
  return (
    <View style={styles.row}>
      <Text style={styles.label}>{item.label}</Text>
      <View style={styles.pair}>
        <View style={styles.cell}>
          <Text style={styles.tag}>figma-shadow</Text>
          <Shadow shadow={item.shadow} {...radiusProps(item)} backgroundColor={item.fill}>
            <View
              style={[
                CARD,
                childRadiusStyle(item),
                { backgroundColor: isInset ? 'transparent' : '#fff' },
              ]}
            />
          </Shadow>
        </View>
        <View style={styles.cell}>
          <Text style={styles.tag}>RN boxShadow</Text>
          <View
            style={[
              CARD,
              childRadiusStyle(item),
              {
                backgroundColor: item.fill ?? '#fff',
                // @ts-ignore boxShadow is valid on RN 0.76+
                boxShadow: item.shadow,
              },
            ]}
          />
        </View>
      </View>
    </View>
  );
}

function LiftDemo() {
  const [pressed, setPressed] = useState(false);
  return (
    <View style={styles.row}>
      <Text style={styles.label}>press to lift{'\n'}(snaps between two shadows)</Text>
      <Pressable onPressIn={() => setPressed(true)} onPressOut={() => setPressed(false)}>
        <Shadow
          shadow={pressed ? '0 2px 6px rgba(0,0,0,0.2)' : '0 14px 30px rgba(0,0,0,0.2)'}
          borderRadius={16}
        >
          <View style={[CARD, { borderRadius: 16, backgroundColor: '#fff' }]} />
        </Shadow>
      </Pressable>
    </View>
  );
}

export default function App() {
  return (
    <SafeAreaView style={styles.safe}>
      <ScrollView contentContainerStyle={styles.scroll}>
        <Text style={styles.title}>react-native-figma-shadow</Text>
        {SAMPLES.map((s) => (
          <Row key={s.label} item={s} />
        ))}
        <LiftDemo />
        <View style={{ height: 40 }} />
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safe: { flex: 1, backgroundColor: '#e9e9ee' },
  scroll: { padding: 20, alignItems: 'center' },
  title: { fontSize: 16, fontWeight: '700', color: '#111', marginBottom: 16 },
  row: { marginBottom: 28, alignItems: 'center' },
  label: { fontSize: 11, lineHeight: 15, color: '#444', textAlign: 'center', marginBottom: 14 },
  pair: { flexDirection: 'row', gap: 28 },
  cell: { alignItems: 'center', gap: 8 },
  tag: { fontSize: 10, color: '#888' },
});
