<template>
  <header>
    <div class="logo">
      <svg viewBox="0 -960 960 960">
        <path
          d="m234-480-12-60q-12-5-22.5-10.5T178-564l-58 18-40-68 46-40q-2-13-2-26t2-26l-46-40 40-68 58 18q11-8 21.5-13.5T222-820l12-60h80l12 60q12 5 22.5 10.5T370-796l58-18 40 68-46 40q2 13 2 26t-2 26l46 40-40 68-58-18q-11 8-21.5 13.5T326-540l-12 60h-80Zm40-120q33 0 56.5-23.5T354-680q0-33-23.5-56.5T274-760q-33 0-56.5 23.5T194-680q0 33 23.5 56.5T274-600ZM592-40l-18-84q-17-6-31.5-14.5T514-158l-80 26-56-96 64-56q-2-18-2-36t2-36l-64-56 56-96 80 26q14-11 28.5-19.5T574-516l18-84h112l18 84q17 6 31.5 14.5T782-482l80-26 56 96-64 56q2 18 2 36t-2 36l64 56-56 96-80-26q-14 11-28.5 19.5T722-124l-18 84H592Zm56-160q50 0 85-35t35-85q0-50-35-85t-85-35q-50 0-85 35t-35 85q0 50 35 85t85 35Z">
        </path>
      </svg>
      <p>骑安智护</p>
    </div>
    <div class="status">
      <div class="it" v-if="!connectStore.connected && !connectStore.error">
        <md-circular-progress indeterminate class="loop"></md-circular-progress>
        <p>正在连接</p>
      </div>
      <div class="it" v-if="connectStore.connected">
        <svg viewBox="0 -960 960 960" class="connected">
          <path d="M382-240 154-468l57-57 171 171 367-367 57 57-424 424Z"></path>
        </svg>
        <p>已连接</p>
      </div>
      <div class="it" v-if="connectStore.error">
        <svg viewBox="0 -960 960 960" class="disconnected">
          <path d="m256-200-56-56 224-224-224-224 56-56 224 224 224-224 56 56-224 224 224 224-56 56-224-224-224 224Z">
          </path>
        </svg>
        <p>无法连接</p>
      </div>
    </div>
  </header>
  <main>
    <RouterView></RouterView>
  </main>
</template>
<script setup lang="ts">
import { useConnectStore } from './stores/connect'
import '@material/web/progress/circular-progress'
const connectStore = useConnectStore()

</script>
<style lang="scss" scoped>
header {
  width: 100%;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px 17px;
  box-sizing: border-box;
  background-color: white;

  .logo {
    display: flex;
    align-items: center;
    color: var(--theme-color);

    svg {
      fill: currentColor;
      width: 30px;
      height: 30px;
    }

    p {
      white-space: nowrap;
      margin-left: 15px;
      font-weight: 900;
      font-size: 1.1em;
    }
  }

  .status {
    color: var(--theme-color);
    display: flex;
    align-items: center;

    .it {
      display: flex;
      align-items: center;
    }

    .loop,
    .connected,
    .disconnected {
      width: 40px;
      height: 40px;
      fill: currentColor;
      --_active-indicator-color: var(--theme-color);
    }

    p {
      margin-left: 10px;
      font-size: 0.9em;
    }
  }

  position: relative;

  &::before{
    content: '';
    position: absolute;
    width: 100%;
    height: 40px;
    top: 100%;
    background-color: white;
    left: 0;
  }

  &::after {
    content: '';
    position: absolute;
    top: 100%;
    left: 0;
    width: 100%;
    height: 40px;
    border-radius: 50px 50px 0 0;
    background-color: var(--background-color);
  }
}
main{
  padding-top: 40px;
  box-sizing: border-box;
}
</style>