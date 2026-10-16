
#include <iostream>
#include <mysql.h>
#include "argparse/argparse.hpp"
#include <string>
#include <fstream>
#include <filesystem>
#include <chrono>

void saveQueryToCSV(const char* server, const char* user, const char* password,
    const char* database, const char* query, std::filesystem::path path) {

    
	if (!std::filesystem::exists(path)) {
        std::cerr << "Directory does not exist." << std::endl;
        return;
	}

    MYSQL* conn = mysql_init(NULL);

    // Connect to database
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        std::cerr << "Connection Error: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return;
    }

    // Set the timezone
    const char* tz_query = "SET time_zone = '+00:00'";
    if (mysql_query(conn, tz_query)) {
        std::cerr << "TZ Query Error: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return;
    }

    // Execute query
    if (mysql_query(conn, query)) {
        std::cerr << "Query Error: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) {
        std::cerr << "Result Error: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        return;
    }

    auto now = std::chrono::system_clock::now();
    std::string filename = std::format("{:%Y%m%d_%H%M%S}", now);

    std::filesystem::path file_path = path / (filename + ".csv");

    std::ofstream csvFile(file_path);
    if (!csvFile.is_open()) {
        std::cerr << "Failed to open output file: " << filename << std::endl;
        mysql_free_result(res);
        mysql_close(conn);
        return;
    }

    int num_fields = mysql_num_fields(res);
    MYSQL_FIELD* field;

    // Write CSV Header
    for (int i = 0; i < num_fields; i++) {
        field = mysql_fetch_field_direct(res, i);
        csvFile << "\"" << field->name << "\"";
        if (i < num_fields - 1) csvFile << ",";
    }
    csvFile << "\n";

    // Write CSV Rows
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        unsigned long* lengths = mysql_fetch_lengths(res);
        for (int i = 0; i < num_fields; i++) {
            if (row[i]) {
                // Escape or wrap fields in quotes
                csvFile << "\"" << row[i] << "\"";
            }
            else {
                csvFile << "\"\""; // Handle NULL values
            }
            if (i < num_fields - 1) csvFile << ",";
        }
        csvFile << "\n";
    }

    csvFile.close();
    mysql_free_result(res);
    mysql_close(conn);
    std::cout << "Successfully exported query to " << filename << std::endl;
}

int main(int argc, char* argv[])
{
	argparse::ArgumentParser program("ExproCanteen", "1.0.0");

	program.add_argument("-p", "--path").help("Output path.").default_value(std::string("."));

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::exception& err) {
		std::cerr << "Error: " << err.what() << std::endl;
		return 1;
	}

	std::string path_str = program.get<std::string>("--path");

    std::filesystem::path path = path_str;

    const char* query = "SELECT * FROM monthly where timestamp > (UNIX_TIMESTAMP() - 86400);";

    saveQueryToCSV("192.168.1.100", "root", "root", "main_db", query, path);

    return 0;

}

