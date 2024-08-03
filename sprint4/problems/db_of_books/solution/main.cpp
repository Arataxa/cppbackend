#include "boost/json.hpp"
#include <pqxx/pqxx>

#include <stdexcept>
#include <iostream>

namespace json = boost::json;

class DBManager {
public:
    DBManager(const std::string& connection_str) {
        conn = new pqxx::connection(connection_str);

        // Создание таблицы, если она не существует
        pqxx::work W(*conn);
        W.exec("CREATE TABLE IF NOT EXISTS books ("
            "id SERIAL PRIMARY KEY, "
            "title VARCHAR(100) NOT NULL, "
            "author VARCHAR(100) NOT NULL, "
            "year INTEGER NOT NULL, "
            "ISBN CHAR(13) UNIQUE"
            ")");
        W.commit();
    }

    ~DBManager() {
        delete conn;
    }

    bool addBook(const std::string& title, const std::string& author, int year, const std::string& isbn) {
        try {
            pqxx::work W(*conn);

            if (!isbn.empty()) {
                W.exec_params("INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)",
                    title, author, year, isbn);
            }
            else {
                W.exec_params("INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, NULL)",
                    title, author, year);
            }

            W.commit();
            return true;
        }
        catch (const pqxx::sql_error& e) {
            std::cerr << "SQL error: " << e.what() << std::endl;
            return false;
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return false;
        }
    }

    std::vector<std::tuple<int, std::string, std::string, int, std::string>> getAllBooks() {
        pqxx::work W(*conn);

        pqxx::result R = W.exec("SELECT id, title, author, year, ISBN FROM books "
            "ORDER BY year DESC, title ASC, author ASC, ISBN ASC");
        W.commit();

        std::vector<std::tuple<int, std::string, std::string, int, std::string>> books;

        for (const auto& row : R) {
            books.emplace_back(
                row[0].as<int>(),
                row[1].as<std::string>(),
                row[2].as<std::string>(),
                row[3].as<int>(),
                row[4].is_null() ? "" : row[4].as<std::string>() // Проверка на NULL
            );
        }

        return books;
    }
private:
    pqxx::connection* conn;
};

void processRequest(DBManager& db, const std::string& request) {
    auto req = json::parse(request).as_object();
    std::string action = json::value_to<std::string>(req["action"]);
    auto payload = req["payload"].as_object();

    if (action == "add_book") {
        std::string title = json::value_to<std::string>(payload["title"]);
        std::string author = json::value_to<std::string>(payload["author"]);
        int year = json::value_to<int>(payload["year"]);
        std::string isbn = payload["ISBN"].is_null() ? "" : json::value_to<std::string>(payload["ISBN"]);

        bool result = db.addBook(title, author, year, isbn);
        json::object response = { {"result", result} };
        std::cout << json::serialize(response) << std::endl;

    }
    else if (action == "all_books") {
        auto books = db.getAllBooks();
        json::array response;

        for (const auto& book : books) {
            response.push_back({
                {"id", std::get<0>(book)},
                {"title", std::get<1>(book)},
                {"author", std::get<2>(book)},
                {"year", std::get<3>(book)},
                {"ISBN", std::get<4>(book).empty() ? json::value() : json::value(std::get<4>(book))}
                });
        }
        std::cout << json::serialize(response) << std::endl;

    }
    else if (action == "exit") {
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <connection_string>" << std::endl;
        return 1;
    }

    std::string connection_str = argv[1];
    DBManager db(connection_str);

    std::string request;
    while (std::getline(std::cin, request)) {
        processRequest(db, request);
    }

    return 0;
}