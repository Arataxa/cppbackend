#include "postgres.h"

#include <pqxx/zview.hxx>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;

void AuthorRepositoryImpl::Save(const domain::Author& author) {
    pqxx::work work{connection_};
    work.exec_params(
        R"(
INSERT INTO authors (id, name) VALUES ($1, $2)
ON CONFLICT (id) DO UPDATE SET name=$2;
)"_zv,
        author.GetId().ToString(), author.GetName());
    work.commit();
}

std::vector<domain::Author> AuthorRepositoryImpl::GetAll() {
    pqxx::work txn{ connection_ };
    auto result = txn.exec("SELECT id, name FROM authors ORDER BY name");

    std::vector<domain::Author> authors;
    for (size_t i = 0; i < result.size(); ++i) {
        const auto& row = result[i];
        authors.push_back({
            domain::AuthorId::FromString(row["id"].as<std::string>()),
            row["name"].as<std::string>()
            });
    }

    return authors;
}

void BookRepositoryImpl::Save(const domain::Book& book) {
    pqxx::work work{ connection_ };
    work.exec_params(
        "INSERT INTO books (id, title, author_id, publication_year) VALUES ($1, $2, $3, $4)",
        book.GetBookId().ToString(), book.GetTitle(), book.GetAuthorId().ToString(), book.GetPublicationYear());
    work.commit();
}

std::vector<domain::Book> BookRepositoryImpl::GetAll() {
    pqxx::work txn{ connection_ };
    auto result = txn.exec("SELECT id, title, author_id, publication_year FROM books ORDER BY title");

    std::vector<domain::Book> books;
    for (size_t i = 0; i < result.size(); ++i) {
        const auto& row = result[i];
        books.push_back({
            domain::BookId::FromString(row["id"].as<std::string>()),
            domain::AuthorId::FromString(row["author_id"].as<std::string>()),
            row["title"].as<std::string>(),
            row["publication_year"].as<int>()
            });
    }

    return books;
}

std::vector<domain::Book> BookRepositoryImpl::GetAuthorBooks(const domain::AuthorId& author_id) {
    pqxx::work txn{ connection_ };
    auto result = txn.exec_params("SELECT id, title, author_id, publication_year FROM books WHERE author_id = $1 ORDER BY publication_year, title",
        author_id.ToString());

    std::vector<domain::Book> books;
    for (size_t i = 0; i < result.size(); ++i) {
        const auto& row = result[i];
        books.push_back({
            domain::BookId::FromString(row["id"].as<std::string>()),
            domain::AuthorId::FromString(row["author_id"].as<std::string>()),
            row["title"].as<std::string>(),
            row["publication_year"].as<int>()
            });
    }

    return books;
}


Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(
CREATE TABLE IF NOT EXISTS authors (
    id UUID PRIMARY KEY,
    name VARCHAR(100) UNIQUE NOT NULL
);
)"_zv);
    work.exec(R"(
CREATE TABLE IF NOT EXISTS books (
    id UUID PRIMARY KEY,
    author_id UUID NOT NULL,
    title VARCHAR(100) NOT NULL,
    publication_year INT NOT NULL,
    FOREIGN KEY (author_id) REFERENCES authors (id)
);
)"_zv);
    work.commit();
}
}  // namespace postgres