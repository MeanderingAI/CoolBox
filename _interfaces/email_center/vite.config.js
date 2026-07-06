import { defineConfig } from 'vite';

export default defineConfig({
  server: {
    port: 5175,
    proxy: {
      '/api': {
        target: 'http://localhost:8022',
        changeOrigin: true,
        ws: true,
      },
    },
  },
});
