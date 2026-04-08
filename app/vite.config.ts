import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import AutoImport from 'unplugin-auto-import/vite'
import compression from 'vite-plugin-compression2'
// https://vite.dev/config/
export default defineConfig({
  plugins: [
    vue({
      template: {
        compilerOptions: {
          isCustomElement: (tag) => tag.startsWith("md-")
        }
      }
    }),
    AutoImport({
      imports: ['vue', 'vue-router', 'pinia'],
      dts: "./src/auto-imports.d.ts"
    }),
    compression()
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
      '$': fileURLToPath(new URL('./src/views', import.meta.url))
    },
  },
  server: {
    port: 5174,
    host: true,
    proxy: {
      '/api': {
        target: "http://192.168.43.13/",
        changeOrigin: true
      }
    }
  }
})
