import { defineConfig } from 'vite';

export default defineConfig({
  server: {
    port: 5174,
    proxy: {
      '/api': {
        target: 'http://localhost:8011',
        changeOrigin: true,
        ws: true,
      },
    },
  },
});
