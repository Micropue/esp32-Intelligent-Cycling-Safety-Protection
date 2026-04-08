#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <WebSocketsServer.h>
#include <math.h>
// 针脚定义
#define RST_PIN 27 // RST -> GPIO27
#define SS_PIN 5   // SDA -> GPIO5
#define SDA_PIN 25 // OLED SDA
#define SCL_PIN 32 // OLED SCL
#define SD_ID "91-b3-07-7c"
#define LOCKED_STRING "[LOCKED]"
#define UNLOCKED_STRING "[UNLOCKED]"
#define BUZZER_PIN 4 // 蜂鸣器连接到 GPIO9
#define SERVOMOTER_IO 17 // 舵机连接到 GPIO17
// 舵机（使用 ESP32 LEDC PWM 控制）
#define SERVO_CHANNEL 0
#define SERVO_FREQ 50
#define SERVO_RESOLUTION 16
#define SERVO_MIN_PULSE_US 1000
#define SERVO_MAX_PULSE_US 2000
int servoAngle = 90; // 初始角度（中位）
// OLED定义
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

const char *ssid = "YCYMate 60 Pro";
const char *password = "12345678";
// 持久化存储实例
Preferences preferences;

// OLED实例
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WebServer server(80);
//  状态机 - 锁的状态
bool isLocked = true; // true: 上锁，false: 解锁

// 创建MFRC522实例
MFRC522 mfrc522(SS_PIN, RST_PIN);
void beep(int times, int duration);

// WebSocket server (port 81)
WebSocketsServer webSocket = WebSocketsServer(81);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void sendLockStateWS();
void applyLockServo(bool locked);
void setServoAngle(int angle);
// 单独用于 MPU 数据的 WebSocket（端口 82）
WebSocketsServer webSocketMPU = WebSocketsServer(82);
void webSocketMPUEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
// 前向声明 MPUData 以便在下面的函数原型中使用
struct MPUData;
void sendMPUDataWS(MPUData &d);

// MPU 报文发送阈值与节流
#define MPU_READ_INTERVAL_MS 200
#define MPU_SEND_MIN_INTERVAL_MS 200
#define MPU_ANGLE_THRESHOLD_DEG 5.0f
#define MPU_GYRO_THRESHOLD_DPS 30.0f
float lastPitch = 10000.0f;
float lastRoll = 10000.0f;
unsigned long lastMPUReadMs = 0;
unsigned long lastMPUSendMs = 0;
// MPU6050 (use separate I2C on GPIO21 SDA, GPIO22 SCL)
TwoWire I2C_MPU = TwoWire(1);
#define MPU_ADDR 0x68
#define MPU_PWR_MGMT_1 0x6B
#define MPU_ACCEL_XOUT_H 0x3B
#define MPU_GYRO_XOUT_H 0x43
struct MPUData
{
  float ax, ay, az;
  float gx, gy, gz;
};
bool initMPU();
bool readMPU(MPUData &data);

// 函数：获取卡片UID字符串
String getCardUID()
{
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    if (mfrc522.uid.uidByte[i] < 0x10)
      uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1)
      uid += "-";
  }
  return uid;
}

// 函数：保存锁的状态到闪存
void saveLockState()
{
  preferences.begin("lock_app", false);
  preferences.putBool("isLocked", isLocked);
  preferences.end();
  Serial.println("[SAVED]");
}

// 函数：绘制锁定的锁头 🔒
void drawLockedLock(int centerX, int centerY)
{
  // 锁钩 - 左边直线
  display.drawLine(centerX - 6, centerY - 8, centerX - 6, centerY, SSD1306_WHITE);
  display.drawLine(centerX - 5, centerY - 8, centerX - 5, centerY, SSD1306_WHITE);

  // 锁钩 - 顶部半圆弧
  for (int i = -180; i <= 0; i += 5)
  {
    double rad = i * 3.14159 / 180.0;
    int x = centerX + (int)(6 * cos(rad));
    int y = centerY - 8 + (int)(6 * sin(rad));
    display.drawPixel(x, y, SSD1306_WHITE);
    display.drawPixel(x, y - 1, SSD1306_WHITE); // 加粗
  }

  // 锁钩 - 右边直线（连接到锁体）
  display.drawLine(centerX + 6, centerY - 8, centerX + 6, centerY, SSD1306_WHITE);
  display.drawLine(centerX + 5, centerY - 8, centerX + 5, centerY, SSD1306_WHITE);

  // 锁体 - 实心矩形
  display.fillRect(centerX - 12, centerY, 24, 18, SSD1306_WHITE);

  // 钥匙孔（黑色镂空效果）
  display.fillCircle(centerX, centerY + 7, 2, SSD1306_BLACK);
  display.fillRect(centerX - 1, centerY + 7, 3, 5, SSD1306_BLACK);
}

// 函数：绘制解锁的锁头 🔓
void drawUnlockedLock(int centerX, int centerY)
{
  // 锁钩（弹起状态）- 左边直线（变长）
  display.drawLine(centerX - 6, centerY - 12, centerX - 6, centerY, SSD1306_WHITE);
  display.drawLine(centerX - 5, centerY - 12, centerX - 5, centerY, SSD1306_WHITE);

  // 锁钩 - 顶部半圆弧
  for (int i = -180; i <= 0; i += 5)
  {
    double rad = i * 3.14159 / 180.0;
    int x = centerX + (int)(6 * cos(rad));
    int y = centerY - 12 + (int)(6 * sin(rad));
    display.drawPixel(x, y, SSD1306_WHITE);
    display.drawPixel(x, y - 1, SSD1306_WHITE); // 加粗
  }

  // 锁钩 - 右边直线（变短，不连接锁体，形成缺口）
  display.drawLine(centerX + 6, centerY - 12, centerX + 6, centerY - 7, SSD1306_WHITE);
  display.drawLine(centerX + 5, centerY - 12, centerX + 5, centerY - 7, SSD1306_WHITE);

  // 锁体 - 实心矩形
  display.fillRect(centerX - 12, centerY, 24, 18, SSD1306_WHITE);

  // 钥匙孔（黑色镂空效果）
  display.fillCircle(centerX, centerY + 7, 2, SSD1306_BLACK);
  display.fillRect(centerX - 1, centerY + 7, 3, 5, SSD1306_BLACK);
}

// 函数：绘制警告三角 ⚠️
void drawWarningTriangle(int centerX, int centerY)
{
  // 绘制外层三角形边框
  display.drawTriangle(
      centerX, centerY - 16,      // 顶部顶点
      centerX - 16, centerY + 10, // 左下角
      centerX + 16, centerY + 10, // 右下角
      SSD1306_WHITE);

  // 绘制内层三角形边框（加粗效果）
  display.drawTriangle(
      centerX, centerY - 14,
      centerX - 14, centerY + 9,
      centerX + 14, centerY + 9,
      SSD1306_WHITE);

  // 绘制感叹号
  // 上部竖线
  display.fillRect(centerX - 1, centerY - 6, 3, 9, SSD1306_WHITE);
  // 下部圆点
  display.fillRect(centerX - 1, centerY + 5, 3, 3, SSD1306_WHITE);
}

// 函数：显示上锁界面
void displayLocked()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // 绘制锁定的锁头
  drawLockedLock(64, 22);

  // 下方显示文字
  display.setTextSize(1);
  display.setCursor(10, 50);
  display.println("Under Surveillance");

  display.display();
}

// 函数：显示解锁界面
void displayUnlocked()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // 绘制解锁的锁头
  drawUnlockedLock(64, 22);

  // 下方显示文字
  display.setTextSize(1);
  display.setCursor(7, 50);
  display.println("Cycling Safety Mode");

  display.display();
}

// 函数：显示未授权警告界面
void displayUnauthorized()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // 绘制警告三角
  drawWarningTriangle(64, 20);

  // 下方显示警告文字 (19个字符，微调居中)
  display.setTextSize(1);
  display.setCursor(7, 50);
  display.println("Unauthorized Access");

  display.display();
}

void initFS()
{
  Serial.println("[Initializing LittleFS]");
  if (!LittleFS.begin())
  {
    Serial.println("[LittleFS Mount Failed]");
    Serial.println("[Formatting LittleFS...]");
    if (LittleFS.format())
    {
      Serial.println("[LittleFS Formatted Successfully]");
      if (LittleFS.begin())
      {
        Serial.println("[LittleFS Mounted Successfully]");
      }
      else
      {
        Serial.println("[LittleFS Mount Failed After Format]");
      }
    }
    else
    {
      Serial.println("[LittleFS Format Failed]");
    }
  }
  else
  {
    Serial.println("[LittleFS Mounted Successfully]");
  }
}

void connectWifi()
{
  Serial.println("[Connecting to WiFi]");

  WiFi.persistent(false);      // 禁用持久化存储
  WiFi.setAutoReconnect(true); // 启用自动重连
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[WiFi Connected]");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initWebServer()
{
  // 优先注册 API 路由
  server.on("/api/status", HTTP_GET, []()
            {
    String json = "{\"isLocked\":" + String(isLocked ? "true" : "false") + "}";
    server.send(200, "application/json", json);
    Serial.println("[API] /api/status called"); });

  // POST 接口：前端请求上锁
  server.on("/api/lock", HTTP_POST, []()
            {
    isLocked = true;
    saveLockState();
    sendLockStateWS();
    applyLockServo(true);
    beep(2, 100); // 锁定时滴滴两声
    displayLocked();
    String json = "{\"isLocked\":true}";
    server.send(200, "application/json", json);
    Serial.println("[API] /api/lock called"); });

  // POST 接口：前端请求解锁
  server.on("/api/lockopen", HTTP_POST, []()
            {
    isLocked = false;
    saveLockState();
    sendLockStateWS();
    applyLockServo(false);
    beep(2, 100); // 锁定时滴滴两声
    displayUnlocked();
    String json = "{\"isLocked\":false}";
    server.send(200, "application/json", json);
    Serial.println("[API] /api/lockopen called"); });

  // 注册静态文件服务
  server.serveStatic("/", LittleFS, "/");

  // 启动服务器
  server.begin();
  Serial.println("[WebServer] Started");
  // 启动 WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("[WebSocket] Started on port 81");
  // 启动 MPU 专用 WebSocket (端口 82)
  webSocketMPU.begin();
  webSocketMPU.onEvent(webSocketMPUEvent);
  Serial.println("[WebSocket-MPU] Started on port 82");
}

// WebSocket 事件处理
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[WS] Client %u disconnected\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[WS] Client %u connected from %u.%u.%u.%u\n", num, ip[0], ip[1], ip[2], ip[3]);
    // 连接时仅发送当前锁状态（MPU 数据独立通过 82 端口推送）
    String json = String("{\"isLocked\":") + (isLocked ? "true" : "false") + "}";
    webSocket.sendTXT(num, json);
  }
  break;
  case WStype_TEXT:
    Serial.printf("[WS] Received from %u: %s\n", num, payload);
    break;
  default:
    break;
  }
}

// MPU WebSocket 事件处理（端口82）
void webSocketMPUEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[WS-MPU] Client %u disconnected\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocketMPU.remoteIP(num);
    Serial.printf("[WS-MPU] Client %u connected from %u.%u.%u.%u\n", num, ip[0], ip[1], ip[2], ip[3]);
    // 连接时发送一次当前 MPU 数据
    MPUData d = {0, 0, 0, 0, 0, 0};
    readMPU(d);
    sendMPUDataWS(d);
  }
  break;
  case WStype_TEXT:
    Serial.printf("[WS-MPU] Received from %u: %s\n", num, payload);
    break;
  default:
    break;
  }
}

// 广播当前锁状态（JSON）
void sendLockStateWS()
{
  String json = String("{\"isLocked\":") + (isLocked ? "true" : "false") + "}";
  webSocket.broadcastTXT(json);
  Serial.print("[WS] Broadcast: ");
  Serial.println(json);
}

// 发送 MPU 数据到专用 WebSocket（端口82）
void sendMPUDataWS(MPUData &d)
{
  String json = String("{\"mpu\":{");
  json += "\"accel\":{\"x\":" + String(d.ax, 3) + ",\"y\":" + String(d.ay, 3) + ",\"z\":" + String(d.az, 3) + "},";
  json += "\"gyro\":{\"x\":" + String(d.gx, 3) + ",\"y\":" + String(d.gy, 3) + ",\"z\":" + String(d.gz, 3) + "}";
  json += "}}";
  webSocketMPU.broadcastTXT(json);
  Serial.print("[WS-MPU] Broadcast: ");
  Serial.println(json);
}

// 初始化 MPU6050，返回是否成功
bool initMPU()
{
  I2C_MPU.beginTransmission(MPU_ADDR);
  I2C_MPU.write(MPU_PWR_MGMT_1);
  I2C_MPU.write(0x00); // wake up
  int res = I2C_MPU.endTransmission();
  delay(50);
  return (res == 0);
}

// 读取 MPU6050 加速度和陀螺仪（单位：g 和 deg/s），返回是否成功
bool readMPU(MPUData &data)
{
  // request 14 bytes starting at ACCEL_XOUT_H
  I2C_MPU.beginTransmission(MPU_ADDR);
  I2C_MPU.write(MPU_ACCEL_XOUT_H);
  if (I2C_MPU.endTransmission(false) != 0)
    return false;
  if (I2C_MPU.requestFrom(MPU_ADDR, (uint8_t)14) != 14)
    return false;
  int16_t ax = (I2C_MPU.read() << 8) | I2C_MPU.read();
  int16_t ay = (I2C_MPU.read() << 8) | I2C_MPU.read();
  int16_t az = (I2C_MPU.read() << 8) | I2C_MPU.read();
  int16_t tmp = (I2C_MPU.read() << 8) | I2C_MPU.read();
  int16_t gx = (I2C_MPU.read() << 8) | I2C_MPU.read();
  int16_t gy = (I2C_MPU.read() << 8) | I2C_MPU.read();
  int16_t gz = (I2C_MPU.read() << 8) | I2C_MPU.read();

  // 默认传感器量程：加速度 ±2g, 陀螺 ±250 deg/s
  data.ax = (float)ax / 16384.0;
  data.ay = (float)ay / 16384.0;
  data.az = (float)az / 16384.0;
  data.gx = (float)gx / 131.0;
  data.gy = (float)gy / 131.0;
  data.gz = (float)gz / 131.0;
  return true;
}

// 设置舵机角度（0-180），通过 LEDC 输出对应脉宽
void setServoAngle(int angle)
{
  if (angle < 0)
    angle = 0;
  if (angle > 180)
    angle = 180;
  servoAngle = angle;
  uint32_t period_us = 1000000UL / SERVO_FREQ; // 20000
  uint32_t maxDuty = (1UL << SERVO_RESOLUTION) - 1;
  uint32_t pulse_us = map(servoAngle, 0, 180, SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US);
  uint32_t duty = (pulse_us * maxDuty) / period_us;
  ledcWrite(SERVO_CHANNEL, duty);
  Serial.printf("[SERVO] set angle=%d pulse=%u duty=%u\n", servoAngle, (unsigned)pulse_us, (unsigned)duty);
}

// 根据上锁/解锁动作相对移动舵机：上锁 -90，解锁 +90（并在 0-180 范围内截断）
void applyLockServo(bool locked)
{
  if (locked)
  {
    int newAngle = servoAngle - 90;
    if (newAngle < 0)
      newAngle = 0;
    setServoAngle(newAngle);
  }
  else
  {
    int newAngle = servoAngle + 90;
    if (newAngle > 180)
      newAngle = 180;
    setServoAngle(newAngle);
  }
}
void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("[System Starting]");
  Wire.begin(SDA_PIN, SCL_PIN);
  // 初始化 OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
  {
    Serial.println(F("[OLED Initialization Failed]"));
    for (;;)
      ;
  }
  display.setTextColor(SSD1306_WHITE);
  Serial.println("[OLED Initialized Successfully]");

  // 初始化 Preferences
  preferences.begin("lock_app", true);
  isLocked = preferences.getBool("isLocked", true);
  preferences.end();

  // 初始化 SPI 和 RFID
  SPI.begin(18, 19, 23, 5);
  mfrc522.PCD_Init();

  // 初始化蜂鸣器引脚
  pinMode(BUZZER_PIN, OUTPUT);   // 初始化蜂鸣器引脚
  digitalWrite(BUZZER_PIN, LOW); // 默认关闭蜂鸣器
  // 初始化舵机 PWM
  ledcSetup(SERVO_CHANNEL, SERVO_FREQ, SERVO_RESOLUTION);
  ledcAttachPin(SERVOMOTER_IO, SERVO_CHANNEL);
  // 初始位置为中位，再根据当前锁状态调整
  setServoAngle(servoAngle);
  applyLockServo(isLocked);
  // 初始化 MPU6050 的 I2C（SDA=21, SCL=22）并唤醒传感器
  I2C_MPU.begin(21, 22, 400000);
  if (initMPU())
  {
    Serial.println("[MPU] Initialized");
  }
  else
  {
    Serial.println("[MPU] Init failed");
  }
                                 // TODO: 后续添加这个功能
  // TODO:  初始化文件系统
  initFS();

  // 初始化 WiFi
  connectWifi();
  initWebServer();
  // 启动后广播一次当前状态（若有客户端连接则会收到）
  sendLockStateWS();
  // 显示初始状态
  if (isLocked)
  {
    Serial.println(LOCKED_STRING);
    displayLocked();
    beep(2, 100); // 锁定时滴滴两声
  }
  else
  {
    Serial.println(UNLOCKED_STRING);
    displayUnlocked();
    beep(2, 100); // 解锁时滴滴两声
  }
}

void loop()
{
  // 处理 Web 服务器请求
  server.handleClient();
  // 保持 WebSocket 事件循环（包括 MPU 专用通道）
  webSocket.loop();
  webSocketMPU.loop();

  unsigned long now = millis();
  // 周期性读取 MPU 并在偏移时通过 82 端口广播
  if (now - lastMPUReadMs >= MPU_READ_INTERVAL_MS)
  {
    MPUData d;
    if (readMPU(d))
    {
      const float RAD2DEG = 57.29577951308232f;
      float pitch = atan2f(-d.ax, sqrtf(d.ay * d.ay + d.az * d.az)) * RAD2DEG;
      float roll = atan2f(d.ay, d.az) * RAD2DEG;

      bool moved = false;
      if (lastPitch == 10000.0f || lastRoll == 10000.0f)
        moved = true; // 首次发送
      else if (fabsf(pitch - lastPitch) >= MPU_ANGLE_THRESHOLD_DEG || fabsf(roll - lastRoll) >= MPU_ANGLE_THRESHOLD_DEG)
        moved = true;
      else if (fabsf(d.gx) >= MPU_GYRO_THRESHOLD_DPS || fabsf(d.gy) >= MPU_GYRO_THRESHOLD_DPS || fabsf(d.gz) >= MPU_GYRO_THRESHOLD_DPS)
        moved = true;

      if (moved && (now - lastMPUSendMs >= MPU_SEND_MIN_INTERVAL_MS))
      {
        sendMPUDataWS(d);
        lastPitch = pitch;
        lastRoll = roll;
        lastMPUSendMs = now;
      }
    }
    lastMPUReadMs = now;
  }

  // RFID：仅在检测到卡片时处理完整流程
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    delay(10);
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  // 获取卡片UID
  String currentCardUID = getCardUID();
  Serial.print("Card UID: ");
  Serial.println(currentCardUID);

  // 状态机：判断是否是授权卡片
  if (currentCardUID == SD_ID)
  {
    // 切换锁的状态
    isLocked = !isLocked;
    // 保存新状态到闪存
    saveLockState();
    // 广播新状态到 81 端口
    sendLockStateWS();
    // 移动舵机到对应位置（上锁 -90，解锁 +90）
    applyLockServo(isLocked);
    if (isLocked)
    {
      Serial.println("[LOCKED]");
      displayLocked();
      beep(2, 100); // 锁定时滴滴两声
    }
    else
    {
      Serial.println("[UNLOCKED]");
      displayUnlocked();
      beep(2, 100); // 解锁时滴滴两声
    }
  }
  else
  {
    Serial.println("[Unauthorized Card]");

    // 显示警告界面
    displayUnauthorized();

    // 持续间断性滴滴滴，直到检测到新卡片；保持 WebSocket 活跃
    while (true)
    {
      if (mfrc522.PICC_IsNewCardPresent())
      {
        if (mfrc522.PICC_ReadCardSerial())
        {
          break;
        }
      }

      // 处理 WebSocket 循环，保持连接活跃
      webSocket.loop();
      webSocketMPU.loop();

      beep(1, 100); // 每次滴响 100ms
      delay(100);   // 间隔 100ms
    }

    // 恢复之前的界面
    if (isLocked)
    {
      displayLocked();
    }
    else
    {
      displayUnlocked();
    }
  }

  // 停止读卡
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(100); // 延迟1秒，避免重复检测
}

// 蜂鸣器控制函数声明

// 蜂鸣器控制函数
void beep(int times, int duration)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration);
    digitalWrite(BUZZER_PIN, LOW);
    delay(duration);
  }
}