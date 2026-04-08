export const useConnectStore = defineStore("ws-esp32", () => {
    const ws = new WebSocket("ws://192.168.43.13:81")
    const connected = ref(false)
    const error = ref(false)
    ws.onopen = () => {
        connected.value = true
        error.value = false
    }
    ws.onclose = () => {
        connected.value = false
        error.value = false
    }
    ws.onerror = () => {
        error.value = true
    }
    return {
        connected,
        error,
        onMessage(messageHandle: (msg: string) => void) {
            ws.onmessage = (e) => {
                messageHandle(e.data)
            }
        }
    }
})


export const useMPUConnectStore = defineStore("mpu-store", () => {
    const ws = new WebSocket("ws://192.168.43.13:82")
    const connected = ref(false)
    const error = ref(false)
    ws.onopen = () => {
        connected.value = true
        error.value = false
    }
    ws.onclose = () => {
        connected.value = false
        error.value = false
    }
    ws.onerror = () => {
        error.value = true
    }
    return {
        connected,
        error,
        onMessage(messageHandle: (msg: string) => void) {
            ws.onmessage = (e) => {
                messageHandle(e.data)
            }
        }
    }
})