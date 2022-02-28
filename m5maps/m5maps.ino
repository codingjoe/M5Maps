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


const uint8_t rgbGrayScaleMap[] = {
  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
  14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
  13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
  11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
  10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
  8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


uint8_t rgb2Grayscale(const uint8_t r, const uint8_t g, const uint8_t b) {
  return rgbGrayScaleMap[(r * 38 + g * 75 + b * 15) >> 7];
}

const uint8_t BI_RGB = 0;
const uint8_t BI_RLE8 = 1;
const uint8_t BI_RLE4 = 2;

// http://www.ece.ualberta.ca/~elliott/ee552/studentAppNotes/2003_w/misc/bmp_file_format/bmp_file_format.htm
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

  uint32_t dataOffset;
  uint16_t w, h, row, col;

  if (r16(bmpFS) == 0x4D42) // Signature = BM
  {
    r32(bmpFS); // FileSize
    r32(bmpFS); // reserved
    dataOffset = r32(bmpFS);
    r32(bmpFS); // size
    w = r32(bmpFS);
    h = r32(bmpFS);

    uint16_t planes = r16(bmpFS);
    uint16_t bitsPerPixel = r16(bmpFS);
    float bytesPerPixel = bitsPerPixel / 8.0;
    uint8_t compression = r32(bmpFS);
    r32(bmpFS); // ImageSize
    r32(bmpFS); // XpixelsPerM
    r32(bmpFS); // YpixelsPerM
    r32(bmpFS); // ImportantColors
    uint32_t numColors = r32(bmpFS);

    Serial.printf("BMP %dbit %d colors (compression=%d)\r\n",  bitsPerPixel, numColors, compression);

    if ((planes == 1) && (compression == BI_RGB))
    {

      if (bitsPerPixel <= 8) {
        uint8_t colorTable[numColors];
        for (int i = 0; i < numColors; i++) {
          uint8_t r = bmpFS.read();
          uint8_t g = bmpFS.read();
          uint8_t b = bmpFS.read();
          uint8_t a = bmpFS.read();
          colorTable[i] = rgb2Grayscale(r, g, b);
        }

        y += h - 1;

        bmpFS.seek(dataOffset);

        uint16_t padding = (4 - ((int) (w * bytesPerPixel) & 3)) & 3;
        uint8_t lineBuffer[(int) (w * bytesPerPixel) + padding];


        uint8_t increment = 1 / bytesPerPixel;


        for (row = 0; row < h; row++)
        {
          bmpFS.read(lineBuffer, sizeof(lineBuffer));
          uint8_t *bptr = lineBuffer;
          for (col = 0; col < w; col += increment)
          {
            if (bitsPerPixel == 4) {
              uint8_t c = *bptr++;
              uint8_t a = (c >> 4) & 0xF;
              uint8_t b = c & 0xF;
              canvas.drawPixel(x + col, y, colorTable[a]);
              canvas.drawPixel(x + col + 1, y, colorTable[b]);
              increment = 2;
            }
          }

          // Push the pixel row to screen, pushImage will crop the line if needed
          // y is decremented as the BMP image is drawn bottom up
          y--;
        }
      } else {
        y += h - 1;

        bmpFS.seek(dataOffset);

        uint16_t padding = (4 - ((int) (w * bytesPerPixel) & 3)) & 3;
        uint8_t lineBuffer[(int) (w * bytesPerPixel) + padding];

        for (row = 0; row < h; row++)
        {
          bmpFS.read(lineBuffer, sizeof(lineBuffer));
          uint8_t *bptr = lineBuffer;
          for (col = 0; col < w; col++)
          {
            uint8_t b = *bptr++;
            uint8_t g = *bptr++;
            uint8_t r = *bptr++;
            canvas.drawPixel(x + col, y, rgb2Grayscale(r, g, b));
          }

          // Push the pixel row to screen, pushImage will crop the line if needed
          // y is decremented as the BMP image is drawn bottom up
          y--;
        }

      }
      log_d("Loaded in %lu ms", millis() - startTime);
    }
    else
    {
      log_e("BMP format not supported.");
      bmpFS.close();
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


bool drawTiles() {
  inactive = 0;
  canvas.fillCanvas(0);
  int row;
  int col;
  bool success = true;
  Serial.println("Loading tiles:");
  for (row = -2; row < 2; row++) {
    for (col = -1; col < 1; col++) {
      String url = "/" + String(z) + "/" + String((int) x + col) + "/" + String((int) y + row) + ".bmp";
      Serial.println("===> " + url);
      success = success and (bool) drawBmpFile(SD, url.c_str() , 256 * (col + 1), 256 * (row + 2));
    }
  }

  if (!success) {
    return success;
  }

  drawLegend();
  canvas.pushCanvas(0 , 0 , UPDATE_MODE_GC16);
  setPosition();
  Serial.println("Done");
  return success;
}

void drawLegend() {
  double height = 1000 / calcMetersPerPixel();
  for (double ly = 0; ly <  960; ly = ly + 2 * height) {
    canvas.fillRect(512, (int) ly, 540 - 512, (int) height, 15);
  }
}


void zoom() {

  if (M5.BtnR.wasPressed() and z != max_zoom) {
    z++;
    Serial.println("Zoom out to " + String(z));
    getPosition();
    if (!drawTiles()) {
      z--;
      getPosition();
    }

  }
  if (M5.BtnL.wasPressed() and z != min_zoom) {
    z--;
    Serial.println("Zoom in to " + String(z));
    getPosition();
    if (!drawTiles()) {
      z++;
      getPosition();
    }


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
        int xShift = 0;
        int yShift = 0;
        if (FingerItem.y < 960 / 3) {
          yShift = -1;
        } else if (FingerItem.y > 960 / 3 * 2) {
          yShift = 1;
        }
        if (FingerItem.x < 540 /  3) {
          xShift = -1;
        } else if (FingerItem.x > 540 / 3 * 2) {
          xShift = 1;
        }
        x += xShift;
        y += yShift;
        if (!drawTiles()) {
          x -= xShift;
          y -= yShift;
          getPosition();
        }
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
