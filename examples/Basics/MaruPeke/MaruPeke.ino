#include <M5Core2.h>

const uint16_t BoardColor = TFT_DARKGREEN;

void onClick_zone(HotZone_t* zone, HotZone_t::EventArgs_t* e)  {
  const int border1 = 10, border2 = border1 / 1.41F;
  const int margin = 5;
  static int touchCount = 0;
  auto z = (HotZoneCircle_t*)zone;
  if(++touchCount % 2)  { // Maru
    M5.Lcd.fillCircle(z->x, z->y, z->r - margin, RED);
    M5.Lcd.fillCircle(z->x, z->y, z->r - border1 - margin, BoardColor);
  } else {                // Peke
    int r = z->r - margin * 2;
    M5.Lcd.fillRect(z->x - r, z->y - r, r * 2, r * 2, YELLOW);
    M5.Lcd.fillTriangle(z->x, z->y - border2, z->x - r, z->y - r - border2, z->x + r, z->y - r - border2, BoardColor);
    M5.Lcd.fillTriangle(z->x, z->y + border2, z->x - r, z->y + r + border2, z->x + r, z->y + r + border2, BoardColor);
    M5.Lcd.fillTriangle(z->x - border2, z->y, z->x - r - border2, z->y - r, z->x - r - border2, z->y + r, BoardColor);
    M5.Lcd.fillTriangle(z->x + border2, z->y, z->x + r + border2, z->y - r, z->x + r + border2, z->y + r, BoardColor);
  }
  z->enabled = false;
}

void setup() {
  M5.begin(true, false, true, false);
  M5.Lcd.fillScreen(BLACK);
  M5.Touch.BtnB.onClick = [] (HotZone_t* zone, HotZone_t::EventArgs_t* e) {ESP.restart();};

// drawTable
  const int box_w = 100, box_h = 60, box_c = 3, box_r = 3;
  const int border = 3;
  const int margin_w = (320 - (box_w * box_c)) / 2;
  const int margin_h = (240 - (box_h * box_r)) / 2;  
  M5.Lcd.fillRect(margin_w - border / 2, margin_h - border / 2, box_w * box_c + (border / 2) * 2, box_h * box_r + (border / 2) * 2, WHITE);
  for(int x = margin_w; x < 320 - box_w; x += box_w) for(int y = margin_h; y < 240 - box_h; y += box_h) {
    M5.Lcd.fillRect(x + border / 2, y + border / 2, box_w - (border / 2) * 2, box_h - (border / 2) * 2, BoardColor);
    auto zone = M5.Touch.addHotZone(x + box_w / 2, y + box_h / 2, box_h / 2);
    zone->onClick = onClick_zone;
  }
}

void loop() {
  M5.Touch.handle();    // .update() のほうがいい？
}
