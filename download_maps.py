#!/usr/bin/env python3
import math
import sys
import urllib.request
import os


def deg2num(lat_deg, lon_deg, zoom):
    lat_rad = math.radians(lat_deg)
    n = 2.0 ** zoom
    x = int((lon_deg + 180.0) / 360.0 * n)
    y = int((1.0 - math.asinh(math.tan(lat_rad)) / math.pi) / 2.0 * n)
    return x, y


def main(lon, lat, path):
    """
    Download maps from the command line.
    """


    print("Downloading tiles...")
    for rad in range(5):
        z = rad + 12
        x, y = deg2num(float(lat), float(lon), z)
        for i in range(-(2 ** rad), (2 ** rad) + 1):
            full_path = os.path.join(path, f"{z}/{x + i}/")
            os.makedirs(full_path, exist_ok=True)
            for j in range(-(2 ** rad), (2 ** rad) + 1):
                file_path = os.path.join(full_path, f"{y + j}.png")
                if os.path.exists(file_path):
                    print(f"File already exists: {file_path}")
                else:
                    print(f"https://a.tile.opentopomap.org/{z}/{x + i}/{y + j}.png -> {file_path}")
                    urllib.request.urlretrieve(f"https://a.tile.opentopomap.org/{z}/{x + i}/{y + j}.png", file_path)
    print("\nDone.")


if __name__ == "__main__":
    main(*sys.argv[1:])
