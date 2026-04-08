import { createRouter, createWebHistory } from 'vue-router'
import Index from '$/index.vue'
const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      component: Index
    }
  ],
})

export default router
