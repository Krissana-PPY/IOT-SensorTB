CREATE SCHEMA IF NOT EXISTS `mydb` DEFAULT CHARACTER SET utf8mb4;
USE `mydb`;

-- Table for location
CREATE TABLE LOCATION (
    ID INT AUTO_INCREMENT PRIMARY KEY,
    ROW_ID CHAR(10) UNIQUE NOT NULL,
    AREA INT NOT NULL,
    NumberOfPages INT NOT NULL
);

-- Insert values into LOCATION table
INSERT INTO LOCATION (ROW_ID, AREA, NumberOfPages) VALUES
('1AA046A', 10, 3), ('1AA044A', 10, 2), ('1AA042A', 10, 2), ('1AA040A', 10, 2), ('1AA038A', 10, 2),
('1AA036A', 10, 2), ('1AA034A', 10, 2), ('1AA032A', 10, 2), ('1AA030A', 10, 2), ('1AA028A', 10, 2),
('1AA026A', 10, 2), ('1AA024A', 10, 2), ('1AA022A', 10, 2), ('1AA020A', 10, 3), ('1AA019A', 5, 2),
('1AA021A', 5, 2), ('1AA023A', 5, 2), ('1AA025A', 5, 2), ('1AA027A', 5, 2), ('1AA029A', 5, 2),
('1AA031A', 5, 2), ('1AA033A', 5, 2), ('1AA035A', 3, 2), ('1AA037A', 3, 2),
('1AA039A', 3, 2), ('1AA041A', 3, 2), ('1AA043A', 3, 2), ('1AA045A', 3, 4);

-- Table for stock
CREATE TABLE STOCK (
    ID INT AUTO_INCREMENT PRIMARY KEY, 
    ROW_ID CHAR(10) NOT NULL,
    PALLET INT,
    UPDATED_AT TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- Table for row pallet
CREATE TABLE ROW_PALLET (
    ID INT AUTO_INCREMENT PRIMARY KEY,
    ROW_ID CHAR(10) NOT NULL,
    PALLET_NO INT,
    EACH_PALLET INT,
    UPDATED_AT TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- Table for row each pallet
CREATE TABLE ROW_EACH_PALLET (
    ID INT AUTO_INCREMENT PRIMARY KEY,
    ROW_ID CHAR(10) NOT NULL,
    PALLET_NO INT NOT NULL,
    DISTANCE DECIMAL(5,2) NOT NULL,
    UPDATED_AT TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
