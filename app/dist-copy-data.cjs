const fs = require('fs').promises;
const path = require('path');

async function copyDir(src, dest) {
    await fs.mkdir(dest, { recursive: true });
    const entries = await fs.readdir(src, { withFileTypes: true });

    for (const entry of entries) {
        const srcPath = path.join(src, entry.name);
        const destPath = path.join(dest, entry.name);

        if (entry.isDirectory()) {
            await copyDir(srcPath, destPath);
        } else {
            await fs.copyFile(srcPath, destPath);
        }
    }
}

(async () => {
    try {
        const srcDir = path.join(__dirname, 'dist');
        const destDir = path.join(__dirname, '..', 'data');

        await copyDir(srcDir, destDir);
        console.log(`已将 ${srcDir} 下的所有内容复制到 ${destDir}`);
    } catch (err) {
        console.error('复制失败：', err);
        process.exit(1);
    }
})();