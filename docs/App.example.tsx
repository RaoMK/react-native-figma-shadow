/**
 * Drop this in as your App.tsx to visually check react-native-figma-shadow.
 *
 *   npm install react-native-figma-shadow@alpha
 *   (iOS)  cd ios && pod install && cd ..
 *
 * Each row renders the SAME box-shadow string two ways:
 *   left  = <Shadow> from this package (one C++ rasterizer, both platforms)
 *   right = core React Native  style={{ boxShadow }}
 *
 * Screenshot in LIGHT mode. The two columns should look the same on iOS and on
 * Android; the left column should also match iOS↔Android exactly.
 */

import React from 'react';
import {
  SafeAreaView,
  ScrollView,
  StyleSheet,
  Text,
  View,
  type ViewStyle,
} from 'react-native';
import { Shadow } from 'react-native-figma-shadow';

type Case = {
  label: string;
  shadow: string;
  radius?: number;
  /** for the inset demo we want a visible fill */
  fill?: string;
};

const CASES: Case[] = [
  {
    label: 'soft drop\n0 0 20px rgba(0,0,0,.5)',
    shadow: '0px 0px 20px 0px rgba(0, 0, 0, 0.5)',
    radius: 12,
  },
  {
    label: 'offset\n8px 10px 16px rgba(0,0,0,.6)',
    shadow: '8px 10px 16px 0px rgba(0, 0, 0, 0.6)',
    radius: 16,
  },
  {
    label: 'Figma card\n0 4px 20px rgba(0,0,0,.15)',
    shadow: '0px 4px 20px 0px rgba(0, 0, 0, 0.15)',
    radius: 16,
  },
  {
    label: 'two layers\n0 2px 4px /.1 + 0 12px 32px /.14',
    shadow:
      '0 2px 4px rgba(0,0,0,0.10), 0 12px 32px rgba(0,0,0,0.14)',
    radius: 16,
  },
  {
    label: 'spread\n0 6px 14px 4px rgba(0,0,0,.25)',
    shadow: '0px 6px 14px 4px rgba(0, 0, 0, 0.25)',
    radius: 20,
  },
  {
    label: 'colored glow\n0 0 24px 2px #3b82f6',
    shadow: '0px 0px 24px 2px rgba(59, 130, 246, 0.9)',
    radius: 24,
  },
  {
    label: 'inset\ninset 0 8px 12px rgba(0,0,0,.7)',
    shadow: 'inset 0px 8px 12px 0px rgba(0, 0, 0, 0.7)',
    radius: 10,
    fill: '#ffffff',
  },
];

const CARD: ViewStyle = { width: 132, height: 84 };

function Row({ item }: { item: Case }) {
  const isInset = !!item.fill;
  return (
    <View style={styles.row}>
      <Text style={styles.label}>{item.label}</Text>
      <View style={styles.pair}>
        <View style={styles.cell}>
          <Text style={styles.tag}>figma-shadow</Text>
          <Shadow
            shadow={item.shadow}
            borderRadius={item.radius}
            backgroundColor={item.fill}
          >
            {/* For inset, keep the child transparent so the shadow sits on top
                of the <Shadow> fill; otherwise give the child its own white. */}
            <View
              style={[
                CARD,
                {
                  borderRadius: item.radius,
                  backgroundColor: isInset ? 'transparent' : '#ffffff',
                },
              ]}
            />
          </Shadow>
        </View>

        <View style={styles.cell}>
          <Text style={styles.tag}>RN boxShadow</Text>
          <View
            style={[
              CARD,
              {
                borderRadius: item.radius,
                backgroundColor: item.fill ?? '#ffffff',
                // @ts-ignore boxShadow is a valid style on RN 0.76+
                boxShadow: item.shadow,
              },
            ]}
          />
        </View>
      </View>
    </View>
  );
}

export default function App() {
  return (
    <SafeAreaView style={styles.safe}>
      <ScrollView contentContainerStyle={styles.scroll}>
        <Text style={styles.title}>react-native-figma-shadow</Text>
        {CASES.map((c) => (
          <Row key={c.label} item={c} />
        ))}
        <View style={{ height: 32 }} />
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safe: { flex: 1, backgroundColor: '#e9e9ee' },
  scroll: { padding: 20, alignItems: 'center' },
  title: {
    fontSize: 16,
    fontWeight: '700',
    color: '#111',
    marginBottom: 16,
  },
  row: {
    marginBottom: 26,
    alignItems: 'center',
  },
  label: {
    fontSize: 11,
    lineHeight: 15,
    color: '#444',
    textAlign: 'center',
    marginBottom: 14,
  },
  pair: { flexDirection: 'row', gap: 28 },
  cell: { alignItems: 'center', gap: 8 },
  tag: { fontSize: 10, color: '#888' },
});
