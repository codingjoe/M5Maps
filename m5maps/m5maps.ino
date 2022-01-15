#include <M5EPD.h>
#include <Math.h>

double longitude_deg = 12.450189;
double latitude_deg = 62.316656;
int max_zoom = 16;

int x;
int y;
int z = 14; // 14 ~35000:1


int point[2];
int inactive = 0;
int loop_delay = 100;

M5EPD_Canvas canvas(&M5.EPD);

uint16_t r16(fs::File &f)
{
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t r32(fs::File &f)
{
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void setup()
{
  M5.begin(true, true, true, false, false);
  M5.EPD.SetRotation(90);
  M5.TP.SetRotation(90);
  canvas.createCanvas(540, 960);
  canvas.setTextSize(5);

  calcPos();


}

void calcPos() {
  double lat_rad = (latitude_deg * PI) / 180;
  double n = pow(2.0, z);
  x = (int) (n * ((longitude_deg + 180) / 360));
  y = (int) (n * (1 - (log(tan(lat_rad) + 1.0 / cos(lat_rad)) / PI)) / 2.0);
}


void drawTiles() {
  inactive = 0;
  M5.EPD.Clear(true);
  canvas.fillCanvas(0);
  int row;
  int col;
  Serial.println("Loading tiles:");
  for (row = 0; row < 4; row++) {
    for (col = 0; col < 3; col++) {
      String url = "/" + String(z) + "/" + String(x + col) + "/" + String(y + row) + ".png";
      Serial.println("===> " + url);
      canvas.drawPngFile(SD, url.c_str() , 256 * col, 256 * row);
    }
  }
  canvas.pushCanvas(0 , 0 , UPDATE_MODE_GC16);
  Serial.println("Done");
}


void zoom() {

  if (M5.BtnL.wasPressed() and z != 19) {
    z++;
    Serial.println("Zoom out to " + String(z));
    calcPos();
    drawTiles();
  }
  if (M5.BtnR.wasPressed() and z != 0) {
    z--;
    Serial.println("Zoom in to " + String(z));
    calcPos();
    drawTiles();


  }
  if (M5.BtnP.wasPressed()) {
    drawTiles();
  }


}


void touchInput () {
  if (M5.TP.avaliable()) {
    if (!M5.TP.isFingerUp()) {
      M5.TP.update();
      tp_finger_t FingerItem = M5.TP.readFinger(0);
      if ((point[0] != FingerItem.x) || (point[1] != FingerItem.y)) {
        point[0] = FingerItem.x;
        point[1] = FingerItem.y;
        if (FingerItem.y < 960 / 3) {
          y--;
        } else if (FingerItem.y > 960 / 3 * 2) {
          y++;
        }
        if (FingerItem.x < 540 /  3) {
          x--;
        } else if (FingerItem.x > 540 / 3 * 2) {
          x++;
        }
        drawTiles();
        Serial.printf("Finger ID:%d-->X: %d*C  Y: %d  Size: %d\r\n", FingerItem.id, FingerItem.x, FingerItem.y , FingerItem.size);
      }
    }
  }

}

void loop()
{
  inactive++;

  zoom();
  touchInput();

  if (inactive == 60 * 1000 / loop_delay) {
    Serial.println("Shuting down");
    M5.shutdown();
    // prevent int overflow on battery as shutdown will have no effect
    inactive = 0;
  }


  M5.update();
  delay(loop_delay);

}
