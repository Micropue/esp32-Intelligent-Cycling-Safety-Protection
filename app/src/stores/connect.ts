export const useConnectStore = defineStore("ws-esp32", () => {
    const ws = new WebSocket("ws://172.20.10.2:81")
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