import math
x = 25

# สร้าง list ของค่าที่คูณกับ x/15
factors = [3, 4.2, 5.4]

# วนลูปเพื่อคำนวณและพิมพ์ผลลัพธ์
for i in range(len(factors)):
    y = factors[i] * math.tan(x * math.pi / 180)
    y = y - 1
    print(y)