#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

// 针脚定义
#define RST_PIN 27 // RST -> GPIO27
#define SS_PIN 5   // SDA -> GPIO5
#define SDA_PIN 25 // OLED SDA
#define SCL_PIN 32 // OLED SCL
#define SD_ID "91-b3-07-7c"
#define LOCKED_STRING "[LOCKED]"
#define UNLOCKED_STRING "[UNLOCKED]"

// OLED定义
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

// 持久化存储实例
Preferences preferences;

// OLED实例
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// 状态机 - 锁的状态
bool isLocked = true; // true: 上锁，false: 解锁

// 创建MFRC522实例
MFRC522 mfrc522(SS_PIN, RST_PIN);

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

// 函数：显示上锁界面
void displayLocked()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // 绘制锁定的锁头 (居中，位置微调)
  drawLockedLock(64, 22);
  
  // 下方显示文字 (已移除图标，文字居中)
  display.setTextSize(1);
  display.setCursor(10, 50); // 微调居中 (128 - 18字符*6像素) / 2 = 10
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
  
  // 下方显示文字 (已移除图标，文字居中)
  display.setTextSize(1);
  display.setCursor(7, 50); // 微调居中 (128 - 19字符*6像素) / 2 = ~7
  display.println("Cycling Safety Mode");
  
  display.display();
}

void setup()
{
  Serial.begin(115200);
  delay(1000);

  // 初始化I2C和OLED
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.setTextColor(SSD1306_WHITE);

  // 初始化Preferences并读取保存的状态
  preferences.begin("lock_app", true);              // true=只读模式加快速度
  isLocked = preferences.getBool("isLocked", true); // 默认值为上锁
  preferences.end();

  // 初始化SPI
  SPI.begin(18, 19, 23, 5); // SCK, MISO, MOSI, CS

  // 初始化MFRC522
  mfrc522.PCD_Init();
  if (isLocked)
  {
    Serial.println(LOCKED_STRING);
    displayLocked();
  }
  else
  {
    Serial.println(UNLOCKED_STRING);
    displayUnlocked();
  }
}

void loop()
{
  // 检查是否有新卡片
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }

  // 选择卡片
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
    if (isLocked)
    {
      Serial.println("[LOCKED]");
      displayLocked();
    }
    else
    {
      Serial.println("[UNLOCKED]");
      displayUnlocked();
    }
  }
  else
  {
    Serial.println("[Unauthorized Card]");
  }

  // 停止读卡
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(100); // 延迟0.1秒，避免重复检测
}