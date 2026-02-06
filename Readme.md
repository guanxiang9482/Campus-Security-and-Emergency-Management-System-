# How to Compile C.M.E.S.

To run this project, you need to link the **MySQL Connector** library.
Please follow these steps:

### Step 1: Locate MySQL on your machine
Find where MySQL Server is installed on your computer.
* **Common Include Path:** `C:\Program Files\MySQL\MySQL Server 8.0\include`
* **Common Library Path:** `C:\Program Files\MySQL\MySQL Server 8.0\lib`

*(Note: If you have MySQL version 8.4 or 9.0, simply adjust the version number in the path).*

### Step 2: Compile
Open your terminal in this folder and run the following command.
**IMPORTANT:** Replace the paths below with YOUR MySQL paths found in Step 1.

### Compilation Command
Open a terminal in the project folder and run:
g++ main.cpp -o main.exe -I . -I "C:/Program Files/MySQL/MySQL Server 8.0/include" -L "C:/Program Files/MySQL/MySQL Server 8.0/lib" -lmysql -lws2_32 -lmswsock