import mysql.connector

# การเชื่อมต่อกับฐานข้อมูล
connection = mysql.connector.connect(
    host="localhost",    # เปลี่ยนเป็นโฮสต์ของคุณ
    user="root",  # เปลี่ยนเป็นชื่อผู้ใช้ของคุณ
    password="1234",  # เปลี่ยนเป็นรหัสผ่านของคุณ
    database="mydb"  # เปลี่ยนเป็นชื่อฐานข้อมูลของคุณ
)

cursor = connection.cursor()

# SQL เพื่อเพิ่มข้อมูลลงในตาราง
insert_query = "INSERT INTO ROW_NAME (ROW_ID) VALUES (%s)"

# การเขียนข้อมูล AA001 ถึง AA032 ลงในฐานข้อมูล
for i in range(1, 33):
    row_id = f"AA{i:03}"  # สร้างค่า AA001 ถึง AA032
    cursor.execute(insert_query, (row_id,))

# ยืนยันการเปลี่ยนแปลงในฐานข้อมูล
connection.commit()

# ปิดการเชื่อมต่อกับฐานข้อมูล
cursor.close()
connection.close()

print("Data has been inserted successfully!")