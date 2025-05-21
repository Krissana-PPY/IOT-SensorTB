import mysql.connector
from mysql.connector import Error
import sys

sys.stdout.reconfigure(encoding='utf-8')

try:
    connection = mysql.connector.connect(
        host='localhost',  
        port=3306,           
        database='datasensor',     
        user='root',  
        password='1234'
    )

    if connection.is_connected():
        print("เชื่อมต่อฐานข้อมูลสำเร็จ!")

        cursor = connection.cursor()
        cursor.execute("SELECT DATABASE();")
        record = cursor.fetchone()
        print(f"Connected to database: {record}")

        cursor.execute("SHOW TABLES;")
        tables = cursor.fetchall()
        print("Tables in the database:")
        for table in tables:
            print(table)

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
