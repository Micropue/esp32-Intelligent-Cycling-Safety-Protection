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

// 函数：绘制锁定的锁头
void drawLockedLock(int centerX, int centerY)
{
  // 锁弧 - U形闭合
  display.drawCircle(centerX - 6, centerY - 3, 8, SSD1306_WHITE);  // 左圆
  display.drawCircle(centerX + 6, centerY - 3, 8, SSD1306_WHITE);  // 右圆
  for (int i = -6; i <= 6; i++)
  {
    display.drawPixel(centerX + i, centerY - 11, SSD1306_WHITE);   // 锁弧顶部
  }
  
  // 锁体 - 矩形主体
  display.drawRect(centerX - 10, centerY + 3, 20, 12, SSD1306_WHITE);
  
  // 钥孔 - 圆形
  display.fillCircle(centerX, centerY + 8, 2, SSD1306_WHITE);
  display.drawPixel(centerX, centerY + 11, SSD1306_WHITE);  // 钥孔下面的线
  display.drawPixel(centerX, centerY + 12, SSD1306_WHITE);
}

// 函数：绘制解锁的锁头
void drawUnlockedLock(int centerX, int centerY)
{
  // 锁弧 - U形打开
  display.drawCircle(centerX - 6, centerY - 3, 8, SSD1306_WHITE);  // 左圆
  display.drawCircle(centerX + 6, centerY - 3, 8, SSD1306_WHITE);  // 右圆
  // 只绘制左右两边
  for (int i = -8; i <= 0; i++)
  {
    display.drawPixel(centerX - 8, centerY - 3 + i, SSD1306_WHITE);   // 左边
  }
  for (int i = 0; i <= 8; i++)
  {
    display.drawPixel(centerX + 8, centerY - 3 + i, SSD1306_WHITE);   // 右边
  }
  
  // 锁体 - 矩形主体
  display.drawRect(centerX - 10, centerY + 3, 20, 12, SSD1306_WHITE);
  
  // 钥孔 - 圆形
  display.fillCircle(centerX, centerY + 8, 2, SSD1306_WHITE);
  display.drawPixel(centerX, centerY + 11, SSD1306_WHITE);  // 钥孔下面的线
  display.drawPixel(centerX, centerY + 12, SSD1306_WHITE);
}

// 函数：绘制护盾图标
void drawShieldIcon(int x, int y)
{
  // 盾牌形：上圆弧，下梯形
  // 绘制顶部圆弧
  for (int i = -5; i <= 5; i++)
  {
    int yOffset = (i * i) / 10;  // 二次曲线近似
    display.drawPixel(x + i, y - yOffset, SSD1306_WHITE);
  }
  
  // 绘制左边
  display.drawLine(x - 5, y, x - 4, y + 6, SSD1306_WHITE);
  
  // 绘制右边
  display.drawLine(x + 5, y, x + 4, y + 6, SSD1306_WHITE);
  
  // 绘制底部
  display.drawLine(x - 4, y + 6, x + 4, y + 6, SSD1306_WHITE);
  
  // 绘制中间的竖线装饰
  display.drawLine(x, y, x, y + 5, SSD1306_WHITE);
}

// 函数：绘制自行车图标
void drawBicycleIcon(int x, int y)
{
  // 左轮
  display.drawCircle(x + 3, y, 4, SSD1306_WHITE);
  display.drawPixel(x + 3, y - 1, SSD1306_WHITE);
  display.drawPixel(x + 3, y + 1, SSD1306_WHITE);
  display.drawPixel(x + 2, y, SSD1306_WHITE);
  display.drawPixel(x + 4, y, SSD1306_WHITE);
  
  // 右轮
  display.drawCircle(x + 13, y, 4, SSD1306_WHITE);
  display.drawPixel(x + 13, y - 1, SSD1306_WHITE);
  display.drawPixel(x + 13, y + 1, SSD1306_WHITE);
  display.drawPixel(x + 12, y, SSD1306_WHITE);
  display.drawPixel(x + 14, y, SSD1306_WHITE);
  
  // 车架 - 从左轮到右轮的斜线
  display.drawLine(x + 7, y - 3, x + 3, y, SSD1306_WHITE);  // 左上支撑
  display.drawLine(x + 7, y - 3, x + 13, y, SSD1306_WHITE); // 右上支撑
  
  // 座椅
  display.drawPixel(x + 8, y - 4, SSD1306_WHITE);
  display.drawPixel(x + 9, y - 4, SSD1306_WHITE);
  
  // 把手
  display.drawPixel(x + 3, y - 5, SSD1306_WHITE);
  display.drawPixel(x + 3, y - 6, SSD1306_WHITE);
}

// 函数：显示上锁界面
void displayLocked()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // 绘制锁定的锁头
  drawLockedLock(64, 18);
  
  // 下方显示护盾图标和文字
  display.setTextSize(1);
  drawShieldIcon(10, 50);
  display.setCursor(25, 52);
  display.println("Under Surveillance");
  
  display.display();
}

// 函数：显示解锁界面
void displayUnlocked()
{
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // 绘制解锁的锁头
  drawUnlockedLock(64, 18);
  
  // 下方显示自行车图标和文字
  display.setTextSize(1);
  drawBicycleIcon(10, 54);
  display.setCursor(28, 52);
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

  delay(1000); // 延迟1秒，避免重复检测
}