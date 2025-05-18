import mysql.connector
from mysql.connector import Error
import sys

# Configure stdout to handle UTF-8 encoding
sys.stdout.reconfigure(encoding='utf-8')

try:
    # กำหนดค่าการเชื่อมต่อ
    connection = mysql.connector.connect(
        #host='188.166.231.183',
        #database='datasensor1',
        host='localhost',  # IP ของ PC1
        port=3306,           # พอร์ต MySQL
        database='datasensor',     # ชื่อฐานข้อมูล
        user='root',  # ชื่อผู้ใช้ที่สร้างไว้
        #password='t295V37rQ%uS'  # รหัสผ่านของผู้ใช้
        password='1234'  # รหัสผ่านของผู้ใช้
    )

    if connection.is_connected():
        print("เชื่อมต่อฐานข้อมูลสำเร็จ!")

        # ทดลอง Query ข้อมูล
        cursor = connection.cursor()
        cursor.execute("SELECT DATABASE();")
        record = cursor.fetchone()
        print(f"Connected to database: {record}")

        # แสดงตารางทั้งหมดในฐานข้อมูล
        cursor.execute("SHOW TABLES;")
        tables = cursor.fetchall()
        print("Tables in the database:")
        for table in tables:
            print(table)

            # แสดงข้อมูลในแต่ละตาราง
            cursor.execute(f"SELECT * FROM {table[0]};")
            rows = cursor.fetchall()
            print(f"Data in {table[0]}:")
            for row in rows:
                print(row)

except Error as e:
    print(f"Error while connecting to MySQL: {e}")
except UnicodeEncodeError as ue:
    print(f"Unicode error: {ue}")
finally:
    if 'cursor' in locals() and connection.is_connected():
        cursor.close()
        connection.close()
        print("MySQL connection is closed")
