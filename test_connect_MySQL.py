import mysql.connector
from mysql.connector import Error

try:
    # กำหนดค่าการเชื่อมต่อ
    connection = mysql.connector.connect(
        host='10.0.200.23',  # IP ของ PC1
        port=3306,           # พอร์ต MySQL
        database='mydb',     # ชื่อฐานข้อมูล
        user='root',  # ชื่อผู้ใช้ที่สร้างไว้
        password='1234'  # รหัสผ่านของผู้ใช้
    )

    if connection.is_connected():
        print("เชื่อมต่อฐานข้อมูลสำเร็จ!")

        # ทดลอง Query ข้อมูล
        cursor = connection.cursor()
        cursor.execute("SELECT DATABASE();")
        record = cursor.fetchone()
        print(f"Connected to database: {record}")

except Error as e:
    print(f"Error while connecting to MySQL: {e}")
finally:
    if connection.is_connected():
        cursor.close()
        connection.close()
        print("MySQL connection is closed")
