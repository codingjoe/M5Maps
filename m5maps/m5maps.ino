#include <M5EPD.h>
#include <WiFi.h>
#include <Math.h>

double longitude_deg = 12.450189;
double latitude_deg = 62.316656;
int zoom = 12;
int max_zoom = 16;
char* wifi_ssid = "";
char* wifi_passwd = "";

M5EPD_Canvas canvas(&M5.EPD);

void setup()
{
  M5.begin(true, true, true, false, false);
  M5.EPD.SetRotation(90);
  canvas.createCanvas(540, 960);
  canvas.setTextSize(5);

  Serial.print("Connecting to WIFI...");
  WiFi.begin(wifi_ssid, wifi_passwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("OK");

}

void drawTiles(double lon_deg, double lat_deg, int z) {
  M5.EPD.Clear(true);
  canvas.fillCanvas(0);
  double lat_rad = (lat_deg * PI) / 180;
  double n = pow(2.0, z);
  int x = (int) (n * ((lon_deg + 180) / 360));
  int y = (int) (n * (1 - (log(tan(lat_rad) + 1.0 / cos(lat_rad)) / PI)) / 2.0);

  int row;
  int col;

  Serial.println("Loading tiles:");
  for (row = 0; row < 4; row++) {
    for (col = 0; col < 3; col++) {
      String url = "https://a.tile.opentopomap.org/" + String(z) + "/" + String(x + col) + "/" + String(y + row) + ".png";
      Serial.println("===> " + url);
      int str_len = url.length() + 1;
      char char_array[str_len];
      url.toCharArray(char_array, str_len);
      canvas.drawPngUrl(char_array , 256 * col, 256 * row);
      M5.EPD.WriteFullGram4bpp((uint8_t*)canvas.frameBuffer());
      M5.EPD.UpdateArea(256 * col, 256 * row, 256 * (col + 1), 256 * (row + 1), UPDATE_MODE_GC16);
    }
  }
  Serial.println("Done");
}

void loop()
{
  M5.update();
  if (M5.BtnL.wasPressed() and zoom != 19) {
    zoom++;
    Serial.println("Zoom out to " + String(zoom));
  }
  if (M5.BtnR.wasPressed() and zoom != 0) {
    zoom--;
    Serial.println("Zoom in to " + String(zoom));

  }
  if (M5.BtnP.wasPressed()) {
    drawTiles(longitude_deg, latitude_deg, zoom);
  }

  delay(250);  // debounce input
}
