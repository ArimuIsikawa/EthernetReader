#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
map_carto.py

CartoDB map + image tab + 3 persistent route slots with colors +
MAVLink/UDP live + TCP image receiver (saves image.png in script dir).

Files created/read in the directory where this script resides:
  - index.html
  - image.png (written by TCP receiver)
  - coords.txt (written on Send)
  - route_1.json .. route_3.json
"""

import http.server
import socketserver
import struct
import threading
import json
import sys
import os
import socket
from typing import List
import webbrowser
import time
from urllib.parse import urlparse, parse_qs

# --- Config ---
HOST = "127.0.0.1"
IMAGE_FILENAME = "image.png"
OUTPUT_COORDS_FILE = "coords.txt"
ROUTE_FILENAME_TEMPLATE = "route_{}.json"  # 1..3

# UDP / MAVLink settings (default port, can override via env)
DEFAULT_UDP_PORT = 14558
UDP_PORT = int(os.environ.get("UDP_PORT", str(DEFAULT_UDP_PORT)))
UDP_BIND_ADDR = "0.0.0.0"

# TCP image receiver settings (default port 6006, can override via env)
DEFAULT_IMAGE_TCP_PORT = 14519
IMAGE_TCP_PORT = int(os.environ.get("IMAGE_TCP_PORT", str(DEFAULT_IMAGE_TCP_PORT)))
IMAGE_TCP_BIND = "127.0.0.1"

# Directory of this script
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Colors for slots 1..3
ROUTE_COLORS = ["#e34a4a", "#2a9d8f", "#f4a261"]

# Shared variable for latest UDP/MAVLink point (thread-safe)
_udp_lock = threading.Lock()
_udp_latest = None  # dict { "lat": float, "lon": float, "ts": iso string, "raw": str, "from": "ip:port" }

# ---------- MAVLink listener (preferred) ----------
def mavlink_listener(bind_addr: str, port: int):
    global _udp_latest
    try:
        from pymavlink import mavutil
    except Exception as e:
        print("pymavlink not available:", e, file=sys.stderr)
        return False

    conn_str = f'udp:{bind_addr}:{port}'
    try:
        print("Opening MAVLink UDP on", conn_str)
        sys.stdout.flush()
        conn = mavutil.mavlink_connection(conn_str, source_system=255)
    except Exception as e:
        print("Failed to open MAVLink UDP:", e, file=sys.stderr)
        return False

    print("MAVLink listener started (waiting for messages)...")
    sys.stdout.flush()
    while True:
        try:
            msg = conn.recv_match(timeout=2)
        except Exception as e:
            print("MAVLink recv error:", e, file=sys.stderr)
            time.sleep(0.5)
            continue

        if msg is None:
            continue

        try:
            mtype = msg.get_type()
        except Exception:
            mtype = None

        lat = None
        lon = None

        if mtype == 'GLOBAL_POSITION_INT':
            try:
                lat = float(msg.lat) / 1e7
                lon = float(msg.lon) / 1e7
            except Exception:
                lat = None; lon = None
        elif mtype == 'GPS_RAW_INT':
            try:
                lat = float(msg.lat) / 1e7
                lon = float(msg.lon) / 1e7
            except Exception:
                lat = None; lon = None
        else:
            # generic tries
            if hasattr(msg, 'lat') and hasattr(msg, 'lon'):
                try:
                    lat = float(getattr(msg, 'lat'))
                    lon = float(getattr(msg, 'lon'))
                except Exception:
                    lat = None; lon = None
            elif hasattr(msg, 'latitude') and hasattr(msg, 'longitude'):
                try:
                    lat = float(getattr(msg, 'latitude'))
                    lon = float(getattr(msg, 'longitude'))
                except Exception:
                    lat = None; lon = None

        if lat is not None and lon is not None:
            with _udp_lock:
                _udp_latest = {
                    "lat": float(lat),
                    "lon": float(lon),
                    "ts": time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime()),
                    "raw": str(msg),
                    "from": conn_str
                }
        time.sleep(1)
    return True

# ---------- Fallback generic UDP parser ----------
def generic_udp_listener(bind_addr: str, port: int):
    """Simple UDP text parser: supports JSON, comma/semicolon/space-separated floats."""
    global _udp_latest
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind((bind_addr, port))
    except Exception as e:
        print(f"Failed to bind UDP {bind_addr}:{port}: {e}", file=sys.stderr)
        return
    sock.setblocking(True)
    print(f"Generic UDP listener bound to {bind_addr}:{port}")
    sys.stdout.flush()
    while True:
        try:
            data, addr = sock.recvfrom(4096)
        except Exception as e:
            print("UDP recv error:", e, file=sys.stderr)
            time.sleep(0.5)
            continue
        try:
            s = data.decode('utf-8', errors='ignore').strip()
            lat = None; lon = None
            # Try JSON first
            if s.startswith('{') or s.startswith('['):
                try:
                    j = json.loads(s)
                    if isinstance(j, dict):
                        lat = j.get('lat') or j.get('y') or j.get('latitude')
                        lon = j.get('lon') or j.get('lng') or j.get('x') or j.get('longitude')
                except Exception:
                    pass
            if lat is None or lon is None:
                for sep in (',', ';'):
                    if sep in s:
                        parts = [p.strip() for p in s.split(sep) if p.strip()]
                        if len(parts) >= 2:
                            try:
                                lat = float(parts[0]); lon = float(parts[1])
                            except Exception:
                                lat = None; lon = None
                            break
                if (lat is None or lon is None) and ' ' in s:
                    parts = [p.strip() for p in s.split() if p.strip()]
                    if len(parts) >= 2:
                        try:
                            lat = float(parts[0]); lon = float(parts[1])
                        except Exception:
                            lat = None; lon = None
            if (lat is None or lon is None):
                import re
                found = re.findall(r'[-+]?\d*\.\d+|[-+]?\d+', s)
                if len(found) >= 2:
                    try:
                        lat = float(found[0]); lon = float(found[1])
                    except Exception:
                        lat = None; lon = None
            if lat is not None and lon is not None:
                with _udp_lock:
                    _udp_latest = {"lat": float(lat), "lon": float(lon),
                                   "ts": time.strftime("%Y-%m-%dT%H:%M:%S", time.gmtime()),
                                   "raw": s, "from": f"{addr[0]}:{addr[1]}"}
                print(f"UDP <- {addr[0]}:{addr[1]}  -> {lat:.7f}, {lon:.7f}")
                sys.stdout.flush()
            else:
                print(f"UDP: couldn't parse datagram from {addr}: {s!r}")
                sys.stdout.flush()
        except Exception as e:
            print("Error processing UDP packet:", e, file=sys.stderr)
            sys.stdout.flush()

# ---------- TCP Image receiver (reads until connection close) ----------
def image_tcp_listener(bind_addr: str, port: int):
    """
    Simple TCP server: accepts connections, reads all bytes until client closes connection,
    writes them atomically into SCRIPT_DIR/image.png (via temp file + os.replace).
    """
    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    try:
        server_sock.bind((bind_addr, port))
        server_sock.listen(5)
    except Exception as e:
        print(f"Failed to bind image TCP {bind_addr}:{port}: {e}", file=sys.stderr)
        return
    
    while True:
        if (client_fd <= 0):
            client_fd, addr = server_sock.accept()
        chunk = client_fd.recv(100000)
        print(len(chunk))
        client_fd.close()


class WGS84Coord:
    def __init__(self, lat: float = 0.0, lon: float = 0.0, alt: float = 0.0):
        self.lat = lat
        self.lon = lon
        self.alt = alt
    
    def __repr__(self):
        return f"WGS84Coord(lat={self.lat}, lon={self.lon}, alt={self.alt})"

class FlyPlaneData:
    def __init__(self):
        self.coords: List[WGS84Coord] = []
        self.point_count = 0
        self.key = "uav"
    
    def set_coords(self, new_coords: List[WGS84Coord]):
        """Устанавливает новые координаты"""
        self.coords = new_coords.copy() if new_coords else []
        self.point_count = len(self.coords)
    
    def get_coords(self) -> List[WGS84Coord]:
        """Возвращает список координат"""
        return self.coords
    
    def get_point_count(self) -> int:
        """Возвращает количество точек"""
        return self.point_count
    
    def xor_encrypt_decrypt(self, data: bytearray):
        """Шифрует/дешифрует данные с помощью XOR"""
        if not data or not self.key:
            return
        
        key_bytes = self.key.encode('utf-8')
        key_length = len(key_bytes)
        
        for i in range(len(data)):
            data[i] ^= key_bytes[i % key_length]
    
    def get_serialized_size(self) -> int:
        """Возвращает размер сериализованных данных"""
        # int (4 байта) + point_count * (3 float по 4 байта каждый)
        return 4 + self.point_count * 12
    
    def serialization(self) -> bytearray:
        """Сериализует данные в байтовый массив"""
        total_size = self.get_serialized_size()
        data = bytearray(total_size)
        
        # Сериализуем point_count
        struct.pack_into('i', data, 0, self.point_count)
        ptr = 4
        
        # Сериализуем координаты
        for coord in self.coords:
            struct.pack_into('fff', data, ptr, coord.lat, coord.lon, coord.alt)
            ptr += 12
        
        # Шифруем данные
        self.xor_encrypt_decrypt(data)
        
        return data
    
    def deserialization(self, data: bytes) -> bool:
        """Десериализует данные из байтового массива"""
        if not data or len(data) < 4:
            return False
        
        # Создаем временную копию для дешифровки
        temp_data = bytearray(data)
        self.xor_encrypt_decrypt(temp_data)
        
        # Десериализуем point_count
        if len(temp_data) < 4:
            return False
        
        point_count = struct.unpack_from('i', temp_data, 0)[0]
        ptr = 4
        
        # Проверяем достаточно ли данных для координат
        required_size = 4 + point_count * 12
        if len(temp_data) < required_size:
            return False
        
        # Десериализуем координаты
        self.coords = []
        for _ in range(point_count):
            if ptr + 12 > len(temp_data):
                return False
            
            lat, lon, alt = struct.unpack_from('fff', temp_data, ptr)
            self.coords.append(WGS84Coord(lat, lon, alt))
            ptr += 12
        
        self.point_count = point_count
        return True

def send_serialized_data_tcp(host: str, port: int, serialized_data: bytearray) -> bool:
    try:
        # Создаем TCP сокет
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            # Устанавливаем таймаут на соединение и отправку (5 секунд)
            sock.settimeout(5.0)
            
            # Подключаемся к серверу
            sock.connect((host, port))
            
            # Отправляем сами данные
            sock.sendall(serialized_data)
            
            print(f"Данные успешно отправлены: {serialized_data} байт")
            return True
            
    except socket.timeout:
        print("Таймаут соединения")
        return False
    except ConnectionRefusedError:
        print("Соединение отклонено")
        return False
    except Exception as e:
        print(f"Ошибка при отправке данных: {e}")
        return False

# ---------- HTTP handler (adds /udp/last endpoint) ----------
class Handler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        parsed = urlparse(self.path)
        path = parsed.path
        qs = parse_qs(parsed.query)

        if path == '/send':
            length = int(self.headers.get('Content-Length', 0))
            raw = self.rfile.read(length) if length > 0 else b''
            try:
                data = json.loads(raw.decode('utf-8'))
            except Exception as e:
                self.send_response(400); self.end_headers()
                self.wfile.write(b'Invalid JSON: ' + str(e).encode('utf-8')); return
            try:
                print(json.dumps(data, ensure_ascii=False, indent=2))
                sys.stdout.flush()
            except Exception:
                pass
            # write coords.txt
            try:
                points = data.get('points', [])
                coords_path = os.path.join(SCRIPT_DIR, OUTPUT_COORDS_FILE)
                coords = []
                with open(coords_path, 'w', encoding='utf-8') as f:
                    f.write(str(len(points)) + "\n")
                    for p in points:
                        lat = p.get('lat', None)
                        lon = p.get('lon', None)
                        if lat is None and 'y' in p:
                            lat = p.get('y')
                        if lon is None and 'x' in p:
                            lon = p.get('x')
                        try:
                            latf = float(lat); lonf = float(lon)
                            coords.append(WGS84Coord(latf, lonf, 100))
                        except Exception:
                            coords.append(WGS84Coord(lat, lon, 100))
                plane_data = FlyPlaneData()
                plane_data.set_coords(coords)
                serialized = plane_data.serialization()

                send_serialized_data_tcp(HOST, 14520, serialized)
                print("Coords sended")
                sys.stdout.flush()
            except Exception as e:
                print("Error writing coords file:", e, file=sys.stderr); sys.stdout.flush()
                self.send_response(500); self.end_headers(); self.wfile.write(b'Failed to write coords file'); return
            self.send_response(200); self.send_header("Content-Type","text/plain; charset=utf-8"); self.end_headers(); self.wfile.write(b'OK')
            return

        if path == '/route/save':
            length = int(self.headers.get('Content-Length', 0))
            raw = self.rfile.read(length) if length > 0 else b''
            try:
                data = json.loads(raw.decode('utf-8'))
            except Exception as e:
                self.send_response(400); self.end_headers()
                self.wfile.write(b'Invalid JSON: ' + str(e).encode('utf-8')); return
            slot = int(qs.get('slot', [0])[0]) if 'slot' in qs else 0
            if slot < 1 or slot > 3:
                self.send_response(400); self.end_headers(); self.wfile.write(b'Invalid slot'); return
            points = data.get('points', [])
            color = data.get('color', ROUTE_COLORS[slot-1] if 1 <= slot <=3 else ROUTE_COLORS[0])
            cleaned = []
            for p in points:
                try:
                    cleaned.append({'lat': float(p['lat']), 'lon': float(p['lon'])})
                except Exception:
                    pass
            route_path = os.path.join(SCRIPT_DIR, ROUTE_FILENAME_TEMPLATE.format(slot))
            try:
                with open(route_path, 'w', encoding='utf-8') as f:
                    json.dump({'points': cleaned, 'color': color}, f, ensure_ascii=False, indent=2)
                print(f"Saved route slot {slot} -> {route_path}")
                sys.stdout.flush()
                self.send_response(200); self.end_headers(); self.wfile.write(b'OK')
            except Exception as e:
                print("Failed save route:", e, file=sys.stderr); sys.stdout.flush()
                self.send_response(500); self.end_headers(); self.wfile.write(b'Error saving')
            return

        if path == '/route/delete':
            slot = int(qs.get('slot', [0])[0]) if 'slot' in qs else 0
            if slot < 1 or slot > 3:
                self.send_response(400); self.end_headers(); self.wfile.write(b'Invalid slot'); return
            route_path = os.path.join(SCRIPT_DIR, ROUTE_FILENAME_TEMPLATE.format(slot))
            try:
                if os.path.exists(route_path):
                    os.remove(route_path)
                print(f"Deleted route slot {slot} (if existed)")
                sys.stdout.flush()
                self.send_response(200); self.end_headers(); self.wfile.write(b'OK')
            except Exception as e:
                print("Failed delete route:", e, file=sys.stderr); sys.stdout.flush()
                self.send_response(500); self.end_headers(); self.wfile.write(b'Error deleting')
            return

        self.send_response(404); self.end_headers(); self.wfile.write(b'Not found')

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path
        qs = parse_qs(parsed.query)

        if path == '/route/load':
            slot = int(qs.get('slot', [0])[0]) if 'slot' in qs else 0
            if slot < 1 or slot > 3:
                self.send_response(400); self.end_headers(); self.wfile.write(b'Invalid slot'); return
            route_path = os.path.join(SCRIPT_DIR, ROUTE_FILENAME_TEMPLATE.format(slot))
            if not os.path.exists(route_path):
                self.send_response(404); self.end_headers(); self.wfile.write(b'Not found'); return
            try:
                with open(route_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)
                self.send_response(200)
                self.send_header("Content-Type","application/json; charset=utf-8")
                self.end_headers()
                self.wfile.write(json.dumps(data, ensure_ascii=False).encode('utf-8'))
            except Exception as e:
                print("Failed load route:", e, file=sys.stderr); sys.stdout.flush()
                self.send_response(500); self.end_headers(); self.wfile.write(b'Error loading')
            return

        if path == '/route/meta':
            meta = {}
            for s in (1,2,3):
                route_path = os.path.join(SCRIPT_DIR, ROUTE_FILENAME_TEMPLATE.format(s))
                if os.path.exists(route_path):
                    try:
                        with open(route_path, 'r', encoding='utf-8') as f:
                            data = json.load(f)
                        pts = data.get('points', [])
                        meta[str(s)] = len(pts)
                    except Exception:
                        meta[str(s)] = 0
                else:
                    meta[str(s)] = 0
            self.send_response(200)
            self.send_header("Content-Type","application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps(meta, ensure_ascii=False).encode('utf-8'))
            return

        if path == '/udp/last':
            with _udp_lock:
                data = _udp_latest.copy() if _udp_latest else None
            if not data:
                self.send_response(204); self.end_headers(); return
            self.send_response(200)
            self.send_header("Content-Type","application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps(data, ensure_ascii=False).encode('utf-8'))
            return

        # fallback to static file serving (index.html, image.png etc.)
        return http.server.SimpleHTTPRequestHandler.do_GET(self)

# ---------- Utilities and startup ----------
def find_free_port():
    s = socket.socket()
    s.bind((HOST, 0))
    addr, port = s.getsockname()
    s.close()
    return port

def start_mavlink_or_udp_thread():
    try:
        import importlib
        spec = importlib.util.find_spec("pymavlink")
        if spec is not None:
            t = threading.Thread(target=mavlink_listener, args=(UDP_BIND_ADDR, UDP_PORT), daemon=True)
            t.start()
            print("Started MAVLink listener thread.")
            return t
    except Exception:
        pass
    t2 = threading.Thread(target=generic_udp_listener, args=(UDP_BIND_ADDR, UDP_PORT), daemon=True)
    t2.start()
    print("Started generic UDP listener thread (fallback).")
    return t2

def start_image_tcp_thread():
    t = threading.Thread(target=image_tcp_listener, args=(IMAGE_TCP_BIND, IMAGE_TCP_PORT), daemon=True)
    t.start()
    print("Started image TCP listener thread.")
    return t

def start_server_and_open_ui():
    # serve from SCRIPT_DIR
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

    # start listeners
    start_mavlink_or_udp_thread()
    start_image_tcp_thread()

    t = threading.Thread(target=server_thread, daemon=True)
    t.start()

    url = f"http://{HOST}:{port}/index.html"
    try:
        import webview
        window = webview.create_window("Карта (CartoDB) — маршруты + live", url, width=1100, height=800)
        webview.start()
    except Exception as e:
        print("pywebview unavailable or failed to start; opening default browser. Error:", e, file=sys.stderr)
        webbrowser.open(url)
        try:
            while True:
                t.join(1)
        except KeyboardInterrupt:
            print("Exit.")
            return

if __name__ == "__main__":
    print("Script directory:", SCRIPT_DIR)
    print(f"UDP/MAVLink listener will bind to {UDP_BIND_ADDR}:{UDP_PORT}")
    print(f"Image TCP listener will bind to {IMAGE_TCP_BIND}:{IMAGE_TCP_PORT}")
    print("If you want MAVLink parsing, install pymavlink: pip install pymavlink")
    start_server_and_open_ui()
