<template>
    <div class="home">
        <div class="status">
            <div class="lock" v-if="status.locked">
                <svg viewBox="0 -960 960 960">
                    <path
                        d="M240-80q-33 0-56.5-23.5T160-160v-400q0-33 23.5-56.5T240-640h40v-80q0-83 58.5-141.5T480-920q83 0 141.5 58.5T680-720v80h40q33 0 56.5 23.5T800-560v400q0 33-23.5 56.5T720-80H240Zm0-80h480v-400H240v400Zm240-120q33 0 56.5-23.5T560-360q0-33-23.5-56.5T480-440q-33 0-56.5 23.5T400-360q0 33 23.5 56.5T480-280ZM360-640h240v-80q0-50-35-85t-85-35q-50 0-85 35t-35 85v80ZM240-160v-400 400Z">
                    </path>
                </svg>
                <p>已锁定</p>
            </div>
            <div class="lockopen" v-else>
                <svg viewBox="0 -960 960 960">
                    <path
                        d="M240-640h360v-80q0-50-35-85t-85-35q-50 0-85 35t-35 85h-80q0-83 58.5-141.5T480-920q83 0 141.5 58.5T680-720v80h40q33 0 56.5 23.5T800-560v400q0 33-23.5 56.5T720-80H240q-33 0-56.5-23.5T160-160v-400q0-33 23.5-56.5T240-640Zm0 480h480v-400H240v400Zm240-120q33 0 56.5-23.5T560-360q0-33-23.5-56.5T480-440q-33 0-56.5 23.5T400-360q0 33 23.5 56.5T480-280ZM240-160v-400 400Z">
                    </path>
                </svg>
                <p>已解锁</p>
            </div>
        </div>
        <md-filled-button style="--_container-color: var(--theme-color);" v-if="status.locked" @click="lockopen">
            解锁
            <svg viewBox="0 -960 960 960" slot="icon">
                <path
                    d="M240-640h360v-80q0-50-35-85t-85-35q-50 0-85 35t-35 85h-80q0-83 58.5-141.5T480-920q83 0 141.5 58.5T680-720v80h40q33 0 56.5 23.5T800-560v400q0 33-23.5 56.5T720-80H240q-33 0-56.5-23.5T160-160v-400q0-33 23.5-56.5T240-640Zm0 480h480v-400H240v400Zm240-120q33 0 56.5-23.5T560-360q0-33-23.5-56.5T480-440q-33 0-56.5 23.5T400-360q0 33 23.5 56.5T480-280ZM240-160v-400 400Z">
                </path>
            </svg>
        </md-filled-button>
        <md-filled-button style="--_container-color: var(--theme-color);" v-else @click="lock">
            锁定
            <svg viewBox="0 -960 960 960" slot="icon">
                <path
                    d="M240-80q-33 0-56.5-23.5T160-160v-400q0-33 23.5-56.5T240-640h40v-80q0-83 58.5-141.5T480-920q83 0 141.5 58.5T680-720v80h40q33 0 56.5 23.5T800-560v400q0 33-23.5 56.5T720-80H240Zm0-80h480v-400H240v400Zm240-120q33 0 56.5-23.5T560-360q0-33-23.5-56.5T480-440q-33 0-56.5 23.5T400-360q0 33 23.5 56.5T480-280ZM360-640h240v-80q0-50-35-85t-85-35q-50 0-85 35t-35 85v80ZM240-160v-400 400Z">
                </path>
            </svg>
        </md-filled-button>
    </div>
</template>
<script setup lang="ts">
import '@material/web/button/filled-button'
import '@material/web/button/outlined-button'
import { useConnectStore } from '@/stores/connect'
const connectStore = useConnectStore()
const status = reactive({
    locked: false
})
onMounted(() => {
    connectStore.onMessage((msg) => {
        const res = JSON.parse(msg) as { "isLocked": boolean }
        status.locked = res.isLocked
    })
})

async function lock() {
    await fetch("/api/lock", {
        method: "POST"
    })
}
async function lockopen(){
    await fetch("/api/lockopen",{
        method: "POST"
    })
}
</script>
<style lang="scss" scoped>
.home {
    width: 100%;
    height: 100%;
    display: flex;
    align-items: center;
    justify-content: center;
    flex-direction: column;

    .status {
        width: 250px;
        height: 250px;

        .lock,
        .lockopen {
            width: 100%;
            height: 100%;
            background-color: white;
            border-radius: 50%;
            display: flex;
            align-items: center;
            justify-content: center;
            flex-direction: column;

            svg {
                fill: currentColor;
                width: 90px;
                height: 90px;
            }

            &.lock {
                color: rgb(144, 0, 0);
                background-color: rgba(255, 0, 0, 0.236);
            }

            &.lockopen {
                color: rgb(0, 177, 0);
                background-color: rgba(0, 255, 0, 0.21);
            }
        }
    }

    md-filled-button {
        margin-top: 20px;
    }
}
</style>