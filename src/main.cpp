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

// 函数：显示上锁界面
void displayLocked()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // 绘制锁头（简单的ASCII艺术）
  display.setTextSize(2);
  display.setCursor(45, 10);
  display.println("[]");
  display.setCursor(40, 26);
  display.println("[==]");
  display.setCursor(45, 42);
  display.println("||");
  
  // 下方显示护盾图标和文字
  display.setTextSize(1);
  display.setCursor(5, 55);
  display.println("[#]");  // 护盾图标
  display.setCursor(45, 55);
  display.println("Under Surveillance");
  
  display.display();
}

// 函数：显示解锁界面
void displayUnlocked()
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // 绘制已解锁的锁头（简单的ASCII艺术）
  display.setTextSize(2);
  display.setCursor(45, 10);
  display.println("[ ]");
  display.setCursor(40, 26);
  display.println("[  ]");
  display.setCursor(45, 42);
  display.println("||");
  
  // 下方显示自行车图标和文字
  display.setTextSize(1);
  display.setCursor(5, 55);
  display.println("(o)");  // 自行车图标
  display.setCursor(35, 55);
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