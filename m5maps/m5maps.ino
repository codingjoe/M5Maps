#include <M5EPD.h>
#include <Math.h>

double longitude_deg;
double latitude_deg;
int max_zoom = 16;
int min_zoom = 10;

double x;
double y;
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


  File longitude_file = SD.open("/longitude_deg", "r");
  longitude_deg = atof(longitude_file.readString().c_str());
  longitude_file.close();

  File latitude_file = SD.open("/latitude_deg", "r");
  latitude_deg = atof(latitude_file.readString().c_str());
  latitude_file.close();

  File zoom_file = SD.open("/zoom", "r");
  z = atoi(zoom_file.readString().c_str());
  zoom_file.close();

  Serial.println("Loading position: " + String(longitude_deg, 6) + "," + String(latitude_deg, 6) + "," + String(z));

  getPosition();
}


uint16_t r16(File &f)
{
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t r32(File &f)
{
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}


bool drawBmpFile(FS &fs, const char *path, uint16_t x, uint16_t y)
{
  if ((x >= canvas.width()) || (y >= canvas.height()))
    return 0;

  // Open requested file on SD card
  File bmpFS = fs.open(path, "r");

  if (!bmpFS)
  {
    log_e("File not found");
    return 0;
  }

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t c;

  if (r16(bmpFS) == 0x4D42)
  {
    r32(bmpFS);
    r32(bmpFS);
    seekOffset = r32(bmpFS);
    r32(bmpFS);
    w = r32(bmpFS);
    h = r32(bmpFS);

    if ((r16(bmpFS) == 1) && (r16(bmpFS) == 4) && (r32(bmpFS) == 0))
    {
      y += h - 1;

      bmpFS.seek(seekOffset);
      uint8_t lineBuffer[w / 2];


      for (row = 0; row < h; row++)
      {
        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t *bptr = lineBuffer;
        for (col = 0; col < w; col = col + 2)
        {
          c = *bptr++;
          uint8_t a = (c >> 4) & 0xF;
          uint8_t b = c & 0xF;
          canvas.drawPixel(x + col, y, a);
          canvas.drawPixel(x + col + 1, y, b);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        y--;
      }
      log_d("Loaded in %lu ms", millis() - startTime);
    }
    else
    {
      log_e("BMP format not recognized.");
      return 0;
    }

  }
  bmpFS.close();
  return 1;
}


void getPosition() {



  double lat_rad = (latitude_deg * PI) / 180;
  double n = pow(2.0, z);
  x = n * ((longitude_deg + 180) / 360);
  y = n * (1 - (log(tan(lat_rad) + 1.0 / cos(lat_rad)) / PI)) / 2.0;
}

void setPosition() {
  double n = pow(2.0, z);
  longitude_deg = x / n * 360.0 - 180.0;
  double lat_rad = atan(sinh(PI * (1 - 2 * y / n)));
  latitude_deg = degrees(lat_rad);

  Serial.println("Saving position: " + String(longitude_deg, 6) + "," + String(latitude_deg, 6) + "," + String(z));

  File longitude_file = SD.open("/longitude_deg", FILE_WRITE);
  longitude_file.print(String(longitude_deg, 6));
  longitude_file.close();

  File latitude_file = SD.open("/latitude_deg", FILE_WRITE);
  latitude_file.print(String(latitude_deg, 6));
  latitude_file.close();

  File zoom_file = SD.open("/zoom", FILE_WRITE);
  zoom_file.print(z);
  zoom_file.close();
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
      String url = "/" + String(z) + "/" + String((int) x + col) + "/" + String((int) y + row) + ".bmp";
      Serial.println("===> " + url);
      drawBmpFile(SD, url.c_str() , 256 * (col + 1), 256 * (row + 2));
    }
  }

  drawLegend();
  canvas.pushCanvas(0 , 0 , UPDATE_MODE_GC16);
  Serial.println("Done");
}

void drawLegend() {
  double height = 1000 / calcMetersPerPixel();
  Serial.println(height);
  for (double ly = 0; ly <  960; ly = ly + 2 * height) {
    canvas.fillRect(512, (int) ly, 540 - 512, (int) height, 15);
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
