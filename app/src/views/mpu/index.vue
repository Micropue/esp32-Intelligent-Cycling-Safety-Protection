<template>
    <div class="bike-container">
        <!-- 3D 渲染画布 -->
        <div ref="canvasContainer" class="canvas-container"></div>
    </div>
</template>

<script setup>
import { onMounted, ref, reactive } from 'vue';
import * as THREE from 'three';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls';
import { useMPUConnectStore } from '@/stores/connect'
const mpuStore = useMPUConnectStore()
const canvasContainer = ref(null);
const rotation = reactive({ x: 0, y: 0, z: 0 });
// 1. 默认陀螺仪数据 (用于矫正/零点)
const DEFAULT_MPU = {
    accel: { x: 0.017, y: -0.001, z: 1.074 },
    gyro: { x: -1.145, y: 0.359, z: 0.420 }
};

let scene, camera, renderer, bikeModel, controls;
mpuStore.onMessage(msg => {
    updatePosture(JSON.parse(msg))
})
// 2. 修改姿态函数 (核心逻辑)
function updatePosture(data) {
    const { accel } = data.mpu;

    // 计算当前加速度与默认加速度的差值 (简单欧拉角映射)
    // 注意：实际物理计算需使用 atan2 或四元数，这里采用线性映射方便快速上手
    // 假设 accel.x 影响左右倾斜 (Roll)，accel.y 影响前后俯仰 (Pitch)
    const diffX = accel.x - DEFAULT_MPU.accel.x;
    const diffY = accel.y - DEFAULT_MPU.accel.y;

    // 将加速度差值转换为弧度 (系数 1.5 可根据实际传感器灵敏度调整)
    rotation.z = diffX * 1.5; // 左右倾斜
    rotation.x = -diffY * 1.5; // 前后俯仰

    if (bikeModel) {
        // 应用旋转到模型
        bikeModel.rotation.set(rotation.x, 0, rotation.z);
    }
};

// 初始化 Three.js 场景
const initThree = () => {
    scene = new THREE.Scene();
    scene.background = new THREE.Color("rgb(255, 251, 226)");

    camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
    camera.position.set(2, 2, 5);

    renderer = new THREE.WebGLRenderer({ antialias: true });
    renderer.setSize(window.innerWidth, window.innerHeight);
    canvasContainer.value.appendChild(renderer.domElement);

    // 灯光
    const ambientLight = new THREE.AmbientLight(0xffffff, 0.8);
    scene.add(ambientLight);
    const directionalLight = new THREE.DirectionalLight(0xffffff, 1);
    directionalLight.position.set(5, 10, 7);
    scene.add(directionalLight);

    // 辅助网格 (地面)
    const gridHelper = new THREE.GridHelper(10, 10);
    scene.add(gridHelper);

    // 控制器
    controls = new OrbitControls(camera, renderer.domElement);

    // 3. 导入模型
    const loader = new GLTFLoader();
    // 这里的路径需指向你的 public 文件夹下的模型文件
    loader.load('/models/bike.glb', (gltf) => {
        bikeModel = gltf.scene;

        // 居中模型
        const box = new THREE.Box3().setFromObject(bikeModel);
        const center = box.getCenter(new THREE.Vector3());
        bikeModel.position.x += (bikeModel.position.x - center.x);
        bikeModel.position.y += (bikeModel.position.y - center.y);
        bikeModel.position.z += (bikeModel.position.z - center.z);

        scene.add(bikeModel);
    }, undefined, (error) => {
        console.error('模型加载失败，请检查路径:', error);
        // 如果没模型，创建一个临时方块代替
        bikeModel = new THREE.Mesh(
            new THREE.BoxGeometry(1, 0.5, 2),
            new THREE.MeshStandardMaterial({ color: 0x00ff00 })
        );
        scene.add(bikeModel);
    });

    const animate = () => {
        requestAnimationFrame(animate);
        controls.update();
        renderer.render(scene, camera);
    };
    animate();
};

// 测试函数
const testNormal = () => {
    updatePosture({
        mpu: {
            accel: { x: 0.017, y: -0.001, z: 1.074 },
            gyro: { x: -1.145, y: 0.359, z: 0.420 }
        }
    });
};

const testTilt = () => {
    updatePosture({
        mpu: {
            accel: { x: -0.215, y: -0.195, z: 1.013 },
            gyro: { x: -12.260, y: 24.366, z: 41.076 }
        }
    });
};

onMounted(() => {
    initThree();
    window.addEventListener('resize', () => {
        camera.aspect = window.innerWidth / window.innerHeight;
        camera.updateProjectionMatrix();
        renderer.setSize(window.innerWidth, window.innerHeight);
    });
});
</script>

<style scoped lang="scss">
.bike-container {
    display: flex;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
    padding: 40px 24px;
    background: linear-gradient(180deg, #f7fbff 0%, #ffffff 60%);
}

.canvas-container {
    width: min(900px, 92%);
    height: min(560px, 70vh);
    background: linear-gradient(180deg, #ffffff 0%, #f6f9ff 100%);
    border-radius: 14px;
    box-shadow: 0 18px 40px rgba(16, 24, 40, 0.08), inset 0 1px 0 rgba(255,255,255,0.6);
    overflow: hidden;
    border: 1px solid rgba(16,24,40,0.04);
    display: block;
}

.canvas-container canvas {
    width: 100%;
    height: 100%;
    display: block;
}

.controls {
    position: absolute;
    top: 20px;
    left: 20px;
    padding: 12px 14px;
    border-radius: 10px;
    font-family: system-ui, -apple-system, 'Segoe UI', Roboto, 'Helvetica Neue', Arial;
    background: rgba(255,255,255,0.85);
    backdrop-filter: blur(6px);
    box-shadow: 0 6px 18px rgba(16,24,40,0.08);
}

.controls button {
    margin-right: 10px;
    margin-bottom: 10px;
    cursor: pointer;
    padding: 8px 12px;
    border-radius: 8px;
    border: none;
    background: #2563eb;
    color: #fff;
}

.controls button.secondary {
    background: #0ea5a9;
}

.data-display {
    margin-top: 8px;
    font-weight: 600;
    color: #0f172a;
}

@media (max-width: 640px) {
    .canvas-container {
        width: 96%;
        height: 60vh;
        border-radius: 12px;
    }
    .controls { left: 12px; top: 12px; padding: 10px; }
}

</style>