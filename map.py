#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
map_carto.py

Интерактивная карта (CartoDB) + вкладка с периодическим отображением getted.png.
Send -> POST /send -> сервер печатает JSON в stdout и записывает coords.txt в формате:
<count>
lat1, lon1, 150
lat2, lon2, 150
...

ВАЖНО: index.html и coords.txt создаются в той же папке, где находится этот скрипт.
"""

import http.server
import socketserver
import threading
import json
import sys
import os
import socket
import webbrowser
from urllib.parse import urlparse

# --- Настройки ---
HOST = "127.0.0.1"
IMAGE_FILENAME = "getted.png"
OUTPUT_COORDS_FILE = "coords.txt"

# Директория, где лежит этот скрипт — туда будем записывать index.html и coords.txt
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# HTML интерфейс (CartoDB)
INDEX_HTML = r"""<!doctype html>
<html>
<head>
<meta charset="utf-8">
<title>Карта (CartoDB) и изображение</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" crossorigin=""/>
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js" crossorigin=""></script>

<style>
  html, body { height:100%; margin:0; padding:0; font-family:Arial, sans-serif; }
  .tabs { display:flex; gap:8px; padding:8px; background:#f2f2f2; align-items:center; box-sizing:border-box; border-bottom:1px solid #ddd; }
  .tabs button { padding:8px 12px; border:1px solid #ccc; background:white; cursor:pointer; border-radius:4px; }
  .tabs button.active { background:#007acc; color:white; border-color:#007acc; }
  .content { height: calc(100% - 56px); }
  #panel-map, #panel-image { width:100%; height:100%; display:flex; flex-direction:column; box-sizing:border-box; }
  .map-controls { padding:8px; display:flex; gap:8px; align-items:center; background:transparent; box-sizing:border-box; }
  .send-btn { padding:8px 10px; border-radius:4px; border:1px solid #666; background:#eee; cursor:pointer; }
  .info { margin-left:auto; font-size:13px; color:#222; padding-right:6px; }
  #map { flex:1; min-height:200px; width:100%; }
  #imgwrap { width:100%; height:100%; display:flex; align-items:center; justify-content:center; background:#111; box-sizing:border-box; }
  #imgwrap img { max-width:100%; max-height:100%; display:block; }
  #loading { color:white; text-align:center; padding:12px; }
  @media (max-width:600px) { .tabs { gap:6px; padding:6px; } .map-controls { padding:6px; gap:6px; } }
</style>
</head>
<body>
<div class="tabs">
  <button id="tab-map" class="active">Карта</button>
  <button id="tab-image">Изображение</button>
</div>

<div class="content">
  <div id="panel-map">
    <div class="map-controls">
      <button id="sendBtn" class="send-btn">Send</button>
      <button id="clearBtn" class="send-btn">Clear</button>
      <div class="info">Маркер(ов): <span id="count">0</span></div>
    </div>
    <div id="map"></div>
  </div>

  <div id="panel-image" style="display:none;">
    <div id="imgwrap">
      <div id="loading">Пробуем загрузить getted.png...</div>
      <img id="theimage" src="" style="display:none"/>
    </div>
  </div>
</div>

<script>
(function(){
  const tabMapBtn = document.getElementById("tab-map");
  const tabImageBtn = document.getElementById("tab-image");
  const panelMap = document.getElementById("panel-map");
  const panelImage = document.getElementById("panel-image");

  tabMapBtn.onclick = () => {
    tabMapBtn.classList.add("active");
    tabImageBtn.classList.remove("active");
    panelMap.style.display = "";
    panelImage.style.display = "none";
    setTimeout(() => { try { map.invalidateSize(); } catch(e){} }, 200);
  };
  tabImageBtn.onclick = () => {
    tabImageBtn.classList.add("active");
    tabMapBtn.classList.remove("active");
    panelImage.style.display = "";
    panelMap.style.display = "none";
  };

  // Leaflet map
  const map = L.map('map', { zoomControl:true }).setView([55.753215, 37.622504], 5);

  // CartoDB basemap (Voyager)
  const cartoVoyager = L.tileLayer('https://{s}.basemaps.cartocdn.com/rastertiles/voyager/{z}/{x}/{y}{r}.png', {
    attribution: '&copy; CARTO',
    maxZoom: 19
  }).addTo(map);

  let markers = [];
  function updateCount(){ document.getElementById("count").textContent = markers.length; }

  map.on('click', function(e){
    const m = L.marker(e.latlng, {riseOnHover:true}).addTo(map);
    m.on('contextmenu', function(ev){
      map.removeLayer(m);
      markers = markers.filter(x => x !== m);
      updateCount();
    });
    markers.push(m);
    updateCount();
  });

  document.getElementById("clearBtn").addEventListener("click", function(){
    markers.forEach(m => map.removeLayer(m));
    markers = [];
    updateCount();
  });

  document.getElementById("sendBtn").addEventListener("click", async function(){
    const pts = markers.map(m => {
      const latlng = m.getLatLng();
      return { lat: Number(latlng.lat.toFixed(7)), lon: Number(latlng.lng.toFixed(7)) };
    });
    const payload = { points: pts, provider: "carto_voyager", ts: (new Date()).toISOString() };
    try {
      const resp = await fetch('/send', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(payload)
      });
      if (!resp.ok) throw new Error("Server returned " + resp.status);
      const text = await resp.text();
      alert("Отправлено: " + pts.length + " точек.\nОтвет сервера: " + text);
    } catch (err) {
      alert("Ошибка отправки: " + err);
    }
  });

  // Image reload (getted.png in server root)
  const imgEl = document.getElementById("theimage");
  const loading = document.getElementById("loading");
  function refreshImage(){
    const url = 'build/getted.png?ts=' + Date.now();
    const tmp = new Image();
    tmp.onload = function(){
      imgEl.src = url;
      imgEl.style.display = "";
      loading.style.display = "none";
    };
    tmp.onerror = function(){
      imgEl.style.display = "none";
      loading.style.display = "";
      loading.textContent = "Файл 'getted.png' не найден в папке программы.";
    };
    tmp.src = url;
  }
  setInterval(refreshImage, 3000);
  refreshImage();

  // initial invalidate
  setTimeout(() => { try { map.invalidateSize(); } catch(e){} }, 300);
})();
</script>
</body>
</html>
"""

# --- HTTP handler ---
class Handler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path == '/send':
            length = int(self.headers.get('Content-Length', 0))
            raw = self.rfile.read(length) if length > 0 else b''
            try:
                data = json.loads(raw.decode('utf-8'))
            except Exception as e:
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b'Invalid JSON: ' + str(e).encode('utf-8'))
                return

            # Print received JSON to stdout (human-readable)
            try:
                print(json.dumps(data, ensure_ascii=False, indent=2))
                sys.stdout.flush()
            except Exception:
                pass

            # Save coords to coords.txt in SCRIPT_DIR in required format:
            try:
                points = data.get("points", [])
                coords_path = os.path.join(SCRIPT_DIR + "/build/", OUTPUT_COORDS_FILE)
                with open(coords_path, "w", encoding="utf-8") as f:
                    f.write(str(len(points)) + "\n")
                    for p in points:
                        lat = p.get("lat", None)
                        lon = p.get("lon", None)
                        if lat is None and "y" in p:
                            lat = p.get("y")
                        if lon is None and "x" in p:
                            lon = p.get("x")
                        try:
                            latf = float(lat)
                            lonf = float(lon)
                            f.write(f"{latf:.7f} {lonf:.7f} 150\n")
                        except Exception:
                            f.write(f"{lat} {lon} 150\n")
                print(f"Wrote {len(points)} points to {coords_path}")
                sys.stdout.flush()
            except Exception as e:
                print("Error writing coords file:", e, file=sys.stderr)
                sys.stdout.flush()
                self.send_response(500)
                self.end_headers()
                self.wfile.write(b'Failed to write coords file')
                return

            # Respond OK
            self.send_response(200)
            self.send_header("Content-Type","text/plain; charset=utf-8")
            self.end_headers()
            self.wfile.write(b'OK')
        else:
            # For other POST paths, return 404
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b'Not found')

# --- Utility functions ---
def find_free_port():
    s = socket.socket()
    s.bind((HOST, 0))
    addr, port = s.getsockname()
    s.close()
    return port

def write_index_html(path):
    with open(path, "w", encoding="utf-8") as f:
        f.write(INDEX_HTML)

def start_server_and_open_ui():
    # ensure index.html is written into the script directory
    index_path = os.path.join(SCRIPT_DIR, "index.html")
    write_index_html(index_path)

    # Change working directory to script dir so SimpleHTTPRequestHandler serves files from there
    os.chdir(SCRIPT_DIR)

    port = find_free_port()

    def server_thread():
        with socketserver.TCPServer((HOST, port), Handler) as httpd:
            httpd.allow_reuse_address = True
            print(f"Serving HTTP on {HOST}:{port} (serving directory: {SCRIPT_DIR})")
            sys.stdout.flush()
            try:
                httpd.serve_forever()
            except KeyboardInterrupt:
                pass

    t = threading.Thread(target=server_thread, daemon=True)
    t.start()

    url = f"http://{HOST}:{port}/index.html"
    try:
        import webview
        window = webview.create_window("Карта (CartoDB) и изображение", url, width=1100, height=800)
        webview.start()
    except Exception as e:
        print("pywebview unavailable or failed to start; opening default browser. Error:", e, file=sys.stderr)
        webbrowser.open(url)
        try:
            # keep main thread alive while server runs
            while True:
                t.join(1)
        except KeyboardInterrupt:
            print("Exit.")
            return

if __name__ == "__main__":
    print("Script directory:", SCRIPT_DIR)
    start_server_and_open_ui()
