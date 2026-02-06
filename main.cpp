#include "crow.h"
#include "models.h"
#include <mysql.h> // This is the standard header for the C API functions
#include <iostream>
#include <cstdlib>
#include <ctime>
#include<string>

bool IsValidName(const std::string& name){
    if(name.empty()) return false;
    //Check if the name contains only letters and spaces
    for(char const& l:name)
    if(!::isalpha(l) && !::isspace(l)) return false;

    return true;
}

bool hasSpecialChar(const std::string& str){
    std::string specialChars = "~`!@#$%^&*()_-=+[]{};:,.<>?";
    return std::any_of(str.begin(),str.end(),[&](char c){
        return specialChars.find(c) != std::string::npos;
    });
}

// Function to automatically open the browser
void openBrowser(std::string url) {
    #ifdef _WIN32
    std::string command = "start " + url;
    #elif __APPLE__
    std::string command = "open " + url;
    #else
    std::string command = "xdg-open " + url;
    #endif
    std::system(command.c_str());
}

int main() {
    crow::SimpleApp app; // Added missing app declaration
    srand(static_cast<unsigned int>(time(NULL)));

    //Serve Login Page
    CROW_ROUTE(app, "/")
    ([](){
        auto page = crow::mustache::load_text("Welcome.html");
        return crow::response(page);
    });

    CROW_ROUTE(app, "/api/notifications")([](const crow::request& req){
        auto id = req.url_params.get("id");
        if(!id) return crow::json::wvalue();

        NotificationSystem ns;
        return crow::json::wvalue(ns.getMyNotifications(std::stoi(id)));
    });

    CROW_ROUTE(app, "/ResetPassword.html")
    ([](){
        auto page = crow::mustache::load_text("ResetPassword.html");
        return crow::response(page);
    });

    CROW_ROUTE(app,"/UpdatePassword.html")([](){
        return crow::mustache::load_text("UpdatePassword.html");
    });

    // Serve Register Page
    CROW_ROUTE(app, "/Register.html")
    ([](){
        auto page = crow::mustache::load_text("Register.html");
        return crow::response(page);
    });

    CROW_ROUTE(app, "/Dashboard.html")
    ([](){
        auto page = crow::mustache::load_text("Dashboard.html");
        return crow::response(page);
    });

    CROW_ROUTE(app, "/Profile.html")
    ([](){
        auto page = crow::mustache::load_text("Profile.html");
        return crow::response(page);
    });

    CROW_ROUTE(app, "/incident.html")
    ([](){
        auto page = crow::mustache::load_text("incident.html");
        return crow::response(page);
    });

    CROW_ROUTE(app, "/escort.html")
    ([](){
        auto page = crow::mustache::load_text("escort.html");
        return crow::response(page);
    });

    CROW_ROUTE(app, "/report.html")
    ([](){
        auto page = crow::mustache::load_text("report.html");
        return crow::response(page);
    });

    CROW_ROUTE(app, "/resources.html")
    ([](){
        auto page = crow::mustache::load_text("resources.html");
        return crow::response(page);
    });

    CROW_ROUTE(app, "/manage_users.html")([](){
        auto page = crow::mustache::load_text("manage_users.html");
        return crow::response(page);
    });
    
    // API: Get List of All Users
    CROW_ROUTE(app, "/api/get_all_users")([](){
        Admin admin;
        auto list = admin.getAllUsers();
        return crow::json::wvalue(list);
    });

    // Route for Registration - aligns with User Profile Management
    CROW_ROUTE(app, "/register").methods("POST"_method)([](const crow::request& req) {

        // Capture form parameters
        auto params = crow::query_string("?" + req.body);
        std::string name = params.get("name") ? params.get("name") : "";
        std::string email = params.get("email") ? params.get("email") : "";
        std::string pass = params.get("password") ? params.get("password") : "";
        std::string phone = params.get("phone") ? params.get("phone") : "";
        std::string role = params.get("role") ? params.get("role") : "";
        std::string SQ1 = params.get("SQ1") ? params.get("SQ1") : "";
        std::string SQ2 = params.get("SQ2") ? params.get("SQ2") : "";

        std::string course = params.get("course") ? params.get("course") : "";
        std::string year = params.get("year") ? params.get("year") : "0";

        // 1. Validation Logic
        if(!IsValidName(name)) return crow::response(400, "Invalid Name.");
        if(!hasSpecialChar(pass)) return crow::response(400, "Password needs a special character.");
        if(role == "Student" && (course.empty() || year.empty())) return crow::response(400, "Student are required to fill out their course and year of study.");


        User curUser(name,email,pass,role,phone,SQ1,SQ2);
        bool success = curUser.Register();

        if(success){
            bool roleinsertion = curUser.Register(course, year);
            if(roleinsertion) {
                crow::response resp;
                resp.code = 303; // 303 See Other (Standard for redirect after POST)
                resp.set_header("Location", "/"); // Redirects to the root (Welcome.html)
                return resp;
            }
        }
        return crow::response(400, "Error: Ensure you enter valid data in required field.");
    });

    // Route for Login - implements Business Rule 1 and Exception E1 [cite: 255, 304]
    CROW_ROUTE(app, "/login").methods("POST"_method)([](const crow::request& req) {
        auto params = crow::query_string("?" + req.body);
        std::string identifier = params.get("identifier") ? params.get("identifier") : "";
        std::string pass = params.get("password")? params.get("password") : "";
        if(identifier.empty() || pass.empty()) return crow::response(400, "Missing credentials");

        User curUser;
        bool success = curUser.Login(identifier, pass);

        if (success) {
            crow::response resp;
            // Redirection code: 303
            resp.code = 303; 
            // 2. Pass the ID to the Dashboard
            resp.set_header("Location", "/Dashboard.html?id=" + std::to_string(curUser.Getid()) + "&name=" + curUser.Getname() + "&role=" + curUser.Getrole());
            return resp;
        } 
        return crow::response(401, "FAIL: Invalid Credentials"); 
    });

    CROW_ROUTE(app, "/reset").methods("POST"_method)([](const crow::request& req){
        auto params = crow::query_string("?" + req.body);
        std::string DOB = params.get("DOB");
        std::string hometown = params.get("hometown");

        DBConnection db;
        std::string query = "SELECT user_id FROM User WHERE SQ1 = '" + DOB + "' AND SQ2 = '" + hometown + "';";

        mysql_query(db.conn, query.c_str());
        MYSQL_RES* res = mysql_store_result(db.conn);

        if(res && mysql_num_rows(res) > 0){
            MYSQL_ROW row = mysql_fetch_row(res);
            std::string userID = row[0];
            mysql_free_result(res);
            crow::response resp;
            resp.code = 303;
            resp.set_header("Location", "/UpdatePassword.html?id=" + userID);
            return resp;
        }
        else{
            mysql_free_result(res);
            return crow::response(401, "FAIL: INVALID ANSWER.");
        }
    });

    CROW_ROUTE(app, "/apply_reset").methods("POST"_method)([](const crow::request& req){
        auto params = crow::query_string("?" + req.body);
        std::string userId = params.get("user_id");
        std::string newPass = params.get("new_password");

        // Security Check: Re-apply your special character check
        if(!hasSpecialChar(newPass)) return crow::response(400, "Password must contain a special character.");

        DBConnection db;
        std::string alter = "UPDATE User SET password = '" + newPass + "' WHERE user_id = " + userId;
        
        if (mysql_query(db.conn, alter.c_str())) {
            return crow::response(500, "Database Error");
        }
        
        // Send them back to login with success message
        crow::response resp;
        resp.code = 303;
        resp.set_header("Location", "/?status=success");
        return resp;
    });

    CROW_ROUTE(app, "/profile")
    ([](const crow::request& req) {
        // 1. Get the ID passed from the Dashboard URL
        auto userId = req.url_params.get("id");
        
        // Both return paths must now return crow::response
        if (!userId) return crow::response(400, "User ID missing");

        MYSQL* conn = mysql_init(NULL);
        mysql_real_connect(conn, "localhost", "root", "Zxcv@123", "cmdb", 0, NULL, 0);

        // 2. Use the ID in the WHERE clause to get the specific user
        std::string query = "SELECT name, email, role, contact_num, user_id FROM user WHERE user_id = " + std::string(userId);
        if(mysql_query(conn, query.c_str()) != 0){
            return crow::response(400, "Profile cannot be loaded.");
        }
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);

        crow::mustache::context ctx;
        if (row) {
            ctx["name"] = row[0];
            ctx["email"] = row[1];
            ctx["role"] = row[2];
            ctx["phone"] = row[3];
            ctx["user_id"] = row[4];
        }

        mysql_free_result(res);
        mysql_close(conn);

        // 3. FIX: Wrap the render in a response object
        return crow::response(crow::mustache::load("Profile.html").render(ctx));
    });

    CROW_ROUTE(app, "/submit_incident").methods("POST"_method)([](const crow::request& req) {
        auto params = crow::query_string("?" + req.body);
        
        // Extract parameters
        int userId = params.get("user_id") ? std::stoi(params.get("user_id")) : -1;
        std::string category = params.get("category") ? params.get("category") : "";
        std::string description = params.get("description") ? params.get("description") : "";
        std::string location = params.get("location") ? params.get("location") : "";

        if(category.empty() || description.empty() || location.empty())
            return crow::response(400, "Submission Failed: Ensure you enter valid data in required field.");

        if(userId == -1)
            return crow::response(400, "Error: User id extraction failed.");

        Student curStudent;
        int studentId = curStudent.GetStudentID(std::to_string(userId));
        if(studentId == -1) return crow::response(400, "Error: Student id extraction failed.");

        bool success = curStudent.reportIncident(category,description,location);

        if(success)
            return crow::response(200, "Incident Reported Successfully. Staff has been notified.");
        else
            return crow::response(400, "Server Error. Please call Campus Emergency Hotline directly.");
    });

    // 1. Fetch all incidents for the Staff Log
    CROW_ROUTE(app, "/get_all_incidents")([]() {
        MYSQL* conn = mysql_init(NULL);
        mysql_real_connect(conn, "localhost", "root", "Zxcv@123", "cmdb", 0, NULL, 0);

        mysql_query(conn, "SELECT incident_id, category, location, status FROM Incident ORDER BY time_reported DESC");
        MYSQL_RES* res = mysql_store_result(conn);
        
        std::vector<crow::json::wvalue> incidentList;
        while (MYSQL_ROW row = mysql_fetch_row(res)) {
            crow::json::wvalue inc;
            inc["id"] = row[0];
            inc["category"] = row[1];
            inc["location"] = row[2];
            inc["status"] = row[3];
            incidentList.push_back(std::move(inc));
        }
        
        mysql_free_result(res);
        mysql_close(conn);
        return crow::json::wvalue(incidentList);
    });

    // 2. Update status from 'Open' to 'Solved' [cite: 186]
    CROW_ROUTE(app, "/update_incident_status").methods("POST"_method)([](const crow::request& req) {
        auto id = req.url_params.get("id");
        auto status = req.url_params.get("status");
        auto userIdPtr = req.url_params.get("userid");
        if(!userIdPtr) return crow::response(400, "Missing User ID");
        string userid = userIdPtr;

        MYSQL* conn = mysql_init(NULL);
        mysql_real_connect(conn, "localhost", "root", "Zxcv@123", "cmdb", 0, NULL, 0);


        Staff s;
        int staffid = s.GetStaffID(userid);
        std::string query = "UPDATE Incident SET status = '" + std::string(status) + "', responded_by_staff_id = '" + std::to_string(staffid) + "' WHERE incident_id = " + std::string(id);
        mysql_query(conn, query.c_str());

        if(std::string(status) == "Solved") {
            // 1. We need to find WHO reported this incident first
            DBConnection db;
            string q = "SELECT S.user_id, I.category, I.location FROM Incident I Join Student S ON I.reported_by_student_id = S.student_id WHERE incident_id = " + std::string(id);
            if (mysql_query(db.conn, q.c_str()) == 0) {
                MYSQL_RES* res = mysql_store_result(db.conn);
                MYSQL_ROW row = mysql_fetch_row(res);
                
                if (row) {
                    int userId = std::stoi(row[0]); // <--- We found the student!
                    std::string category = row[1];
                    std::string location = row[2];
                    
                    // 3. Send the Notification
                    NotificationSystem ns;
                    ns.sendNotification(userId, "Your Incident regarding the " + category + " at location " + location + " has been marked as SOLVED.");
                }
                mysql_free_result(res);
            }
        }
       

        mysql_close(conn);
        return crow::response(200, "Status Updated");
    });

    CROW_ROUTE(app, "/submit_report").methods("POST"_method)([](const crow::request& req){
        auto id = req.url_params.get("id");
        auto title = req.url_params.get("title");
        auto description = req.url_params.get("content"); // Frontend sends 'content', backend stores 'description'
        auto place = req.url_params.get("place");

        // Check if any value is missing (nullptr check)
        if(!id || !title || !description || !place) {
            return crow::response(400, "Failed to load the report: Missing parameters.");
        }

        Staff s;
        int staffid = s.GetStaffID(id);
        if(staffid == -1) return crow::response(400, "Error: Staff id extraction failed.");
        bool success = s.SubmitReport(title,description,place);

        if(success) return crow::response(200, "Report Inserted Successfully.");
        else
            return crow::response(400, "Submission Failed: Ensure you enter valid data in required field.");
    });

    CROW_ROUTE(app, "/api/reports")([](const crow::request& req){
        auto idStr = req.url_params.get("id");
        auto role = req.url_params.get("role");

        int queryID = -1;

        // 2. LOGIC FIX: Check Role BEFORE checking Staff ID
        if(std::string(role) == "Admin") {
            queryID = 0; // Admins don't need a specific ID, they see everything
        } 
        else {
            // Only look up Staff ID if they are actually Staff/Security
            Staff s;
            queryID = s.GetStaffID(idStr);
            
            // If they are not Admin AND not in Staff table, block them.
            if(queryID == -1) {
                std::vector<crow::json::wvalue> empty;
                return crow::json::wvalue(empty);
            }
        }

        ReportSystem rs;
        // The class decides whether to return ALL or FEW based on 'role'
        auto data = rs.fetchReports(queryID, role);
        
        return crow::json::wvalue(data);
    });

    CROW_ROUTE(app, "/api/admin_update_user").methods("POST"_method)([](const crow::request& req){
        auto id = req.url_params.get("id");
        auto name = req.url_params.get("name");
        auto email = req.url_params.get("email");
        auto role = req.url_params.get("role");
        auto phone = req.url_params.get("phone");
        auto status = req.url_params.get("status"); // <--- Capture Status

        if(!id || !name || !email || !role || !phone || !status) 
            return crow::response(400, "Missing fields");

        Admin admin;
        // Pass 'status' to the function
        if(admin.updateUserProfile(std::stoi(id), name, email, role, phone, status))
            return crow::response(200, "User Updated Successfully");
        else
            return crow::response(500, "Database Update Failed");
    });

    CROW_ROUTE(app, "/submit_escort").methods("POST"_method)([](const crow::request& req) {
        auto params = crow::query_string("?" + req.body);
        
        // Extract parameters
        int userId = params.get("user_id") ? std::stoi(params.get("user_id")) : -1;
        std::string description = params.get("description") ? params.get("description") : "";
        std::string location = params.get("location") ? params.get("location") : "";

        if(userId == -1)
            return crow::response(400, "Error: User id extraction failed.");

        Student curStudent;
        int studentId = curStudent.GetStudentID(std::to_string(userId));
        if(studentId == -1) return crow::response(400, "Error: Student id extraction failed.");

        bool success = curStudent.RequestEscort(description,location);

        if(success)
            return crow::response(200, "Escort requested Successfully. Security staff has been notified.");
        else
            return crow::response(400, "Submission Failed: Ensure you enter valid data in required field.");
    });

    CROW_ROUTE(app, "/get_all_requests")([]() {
        MYSQL* conn = mysql_init(NULL);
        mysql_real_connect(conn, "localhost", "root", "Zxcv@123", "cmdb", 0, NULL, 0);

        mysql_query(conn, "SELECT E.escort_id, U.name, E.description, E.location, E.status FROM Escort_Request E JOIN Student S ON E.requested_by_student_id = S.student_id JOIN User U ON S.user_id = U.user_id ORDER BY E.request_time DESC;");
        MYSQL_RES* res = mysql_store_result(conn);
        
        std::vector<crow::json::wvalue> requestList;
        while (MYSQL_ROW row = mysql_fetch_row(res)) {
            crow::json::wvalue req;
            req["id"] = row[0];
            req["name"] = row[1];
            req["description"] = row[2];
            req["location"] = row[3];
            req["status"] = row[4];
            requestList.push_back(std::move(req));
        }
        
        mysql_free_result(res);
        mysql_close(conn);
        return crow::json::wvalue(requestList);
    });

    CROW_ROUTE(app, "/update_escort_status").methods("POST"_method)([](const crow::request& req) {
        auto id = req.url_params.get("id");
        auto status = req.url_params.get("status");
        auto userIdPtr = req.url_params.get("userid");
        if(!userIdPtr) return crow::response(400, "Missing User ID");
        string userid = userIdPtr;

        MYSQL* conn = mysql_init(NULL);
        mysql_real_connect(conn, "localhost", "root", "Zxcv@123", "cmdb", 0, NULL, 0);

        SecurityStaff ss;
        int ssid = ss.GetSecStaffID(userid);
        std::string query = "UPDATE Escort_Request SET status = '" + std::string(status) + "', assigned_security_id = '" + std::to_string(ssid) + "' WHERE escort_id = " + std::string(id);
        mysql_query(conn, query.c_str());

        if(std::string(status) == "Assigned") {
            // 1. We need to find WHO reported this incident first
            DBConnection db;
            string q = "SELECT S.user_id, U.name, E.location FROM Escort_Request E JOIN Security_Staff SS ON E.assigned_security_id = SS.security_staff_id JOIN USER U ON SS.user_id = U.user_id JOIN Student S ON E.requested_by_student_id = S.student_id where E.escort_id = " + std::string(id);
            if (mysql_query(db.conn, q.c_str()) == 0) {
                MYSQL_RES* res = mysql_store_result(db.conn);
                MYSQL_ROW row = mysql_fetch_row(res);
                
                if (row) {
                    int userId = std::stoi(row[0]); // <--- We found the student!
                    std::string name = row[1];
                    std::string location = row[2];
                    
                    // 3. Send the Notification
                    NotificationSystem ns;
                    ns.sendNotification(userId, "Dear Student, Security Staff has noticed your escort requet. Please stay at " + location + ". Staff " + name + " is on the way.");
                }
                mysql_free_result(res);
            }
        }
       

        mysql_close(conn);
        return crow::response(200, "Status Updated");
    });

    // 1. Serve the Role-Based HTML
    CROW_ROUTE(app, "/digital_id.html")([](){
        return crow::response(crow::mustache::load_text("digital_id.html"));
    });

    // 2. API: Get ID Details (Used by both Student and Staff View)
    CROW_ROUTE(app, "/api/get_digital_id")([](const crow::request& req){
        auto idStr = req.url_params.get("id");
        if(!idStr) return crow::response(400, "Missing ID");
        
        Student st;
        DigitalIDData data = st.getDigitalIDDetails(std::stoi(idStr));
        
        if(data.found) {
            crow::json::wvalue json;
            json["name"] = data.name;
            json["course"] = data.course;
            json["status"] = data.academic_status;
            json["verify_status"] = data.verify_status;
            json["phone"] = data.phone;
            json["year"] = data.year;
            json["faculty"] = data.faculty;
            return crow::response(json);
        }
        return crow::response(404, "Not Found");
    });

    // 3. API: Staff Get Unverified List
    CROW_ROUTE(app, "/api/get_unverified_list")([](){
        Staff staff;
        auto list = staff.getUnverifiedStudents();
        return crow::json::wvalue(list);
    });

    // 4. API: Staff Verify Action
    CROW_ROUTE(app, "/api/verify_student").methods("POST"_method)([](const crow::request& req){
        auto targetId = req.url_params.get("target_id");
        auto action = req.url_params.get("action"); // "approve" or "reject"
        auto verifierId = req.url_params.get("verifier_id"); //staff id
        
        if(!targetId || !action) return crow::response(400, "Missing params");
        
        Staff staff;
        bool approved = (std::string(action) == "approve");
        if(approved) staff.GetStaffID(verifierId);
        
        if(staff.verifyStudent(std::stoi(targetId), approved))
            return crow::response(200, "Updated");
        else
            return crow::response(500, "DB Error");
    });

    // 1. TRIGGER ROUTE (Security Staff Only)
    CROW_ROUTE(app, "/api/trigger_emergency").methods("POST"_method)([](const crow::request& req){
        auto isActive = req.url_params.get("active");
        auto msg = req.url_params.get("message");
        
        EmergencySystem es;
        bool turnOn = (std::string(isActive) == "true");
        
        if(es.setEmergencyStatus(turnOn, msg ? msg : "EMERGENCY ALERT"))
            return crow::response(200, "Broadcast Sent");
        return crow::response(500, "Error");
    });

    // 2. CHECK ROUTE (All Users Poll This)
    CROW_ROUTE(app, "/api/check_emergency")([](){
        EmergencySystem es;
        return es.checkStatus();
    });

    CROW_ROUTE(app, "/api/manage_resource").methods("POST"_method)([](const crow::request& req){
        auto id = req.url_params.get("id");
        auto name = req.url_params.get("name");
        auto type = req.url_params.get("type");
        auto qty = req.url_params.get("quantity");
        auto loc = req.url_params.get("location");
        auto userIdParam = req.url_params.get("admin_id"); // This is actually the User ID from the frontend

        if(!name || !qty || !userIdParam) return crow::response(400, "Missing Info");

        // --- NEW LOGIC START: Convert User ID to Admin ID ---
        Admin adminObj;
        int realAdminID = adminObj.GetAdminID(userIdParam);
        
        // If we can't find an Admin ID for this user, stop.
        if (realAdminID == -1) {
            std::cerr << "Error: User " << userIdParam << " is not in the Admin table." << std::endl;
            return crow::response(400, "Error: User is not an Admin.");
        }
        // --- NEW LOGIC END ---

        ResourceSystem rs;
        int rID = (id && std::string(id) != "") ? std::stoi(id) : -1;

        // CRITICAL CHANGE: We pass 'realAdminID', NOT 'std::stoi(userIdParam)'
        if(rs.saveResource(rID, name, type ? type : "", std::stoi(qty), loc ? loc : "", realAdminID))
            return crow::response(200, "Saved");
        
        return crow::response(500, "Error");
    });

    // 1. API: Get List of Resources
    CROW_ROUTE(app, "/api/get_resources")([](){
        ResourceSystem rs;
        // This functions fetches data from DB and returns JSON
        return crow::json::wvalue(rs.getAllResources());
    });

    // 2. API: Delete Resource
    CROW_ROUTE(app, "/api/delete_resource").methods("POST"_method)([](const crow::request& req){
        auto id = req.url_params.get("id");
        if(!id) return crow::response(400, "Missing ID");

        ResourceSystem rs;
        if(rs.deleteResource(std::stoi(id))) return crow::response(200, "Deleted");
        return crow::response(500, "Error");
    });

    // 1. Serve HTML
    CROW_ROUTE(app, "/patrol")([](){
        return crow::response(crow::mustache::load_text("patrol_route.html"));
    });

    // 2. API: Get Data
    CROW_ROUTE(app, "/api/patrol/get")([](const crow::request& req){
        auto id = req.url_params.get("id"); // User ID (User 17)
        if(!id) return crow::json::wvalue();
        
        SecurityStaff ss;
        int secId = ss.GetSecStaffID(id); // CONVERT TO SECURITY ID (e.g., 2)
        
        PatrolSystem ps;
        return crow::json::wvalue(ps.getPatrolDashboard(secId));
    });

    // 3. API: Assign Route
    CROW_ROUTE(app, "/api/patrol/assign").methods("POST"_method)([](const crow::request& req){
        auto uId = req.url_params.get("uid");
        auto rId = req.url_params.get("rid");
        
        SecurityStaff ss;
        int secId = ss.GetSecStaffID(uId);
        
        PatrolSystem ps;
        if(ps.assignRoute(std::stoi(rId), secId)) return crow::response(200, "Assigned");
        return crow::response(400, "Failed");
    });

    // 4. API: Sign Checkpoint
    CROW_ROUTE(app, "/api/patrol/sign").methods("POST"_method)([](const crow::request& req){
        auto rId = req.url_params.get("rid");
        auto idx = req.url_params.get("index");
        
        PatrolSystem ps;
        if(ps.signCheckpoint(std::stoi(rId), std::stoi(idx))) return crow::response(200, "Signed");
        return crow::response(400, "Failed");
    });

    // 5. API: Complete/Reset Patrol
    CROW_ROUTE(app, "/api/patrol/complete").methods("POST"_method)([](const crow::request& req){
        auto rId = req.url_params.get("rid");
        PatrolSystem ps;
        if(ps.completePatrol(std::stoi(rId))) return crow::response(200, "Completed");
        return crow::response(400, "Failed");
    });

    CROW_ROUTE(app, "/icon.png")([](){
        crow::response res;
        // Make sure icon.png is in the same folder as your .exe file!
        res.set_static_file_info("icon.png"); 
        return res;
    });


    CROW_ROUTE(app, "/barcode.png")([](){
        crow::response res;
        res.set_static_file_info("barcode.png");
        return res;
    });


    // Call this right before starting the app
    std::cout << "Starting server at http://localhost:18080..." << std::endl;
    openBrowser("http://localhost:18080");

    app.port(18080).multithreaded().run();
}