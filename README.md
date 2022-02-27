# M5Maps
Map software for the M5 Paper ESP32 eInk development board

### Setup

```shell
pip install -r requirements.txt
```

### Usage

Usage: `download_maps.py [OPTIONS] LON LAT DEST TILE_SRV...`


OSM example:

```shell
./download_maps.py 10.1574377 61.7749463 "/Volumes/NO NAME" "https://a.tile.opentopomap.org/{z}/{x}/{y}.png" "https://b.tile.opentopomap.org/{z}/{x}/{y}.png" "https://c.tile.opentopomap.org/{z}/{x}/{y}.png"
```

Statkart example:

```shell
./download_maps.py 10.1574377 61.7749463 "/Volumes/NO NAME" "https://opencache.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=topo4&zoom={z}&x={x}&y={y}" "https://opencache2.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=topo4&zoom={z}&x={x}&y={y}" "https://opencache3.statkart.no/gatekeeper/gk/gk.open_gmaps?layers=topo4&zoom={z}&x={x}&y={y}"
```
