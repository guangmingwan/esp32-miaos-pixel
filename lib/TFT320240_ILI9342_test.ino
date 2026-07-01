/*
 * ILI9342C 屏幕软件 SPI 测试 (ESP32-S3)
 * 直接使用商家初始化序列，不依赖 TFT_eSPI 库
 * 引脚连接：
 * DC   -> GPIO9
 * CS   -> GPIO10
 * MOSI -> GPIO11
 * CLK  -> GPIO12
 * RST  -> GPIO3
 * BL   -> GPIO13
 */

// 引脚定义
#define LCD_DC    9
#define LCD_CS    10
#define LCD_MOSI  11
#define LCD_SCK   12
#define LCD_RST   3
#define LCD_BL    13

// 屏幕分辨率（ILI9342C 通常为 320x240）
#define TFT_WIDTH  320
#define TFT_HEIGHT 240

// 软件 SPI 写一个字节（MSB first）
void spi_write(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(LCD_SCK, LOW);
    digitalWrite(LCD_MOSI, (data & 0x80) ? HIGH : LOW);
    data <<= 1;
    digitalWrite(LCD_SCK, HIGH);
  }
}

// 发送命令
void write_cmd(uint8_t cmd) {
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_DC, LOW);   // 命令模式
  spi_write(cmd);
  digitalWrite(LCD_CS, HIGH);
}

// 发送数据（8位）
void write_data(uint8_t data) {
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_DC, HIGH);  // 数据模式
  spi_write(data);
  digitalWrite(LCD_CS, HIGH);
}

// 发送16位数据（RGB565 颜色）
void write_data16(uint16_t data) {
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_DC, HIGH);
  spi_write(data >> 8);    // 高8位
  spi_write(data & 0xFF);  // 低8位
  digitalWrite(LCD_CS, HIGH);
}

// 设置显示窗口（列地址和行地址）
void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  write_cmd(0x2A);  // 列地址设置
  write_data(x0 >> 8);
  write_data(x0 & 0xFF);
  write_data(x1 >> 8);
  write_data(x1 & 0xFF);

  write_cmd(0x2B);  // 行地址设置
  write_data(y0 >> 8);
  write_data(y0 & 0xFF);
  write_data(y1 >> 8);
  write_data(y1 & 0xFF);

  write_cmd(0x2C);  // 开始写入内存
}

// 填充整个屏幕为指定颜色（RGB565）
void fill_screen(uint16_t color) {
  set_window(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
  for (uint32_t i = 0; i < (uint32_t)TFT_WIDTH * TFT_HEIGHT; i++) {
    write_data16(color);
  }
}

// 屏幕初始化（商家提供的序列）
void lcd_init() {
  // 硬件复位
  pinMode(LCD_RST, OUTPUT);
  digitalWrite(LCD_RST, HIGH);
  delay(1);
  digitalWrite(LCD_RST, LOW);
  delay(10);
  digitalWrite(LCD_RST, HIGH);
  delay(120);

  // 发送初始化命令
  write_cmd(0xC8);
  write_data(0xFF);
  write_data(0x93);
  write_data(0x42);

  write_cmd(0x36);      // 内存访问控制
  write_data(0xC8);     // MY=1, MX=1, MV=0, ML=0, BGR=1, MH=0

  write_cmd(0x3A);      // 像素格式
  write_data(0x55);     // 16位色 (RGB565)

  write_cmd(0xC0);      // 电源控制1
  write_data(0x10);
  write_data(0x10);

  write_cmd(0xC1);      // 电源控制2
  write_data(0x01);

  write_cmd(0xC5);      // VCOM 控制
  write_data(0xCD);

  write_cmd(0xB1);      // 帧率控制
  write_data(0x00);
  write_data(0x1B);

  write_cmd(0xB4);      // 反转控制
  write_data(0x02);

  // 正电压伽马校正
  write_cmd(0xE0);
  uint8_t gamma_pos[] = {0x0F,0x14,0x17,0x07,0x16,0x0A,0x3F,0x68,0x4C,0x06,0x0F,0x0D,0x18,0x1A,0x00};
  for (uint8_t i = 0; i < 15; i++) write_data(gamma_pos[i]);

  // 负电压伽马校正
  write_cmd(0xE1);
  uint8_t gamma_neg[] = {0x00,0x29,0x29,0x04,0x0F,0x04,0x3C,0x24,0x4B,0x02,0x0B,0x09,0x32,0x37,0x0F};
  for (uint8_t i = 0; i < 15; i++) write_data(gamma_neg[i]);

  write_cmd(0x11);      // 退出睡眠
  delay(120);

  write_cmd(0x29);      // 开启显示
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("ILI9342C 软件 SPI 测试开始");

  // 初始化控制引脚
  pinMode(LCD_DC, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_MOSI, OUTPUT);
  pinMode(LCD_SCK, OUTPUT);
  pinMode(LCD_BL, OUTPUT);

  digitalWrite(LCD_CS, HIGH);   // 片选默认高电平
  digitalWrite(LCD_DC, HIGH);
  digitalWrite(LCD_SCK, LOW);
  digitalWrite(LCD_MOSI, LOW);
  digitalWrite(LCD_BL, LOW);   // 点亮背光（若背光极性相反，改为 LOW）

  lcd_init();
  Serial.println("初始化完成");

  // 依次显示纯色测试
  fill_screen(0xF800);  // 红色
  delay(2000);
  fill_screen(0x07E0);  // 绿色
  delay(2000);
  fill_screen(0x001F);  // 蓝色
  delay(2000);
  fill_screen(0xFFFF);  // 白色
  delay(2000);
  fill_screen(0x0000);  // 黑色
  Serial.println("测试结束");
}

void loop() {
  // 可在此添加动态效果
}