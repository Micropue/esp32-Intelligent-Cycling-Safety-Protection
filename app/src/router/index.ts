import { createRouter, createWebHistory } from 'vue-router'
import Index from '$/index.vue'
const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    {
      path: '/',
      component: Index
    },
    {
      path: '/mpu',
      component: () => import("$/mpu/index.vue")
    }
  ],
})

export default router
