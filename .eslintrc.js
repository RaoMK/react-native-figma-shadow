module.exports = {
  root: true,
  extends: ['@react-native/eslint-config'],
  ignorePatterns: ['lib/', 'node_modules/', 'cpp/', 'coverage/', '*.config.js', '.eslintrc.js'],
  rules: {
    'prettier/prettier': 'off',
    '@typescript-eslint/no-unused-vars': [
      'error',
      { ignoreRestSiblings: true, argsIgnorePattern: '^_', varsIgnorePattern: '^_' },
    ],
  },
};
