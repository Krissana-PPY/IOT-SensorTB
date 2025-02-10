#!/bin/bash
# รัน Flask App
python3 /home/tbl/IOT-SensorTB/flask_app.py &

# รอให้ Flask เริ่มต้นทำงาน (3 วินาที)
sleep 3

# รอให้ X11 พร้อมทำงาน
timeout 300 bash -c 'until xset q &>/dev/null; do sleep 1; done'

# รอเพิ่มเติมสำหรับระบบ
sleep 10

# เปิด Chromium ในโหมด kiosk (เต็มจอ, ไม่มี address bar)
chromium-browser --kiosk http://localhost:5000

# หรือถ้าไม่ต้องการโหมด kiosk:
# chromium-browser http://localhost:5000