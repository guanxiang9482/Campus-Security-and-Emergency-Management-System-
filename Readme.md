# Campus Security & Emergency System (C.M.E.S.)

A C++ web-based application for managing campus security, incidents, and emergency resources. This project uses the **Crow** web framework and **MySQL** for database management.

---

## 📋 Prerequisites
Before you begin, ensure you have the following installed on your machine:

1.  **C++ Compiler:** G++ (MinGW-w64) supporting C++17 or later.
2.  **MySQL Server:** [Download MySQL Installer](https://dev.mysql.com/downloads/installer/).
3.  **Git:** To clone the repository.

*(Note: The `crow` and `asio` libraries are included locally in this repository, so you do not need to install them via vcpkg).*

---

## ⚙️ Step 1: Database Setup
The application will crash if the database is not set up correctly.

1.  **Open MySQL Workbench** (or your command line terminal).
2.  **Create the Database:**
    Run the following SQL command to create the required database:
    ```sql
    CREATE DATABASE cmdb;
    ```
3.  **Import the Schema:**
    You must import the tables and data.
    * **Option A (Command Line):** Run this command in the project folder:
        ```bash
        mysql -u root -p cmdb < database/database_setup.sql
        ```
    * **Option B (Workbench):** Go to **Server** > **Data Import**, select the `database/database_setup.sql` file, choose `cmdb` as the target schema, and click **Start Import**.

---

## 🔧 Step 2: Configure Connection Settings (CRITICAL)
**You must update the code to match your local MySQL password.**

1.  Open the file **`models.h`** in your code editor.
2.  Locate the database connection line (inside the `DBConnection` class):
    ```cpp
    mysql_real_connect(conn, "localhost", "root", "Zxcv@123", "cmdb", 0, NULL, 0);
    ```
3.  **Change `"Zxcv@123"`** to the password you created when installing MySQL.
4.  If your MySQL username is not `root`, change that as well.
5.  **Save the file.**

> **Warning:** If you skip this step, you will see a "Connection lost" or "Access Denied" error when trying to register or login.

---

## 🔨 Step 3: Compilation
You need to link the MySQL libraries to compile the project.

1.  **Locate your MySQL Installation:**
    Check `C:\Program Files\MySQL\`.
    * If you see `MySQL Server 8.0`, use the command below as is.
    * If you see `MySQL Server 8.4` or `9.0`, update the path in the command to match your folder.

2.  **Run the Build Command:**
    Open your terminal in this project folder and run:

    ```bash
    g++ main.cpp -o main.exe -I . -I "C:/Program Files/MySQL/MySQL Server 8.0/include" -L "C:/Program Files/MySQL/MySQL Server 8.0/lib" -lmysql -lws2_32 -lmswsock
    ```

---

## 🚀 Step 4: Running the Application

1.  **Verify DLL:** Ensure the file **`libmysql.dll`** is present in the same folder as `main.exe` (it is included in this repository).
2.  **Run the Server:**
    ```bash
    ./main.exe
    ```
3.  **Access the Dashboard:**
    Open your web browser and go to:
    [http://localhost:18080](http://localhost:18080)

---

## 📂 Project Structure
* **`main.cpp`**: Entry point of the server and route definitions.
* **`models.h`**: Handles database connections and SQL logic.
* **`crow/` & `asio/`**: Local libraries for the web server and networking.
* **`templates/`**: Contains all HTML files (Dashboard, Register, etc.).
* **`database/`**: Contains the SQL backup file (`database_setup.sql`).
* **`libmysql.dll`**: Required runtime library for MySQL.

---

## ❓ Troubleshooting

**Issue: "Connection lost during query" or "Access Denied"**
* **Cause:** The password in `models.h` does not match your local MySQL password.
* **Fix:** Go back to **Step 2** and update the password string.

**Issue: "fatal error: mysql.h: No such file or directory"**
* **Cause:** The compiler cannot find the MySQL include folder.
* **Fix:** Check if your MySQL version is 8.0, 8.4, or 9.0 in `C:\Program Files\MySQL\` and update the `-I` path in the compilation command.

**Issue: "The code execution cannot proceed because libmysql.dll was not found"**
* **Cause:** The DLL is missing from the folder.
* **Fix:** Ensure `libmysql.dll` is in the same folder as `main.exe`.
