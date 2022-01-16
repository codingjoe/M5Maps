#include <M5EPD.h>
#include <Math.h>

double longitude_deg;
double latitude_deg;
int max_zoom = 16;
int min_zoom = 10;

int x;
int y;
int z = 14; // 14 ~35000:1


int point[2];
int inactive = 0;
int loop_delay = 100;

M5EPD_Canvas canvas(&M5.EPD);

void setup()
{
  M5.begin(true, true, true, false, false);
  M5.EPD.SetRotation(90);
  M5.TP.SetRotation(90);
  canvas.createCanvas(540, 960);
  canvas.setTextSize(5);


  getPosition();
}

void getPosition() {
  File longitude_file = SD.open("/longitude_deg", "r");
  longitude_deg = atof(longitude_file.readString().c_str());
  longitude_file.close();
  
  File latitude_file = SD.open("/latitude_deg", "r");
  latitude_deg = atof(latitude_file.readString().c_str());
  latitude_file.close();

  Serial.println("Loading position: " + String(longitude_deg, 6) + "," + String(latitude_deg, 6));
  
  double lat_rad = (latitude_deg * PI) / 180;
  double n = pow(2.0, z);
  x = (int) (n * ((longitude_deg + 180) / 360));
  y = (int) (n * (1 - (log(tan(lat_rad) + 1.0 / cos(lat_rad)) / PI)) / 2.0);
}

void setPosition() {
  double n = pow(2.0, z);
  longitude_deg = x / n * 360.0 - 180.0;
  double lat_rad = atan(sinh(PI * (1 - 2 * y / n)));
  latitude_deg = degrees(lat_rad);

  Serial.println("Saving position: " + String(longitude_deg, 6) + "," + String(latitude_deg, 6));
  
  File longitude_file = SD.open("/longitude_deg", FILE_WRITE);
  longitude_file.print(String(longitude_deg, 6));
  longitude_file.close();
  
  File latitude_file = SD.open("/latitude_deg", FILE_WRITE);
  latitude_file.print(String(latitude_deg, 6));
  latitude_file.close();
}

double calcMetersPerPixel() {
  double C = 2 * PI * 6378137.0;
  return (C * cos(latitude_deg)) / pow(2, (z + 8));
}


void drawTiles() {
  inactive = 0;
  setPosition();
  canvas.fillCanvas(0);
  int row;
  int col;
  Serial.println("Loading tiles:");
  for (row = -2; row < 2; row++) {
    for (col = -1; col < 1; col++) {
      String url = "/" + String(z) + "/" + String(x + col) + "/" + String(y + row) + ".png";
      Serial.println("===> " + url);
      canvas.drawPngFile(SD, url.c_str() , 256 * (col + 1), 256 * (row + 2));
    }
  }

  drawLegend();
  canvas.pushCanvas(0 , 0 , UPDATE_MODE_GC16);
  Serial.println("Done");
}

void drawLegend() {
  double height = 1000 / calcMetersPerPixel();
  Serial.println(height);
  for (double y = 0; y <  960; y = y + 2 * height) {
    canvas.fillRect(512, (int) y, 540 - 512, (int) height, 15);
  }
}


void zoom() {

  if (M5.BtnR.wasPressed() and z != max_zoom) {
    z++;
    Serial.println("Zoom out to " + String(z));
    getPosition();
    drawTiles();
  }
  if (M5.BtnL.wasPressed() and z != min_zoom) {
    z--;
    Serial.println("Zoom in to " + String(z));
    getPosition();
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
