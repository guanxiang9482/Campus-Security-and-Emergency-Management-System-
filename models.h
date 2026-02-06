#ifndef MODELS_H
#define MODELS_H

#include<iostream>
#include "crow.h"
#include"mysql.h"
#include<vector>
#include<string>

using std::string;

// Define the Database Connection (Helper)
// This prevents from writing mysql_init() in every single function
class DBConnection {
public:
    MYSQL* conn;
    DBConnection() {
        conn = mysql_init(NULL);
        mysql_real_connect(conn, "localhost", "root", "Zxcv@123", "cmdb", 0, NULL, 0);
    }
    ~DBConnection() {
        mysql_close(conn);
    }
};

class NotificationSystem {
public:
    // 1. SEND: Internal function used by other systems (Incident, Escort, etc.)
    void sendNotification(int userId, string message) {
        DBConnection db;
        string query = "INSERT INTO Notifications (user_id, message) VALUES (" + 
                       std::to_string(userId) + ", '" + message + "')";
        mysql_query(db.conn, query.c_str());
    }

    // 2. FETCH: Get messages for the logged-in user
    std::vector<crow::json::wvalue> getMyNotifications(int userId) {
        DBConnection db;
        string query = "SELECT id, message, created_at FROM Notifications WHERE user_id = " + 
                       std::to_string(userId) + " ORDER BY created_at DESC LIMIT 10";

        std::vector<crow::json::wvalue> list;
        if (mysql_query(db.conn, query.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(db.conn);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                crow::json::wvalue n;
                n["id"] = std::stoi(row[0]);
                n["message"] = row[1];
                n["time"] = row[2];
                list.push_back(n);
            }
            mysql_free_result(res);
        }
        return list;
    }
};

struct Incident{
    int id;
    int reported_by_stu;
    int responded_by_staff;
    string category;
    string location;
    string time_reported;
    string status;
};

struct ReportItem {
    int id;
    int author_id;
    string title;
    string content;
    string time;
};

class ReportSystem {
    public:
    std::vector<crow::json::wvalue> fetchReports(int staffId, string role) {
            DBConnection db;
            string query;

            // --- ADMIN VIEW: SEE ALL REPORTS + STAFF NAMES ---
            if (role == "Admin") {
                query = "SELECT R.report_id, R.submitted_by_staff_id, R.title, R.description, R.generated_at, R.time_generated "
                        "FROM Report_Log R "
                        "ORDER BY R.time_generated DESC";
            } 
            // --- STAFF VIEW: SEE ONLY MY REPORTS ---
            else {
                query = "SELECT report_id, submitted_by_staff_id, title, description, generated_at, time_generated "
                        "FROM Report_Log WHERE submitted_by_staff_id = " + std::to_string(staffId) + 
                        " ORDER BY time_generated DESC";
            }

            std::vector<crow::json::wvalue> reports;
            if (mysql_query(db.conn, query.c_str()) == 0) {
                MYSQL_RES* res = mysql_store_result(db.conn);
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res))) {
                    crow::json::wvalue item;
                    item["id"] = std::stoi(row[0]);
                    
                    // For Admin, row[1] is the Name. For Staff, it's the ID.
                    if (role == "Admin") {
                        item["author"] = row[1]; // Actual Name (e.g., "Ali")
                    } else {
                        item["author"] = "Me";   // Just "Me" for staff
                    }

                    item["title"] = row[2];
                    item["content"] = row[3];
                    item["location"] = row[4]; // generated_at
                    item["time"] = row[5];
                    reports.push_back(item);
                }
                mysql_free_result(res);
            }
            return reports;
        }
    };

class PatrolSystem {
public:
    // Helper: Parse string "Gate:0,Fence:1" into JSON
    std::vector<crow::json::wvalue> parseCheckpoints(string raw) {
        std::vector<crow::json::wvalue> points;
        std::stringstream ss(raw);
        string segment;
        
        while(std::getline(ss, segment, ',')) { 
            size_t delimiter = segment.find(':');
            if (delimiter != string::npos) {
                crow::json::wvalue p;
                p["name"] = segment.substr(0, delimiter);
                p["signed"] = (segment.substr(delimiter + 1) == "1");
                points.push_back(p);
            }
        }
        return points;
    }

    // Helper: Notify ALL Security Staff
    void notifyAllSecurity(string message) {
        DBConnection db;
        NotificationSystem ns;
        // Select user_id from Security_Staff table (linked to User table)
        string query = "SELECT user_id FROM Security_Staff";
        
        if (mysql_query(db.conn, query.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(db.conn);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                ns.sendNotification(std::stoi(row[0]), message);
            }
            mysql_free_result(res);
        }
    }

    // 1. Get Dashboard Data
    crow::json::wvalue getPatrolDashboard(int securityId) {
        DBConnection db;
        crow::json::wvalue data;
        
        // A. Check if I have an active route
        string myQuery = "SELECT id, name, checkpoints, status FROM patrol_route "
                         "WHERE assigned_security_id = " + std::to_string(securityId);
        
        if (mysql_query(db.conn, myQuery.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(db.conn);
            if (mysql_num_rows(res) > 0) {
                MYSQL_ROW row = mysql_fetch_row(res);
                data["has_route"] = true;
                data["route_id"] = std::stoi(row[0]);
                data["route_name"] = row[1];
                data["checkpoints"] = parseCheckpoints(row[2]);
                data["status"] = row[3];
                mysql_free_result(res);
                return data;
            }
        }

        // B. If no active route, fetch AVAILABLE (Not Completed) routes
        data["has_route"] = false;
        std::vector<crow::json::wvalue> available;
        
        // FIX: Only show routes that are NOT 'Completed'
        string freeQuery = "SELECT id, name FROM patrol_route "
                           "WHERE assigned_security_id IS NULL AND status != 'Completed'";
        
        if (mysql_query(db.conn, freeQuery.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(db.conn);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                crow::json::wvalue r;
                r["id"] = std::stoi(row[0]);
                r["name"] = row[1];
                available.push_back(std::move(r));
            }
            mysql_free_result(res);
        }
        data["available_routes"] = std::move(available);
        return data;
    }

    // 2. Assign Route
    bool assignRoute(int routeId, int securityId) {
        DBConnection db;
        string query = "UPDATE patrol_route SET assigned_security_id = " + std::to_string(securityId) + 
                       ", status = 'In Progress' WHERE id = " + std::to_string(routeId) + 
                       " AND assigned_security_id IS NULL";
        return (mysql_query(db.conn, query.c_str()) == 0);
    }

    // 3. Sign Checkpoint
    bool signCheckpoint(int routeId, int index) {
        DBConnection db;
        
        string fetchQ = "SELECT checkpoints FROM patrol_route WHERE id = " + std::to_string(routeId);
        mysql_query(db.conn, fetchQ.c_str());
        MYSQL_RES* res = mysql_store_result(db.conn);
        if(!res) return false;
        MYSQL_ROW row = mysql_fetch_row(res);
        if(!row) return false;
        
        string raw = row[0];
        mysql_free_result(res);

        std::stringstream ss(raw);
        string segment, newRaw = "";
        int current = 0;

        while(std::getline(ss, segment, ',')) {
            size_t del = segment.find(':');
            string name = segment.substr(0, del);
            string val = segment.substr(del+1);

            if(current == index) val = "1"; 
            newRaw += name + ":" + val + ",";
            current++;
        }
        if(!newRaw.empty()) newRaw.pop_back();

        string updateQ = "UPDATE patrol_route SET checkpoints = '" + newRaw + "' WHERE id = " + std::to_string(routeId);
        return (mysql_query(db.conn, updateQ.c_str()) == 0);
    }

    // 4. COMPLETE MISSION (Updated Logic)
    bool completePatrol(int routeId) {
        DBConnection db;

        // A. Mark this specific route as 'Completed' and unassign the user
        // We keep the checkpoints as '1' (Signed) to show it's done for the day.
        string updateQ = "UPDATE patrol_route SET status = 'Completed', assigned_security_id = NULL "
                         "WHERE id = " + std::to_string(routeId);

        if (mysql_query(db.conn, updateQ.c_str()) != 0) return false;

        // B. Check Mission Status (Count remaining routes)
        string countQ = "SELECT COUNT(*) FROM patrol_route WHERE status != 'Completed'";
        int remaining = 0;
        
        if (mysql_query(db.conn, countQ.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(db.conn);
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row) remaining = std::stoi(row[0]);
            mysql_free_result(res);
        }

        // C. Send Notifications based on progress
        if (remaining == 0) {
            notifyAllSecurity("🎉 MISSION ACCOMPLISHED: All patrol routes for today have been completed!");
        } else {
            notifyAllSecurity("⚠️ Patrol Update: A route was finished. " + std::to_string(remaining) + " route(s) still remaining.");
        }

        return true;
    }
};

class ResourceSystem {
public:
    // 1. Helper: Notify ALL Admins
    void notifyAllAdmins(string message) {
        DBConnection db;
        NotificationSystem ns;
        string query = "SELECT user_id FROM User WHERE role = 'Admin'";
        if (mysql_query(db.conn, query.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(db.conn);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                ns.sendNotification(std::stoi(row[0]), message);
            }
            mysql_free_result(res);
        }
    }

    // 2. Fetch Resources (With Admin Name)
    std::vector<crow::json::wvalue> getAllResources() {
        DBConnection db;
        std::vector<crow::json::wvalue> list;
        
        // JOIN User table to get the name of the admin who managed it
        string query = "SELECT R.id, R.name, R.type, R.quantity, R.location, U.name "
                       "FROM Resources R "
                       "LEFT JOIN Admin A ON R.managed_by_admin_id = A.admin_id " // Join to Admin first
                       "LEFT JOIN User U ON A.user_id = U.user_id "               // Then to User
                       "ORDER BY R.id ASC";
        
        if (mysql_query(db.conn, query.c_str()) == 0) {
            MYSQL_RES* res = mysql_store_result(db.conn);
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                crow::json::wvalue r;
                r["id"] = std::stoi(row[0]);
                r["name"] = row[1];
                r["type"] = row[2];
                r["quantity"] = std::stoi(row[3]);
                r["location"] = row[4];
                // Handle NULL (if no one has edited it yet)
                r["managed_by"] = row[5] ? row[5] : "System"; 
                list.push_back(r);
            }
            mysql_free_result(res);
        }
        return list;
    }

    // 3. Add or Update Resource (Now accepts adminId)
    bool saveResource(int id, string name, string type, int quantity, string location, int adminId) {
        DBConnection db;
        string query;
        string notifMsg;

        if (id == -1) {
            // INSERT
            query = "INSERT INTO Resources (name, type, quantity, location, managed_by_admin_id) VALUES ('" + 
                    name + "', '" + type + "', " + std::to_string(quantity) + ", '" + location + "', " + std::to_string(adminId) + ")";
            notifMsg = "📦 New Resource Added: " + name + " (" + std::to_string(quantity) + " units).";
        } else {
            // UPDATE
            query = "UPDATE Resources SET name='" + name + "', type='" + type + 
                    "', quantity=" + std::to_string(quantity) + ", location='" + location + 
                    "', managed_by_admin_id=" + std::to_string(adminId) + 
                    " WHERE id=" + std::to_string(id);
            notifMsg = "🔄 Resource Updated: " + name + " is now " + std::to_string(quantity) + " units.";
        }

        if (mysql_query(db.conn, query.c_str()) == 0) {
            notifyAllAdmins(notifMsg);
            return true;
        }
        else std::cerr << "SQL Error: " << mysql_error(db.conn) << std::endl;
        return false;
    }

    // 4. Delete Resource
    bool deleteResource(int id) {
        DBConnection db;
        string query = "DELETE FROM Resources WHERE id=" + std::to_string(id);
        if (mysql_query(db.conn, query.c_str()) == 0) {
            notifyAllAdmins("🗑️ A resource (ID: " + std::to_string(id) + ") was removed from inventory.");
            return true;
        }
        return false;
    }
};

struct DigitalIDData {
    int student_id;
    string name;
    string course;
    string academic_status;
    string verify_status;
    string phone;  
    string year;    
    string faculty; // (Always "Faculty of Multimedia")
    bool found;
};

class EmergencySystem {
public:
    // 1. Security Staff triggers this
    bool setEmergencyStatus(bool active, string msg) {
        DBConnection db;
        // We update the single row (ID=1) to keep it simple
        string status = active ? "1" : "0";
        string query = "UPDATE Emergency_Alert SET is_active = " + status + 
                       ", message = '" + msg + "', created_at = NOW() WHERE alert_id = 1";
        
        return (mysql_query(db.conn, query.c_str()) == 0);
    }

    // 2. All Dashboards check this
    crow::json::wvalue checkStatus() {
        DBConnection db;
        crow::json::wvalue status;
        status["active"] = false;

        if (mysql_query(db.conn, "SELECT is_active, message FROM Emergency_Alert WHERE alert_id = 1") == 0) {
            MYSQL_RES* res = mysql_store_result(db.conn);
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row) {
                status["active"] = (std::string(row[0]) == "1");
                status["message"] = row[1];
            }
            mysql_free_result(res);
        }
        return status;
    }
};


class User{
    protected:
        int user_id;
        string name;
        string email;
        string password;
        string role;
        string phone;
        string SQ1;
        string SQ2;
    public:
        User(){}
        User(string n, string mail, string pass, string r, string num,string q1, string q2)
        :name(n), email(mail), password(pass), role(r), phone(num), SQ1(q1), SQ2(q2){}
        int Getid()const{return user_id;}
        string Getname()const{return name;}
        string Getrole()const{return role;}
        bool Register(){
            DBConnection db;

            std::string query = "INSERT INTO User (name, email, password, role, contact_num, SQ1, SQ2) VALUES ('" 
                                + name + "', '" + email + "', '" + password + "', '" + role + "', '" + phone + "','"
                                + SQ1 + "', '" + SQ2 + "')";

            if (mysql_query(db.conn, query.c_str()) != 0) {
                // Print error to console to help debug
                std::cerr << "MySQL Error: " << mysql_error(db.conn) << std::endl;
                return false;
            }
            return true;
        }

        bool Register(string course, string year){

            DBConnection db;

            std::string query = "SELECT user_id FROM User WHERE Email='" + email + 
                            "' AND Password='" + password + "';";
            if (mysql_query(db.conn, query.c_str()) != 0) {
                // Print error to console to help debug
                std::cerr << "MySQL Error: " << mysql_error(db.conn) << std::endl;
                return false;
            }

            MYSQL_RES* res = mysql_store_result(db.conn);

            if (res && mysql_num_rows(res) > 0) {
                MYSQL_ROW row = mysql_fetch_row(res);
                std::string newUserID = row[0];

                std::string roleQuery = "";
                if (role == "Student") {
                    // For Students, we use the Phone/Matrix as a primary key or unique identifier as per your SRS [cite: 170-171]
                    roleQuery = "INSERT INTO Student (user_id, course, year) VALUES ("  + newUserID + ", '" + course + "', " + year + ")";
                } else if (role == "Staff") {
                    roleQuery = "INSERT INTO Staff (user_id) VALUES ("  + newUserID + ")";
                } else if (role == "Security Staff") {
                    roleQuery = "INSERT INTO Security_Staff (user_id) VALUES ("  + newUserID + ")";
                } else if (role == "Admin") {
                    roleQuery = "INSERT INTO Admin (user_id) VALUES ("  + newUserID + ")";
                }

                if (!roleQuery.empty()) {
                    if (mysql_query(db.conn, roleQuery.c_str())) {
                        std::cerr << "MySQL Error: " << mysql_error(db.conn) << std::endl;
                        return false;
                    }

                    if (role == "Student") {
                        // Get the Student ID that was just created (Auto Increment)
                        my_ulonglong newStudentID = mysql_insert_id(db.conn);

                        // Insert into Digital_Id with 'Pending' status
                        std::string digiQuery = "INSERT INTO Digital_Id (student_id, verify_status) VALUES (" 
                                              + std::to_string(newStudentID) + ", 'Pending')";

                        if (mysql_query(db.conn, digiQuery.c_str())) {
                            std::cerr << "Digital ID Creation Error: " << mysql_error(db.conn) << std::endl;
                        }
                    }
                }
            }
            return true;
        }

        bool Login(string identifier, string pass){
            DBConnection db;
            std::string query = "SELECT user_id, name, Role FROM User WHERE (Email='" + identifier + 
                            "' OR contact_num ='" + identifier + "') AND Password='" + pass + "';";
        
            if(mysql_query(db.conn, query.c_str()) != 0) return false;

            MYSQL_RES* res = mysql_store_result(db.conn);
            if (res && mysql_num_rows(res) > 0) {
                MYSQL_ROW row = mysql_fetch_row(res);
                this->user_id = std::stoi(row[0]);
                this->name = row[1];
                this->role = row[2];
                mysql_free_result(res);
                return true;
            }
            mysql_free_result(res);
            return false;
        }
};

class Student : public User{
    private:
        int student_id;
        string course;
        int year;
    public:
        int GetStudentID(string userid){
            this->student_id = -1;
            DBConnection db;
            std::string query = "SELECT student_id FROM Student WHERE user_id = " + userid;
        
            if(mysql_query(db.conn, query.c_str()) != 0) return false;

            MYSQL_RES* res = mysql_store_result(db.conn);
            if (res && mysql_num_rows(res) > 0) {
                MYSQL_ROW row = mysql_fetch_row(res);
                this->student_id = std::stoi(row[0]);
            }
            mysql_free_result(res);
            return student_id;
        }
        bool reportIncident(std::string category, std::string description, std::string location){
            DBConnection db; // Open connection
            
            std::string query = "INSERT INTO Incident (reported_by_student_id, category, description, location, time_reported, status) VALUES (" 
                                + std::to_string(student_id) + ", '" 
                                + category + "', '" 
                                + description + "', '" 
                                + location + "', NOW(), 'Open')";

            if (mysql_query(db.conn, query.c_str()) != 0) {
                // Print error to console to help debug
                std::cerr << "MySQL Error: " << mysql_error(db.conn) << std::endl;
                return false;
            }
            return true;
        }
        bool RequestEscort(string description, string location){
            DBConnection db; // Open connection
            
            std::string query = "INSERT INTO Escort_Request (requested_by_student_id, description, location, request_time, status) VALUES (" 
                                + std::to_string(student_id) + ", '" 
                                + description + "', '" 
                                + location + "', NOW(), 'Open')";

            if (mysql_query(db.conn, query.c_str()) != 0) {
                // Print error to console to help debug
                std::cerr << "MySQL Error: " << mysql_error(db.conn) << std::endl;
                return false;
            }
            return true;
        }
        DigitalIDData getDigitalIDDetails(int userId) {
            DBConnection db;
            DigitalIDData data;
            data.found = false;
            data.faculty = "Faculty of Multimedia"; // Hardcoded per requirement

            // Fetch Name, Phone from User | Course, Status, Year from Student
            // Assumes 'year' column exists in Student table. If not, adds it or use placeholder.
            string query = "SELECT U.name, U.contact_num, S.course, S.academic_status, S.year, S.student_id, D.verify_status "
                       "FROM Student S "
                       "JOIN User U ON S.user_id = U.user_id "
                       "JOIN Digital_Id D ON S.student_id = D.student_id "
                       "WHERE S.user_id = " + std::to_string(userId);

            if (mysql_query(db.conn, query.c_str()) == 0) {
                MYSQL_RES* res = mysql_store_result(db.conn);
                if (res && mysql_num_rows(res) > 0) {
                    MYSQL_ROW row = mysql_fetch_row(res);
                    data.name = row[0] ? row[0] : "Unknown";
                    data.phone = row[1] ? row[1] : "N/A";
                    data.course = row[2] ? row[2] : "N/A";
                    data.academic_status = row[3] ? row[3] : "Inactive";
                    data.year = row[4] ? row[4] : "1"; // Default to Year 1 if null
                    data.student_id = std::stoi(row[5]);
                    // Fetch the actual verification status (Pending/Approved/Rejected)
                    data.verify_status = row[6] ? row[6] : "Pending";
                    data.found = true;
                }
                mysql_free_result(res);
            }
            return data;
        }
};
class Staff : public User{
    private:
        int staff_id;
    public:
        int GetStaffID(string userid){
            this->staff_id = -1;
            DBConnection db;
            std::string query = "SELECT staff_id FROM Staff WHERE user_id = " + userid;
        
            if(mysql_query(db.conn, query.c_str()) != 0) return false;

            MYSQL_RES* res = mysql_store_result(db.conn);
            if (res && mysql_num_rows(res) > 0) {
                MYSQL_ROW row = mysql_fetch_row(res);
                this->staff_id = std::stoi(row[0]);
            }
            mysql_free_result(res);
            return staff_id;
        }
        bool SubmitReport(string title, string description,string place){
            DBConnection db; // Open connection
            
            std::string query = "INSERT INTO REPORT_LOG (submitted_by_staff_id, title, description, generated_at, time_generated) VALUES (" 
                                + std::to_string(staff_id) + ", '" 
                                + title + "', '" 
                                + description  +  "', '" + place + "', NOW());";

            if (mysql_query(db.conn, query.c_str()) != 0) {
                // Print error to console to help debug
                std::cerr << "MySQL Error: " << mysql_error(db.conn) << std::endl;
                return false;
            }
            return true;
        }
        std::vector<crow::json::wvalue> getUnverifiedStudents() {
            DBConnection db;
            std::vector<crow::json::wvalue> list;
            
            string query = "SELECT U.name, S.course, D.verify_status, U.user_id "
                           "FROM Digital_Id D "
                           "JOIN Student S ON D.student_id = S.student_id "
                           "JOIN User U ON S.user_id = U.user_id "
                           "WHERE D.verify_status = 'Pending'";

            if (mysql_query(db.conn, query.c_str()) == 0) {
                MYSQL_RES* res = mysql_store_result(db.conn);
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res))) {
                    crow::json::wvalue student;
                    student["name"] = row[0];
                    student["course"] = row[1];
                    student["status"] = row[2];
                    student["user_id"] = row[3]; // We need UserID to fetch the full card later
                    list.push_back(student);
                }
                mysql_free_result(res);
            }
            return list;
        }

        // Verify Student (Update Status)
        bool verifyStudent(int targetUserId, bool approve) {
            DBConnection db;
            string newStatus = approve ? "Approved" : "Rejected";

            Student s;
            int stu_id = s.GetStudentID(std::to_string(targetUserId));
            
            string query = "UPDATE Digital_Id SET verify_status = '" + newStatus + 
                            "', verified_by_staff_id = " + std::to_string(staff_id) +
                            " WHERE student_id = " + std::to_string(stu_id);

            return (mysql_query(db.conn, query.c_str()) == 0);
        }
};
class SecurityStaff : public User{
    private:
        int sec_staff_id;
    public:
        int GetSecStaffID(string userid){
                this->sec_staff_id = -1;
                DBConnection db;
                std::string query = "SELECT security_staff_id FROM security_staff WHERE user_id = " + userid;
            
                if(mysql_query(db.conn, query.c_str()) != 0) return false;

                MYSQL_RES* res = mysql_store_result(db.conn);
                if (res && mysql_num_rows(res) > 0) {
                    MYSQL_ROW row = mysql_fetch_row(res);
                    this->sec_staff_id = std::stoi(row[0]);
                }
                mysql_free_result(res);
                return sec_staff_id;
        }
};
class Admin : public User{
    private:
        int admin_id;
    public: 
        int GetAdminID(string userid){
            this->admin_id = -1;
            DBConnection db;
            std::string query = "SELECT admin_id FROM Admin WHERE user_id = " + userid;
        
            if(mysql_query(db.conn, query.c_str()) != 0) return -1;

            MYSQL_RES* res = mysql_store_result(db.conn);
            if (res && mysql_num_rows(res) > 0) {
                MYSQL_ROW row = mysql_fetch_row(res);
                this->admin_id = std::stoi(row[0]);
            }
            mysql_free_result(res);
            return admin_id;
        }
        std::vector<crow::json::wvalue> getAllUsers() {
            DBConnection db;
            
            // USE LEFT JOIN: This gets ALL users. 
            // If they are not a student, S.academic_status will be NULL.
            string query = "SELECT U.user_id, U.name, U.email, U.role, U.contact_num, S.academic_status "
                           "FROM User U "
                           "LEFT JOIN Student S ON U.user_id = S.user_id "
                           "ORDER BY U.user_id ASC";
            
            std::vector<crow::json::wvalue> users;
            if (mysql_query(db.conn, query.c_str()) == 0) {
                MYSQL_RES* res = mysql_store_result(db.conn);
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res))) {
                    crow::json::wvalue u;
                    u["id"] = std::stoi(row[0]);
                    u["name"] = row[1];
                    u["email"] = row[2];
                    u["role"] = row[3];
                    u["phone"] = row[4];
                    
                    // LOGIC: If academic_status exists (Student), use it.
                    // Otherwise (Staff/Admin), default to "Active".
                    if (row[5] != NULL) {
                        u["status"] = row[5];
                    } else {
                        u["status"] = "Active"; 
                    }
                    
                    users.push_back(u);
                }
                mysql_free_result(res);
            }
            return users;
        }

        // Update User Logic (with Comparison & Notification)
        bool updateUserProfile(int targetId, string newName, string newEmail, string newRole, string newPhone, string newStatus) {
            DBConnection db;
            NotificationSystem ns; 
            
            // --- STEP A: Fetch OLD Data for Comparison ---
            string oldName, oldEmail, oldRole, oldPhone, oldStatus = "Active"; // Default
            
            // Fetch User Details
            string fetchQ = "SELECT name, email, role, contact_num FROM User WHERE user_id = " + std::to_string(targetId);
            if (mysql_query(db.conn, fetchQ.c_str()) == 0) {
                MYSQL_RES* res = mysql_store_result(db.conn);
                if (MYSQL_ROW row = mysql_fetch_row(res)) {
                    oldName = row[0] ? row[0] : "";
                    oldEmail = row[1] ? row[1] : "";
                    oldRole = row[2] ? row[2] : "";
                    oldPhone = row[3] ? row[3] : "";
                }
                mysql_free_result(res);
            }

            // Fetch Student Status (if they were already a student)
            if (oldRole == "Student") {
                string stuQ = "SELECT academic_status FROM Student WHERE user_id = " + std::to_string(targetId);
                if (mysql_query(db.conn, stuQ.c_str()) == 0) {
                    MYSQL_RES* res = mysql_store_result(db.conn);
                    if (MYSQL_ROW row = mysql_fetch_row(res)) {
                        oldStatus = row[0] ? row[0] : "Active";
                    }
                    mysql_free_result(res);
                }
            }

            // --- STEP B: Perform the UPDATE (Original Logic) ---
            string query = "UPDATE User SET name = '" + newName + 
                        "', email = '" + newEmail + 
                        "', role = '" + newRole + 
                        "', contact_num = '" + newPhone + 
                        "' WHERE user_id = " + std::to_string(targetId);

            if (mysql_query(db.conn, query.c_str()) != 0) return false;

            // Update Student Status if role is Student
            if (newRole == "Student") {
                string studentQuery = "UPDATE Student SET academic_status = '" + newStatus + 
                                      "' WHERE user_id = " + std::to_string(targetId);
                mysql_query(db.conn, studentQuery.c_str());
            }

            // --- STEP C: Compare & Notify ---
            string changes = "";
            if (oldName != newName) changes += "Name, ";
            if (oldEmail != newEmail) changes += "Email, ";
            if (oldPhone != newPhone) changes += "Phone, ";
            if (oldRole != newRole) changes += "Role (now " + newRole + "), ";
            
            // Compare status only if it's relevant (Student -> Student)
            if (newRole == "Student" && oldRole == "Student" && oldStatus != newStatus) {
                changes += "Status (now " + newStatus + "), ";
            }

            if (!changes.empty()) {
                // Remove trailing comma
                changes = changes.substr(0, changes.length() - 2); 
                string msg = "Admin updated your profile: " + changes + ".";
                ns.sendNotification(targetId, msg);
            }

            return true;
        }

};


#endif