#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"
#include "../util/tagged_uuid.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::connection& connection)
        : connection_{connection} {
    }

    void Save(const domain::Author& author) override;

    void Delete(const domain::AuthorId& author_id) override;

    std::vector<domain::Author> GetAll() override;

    void EditName(const domain::AuthorId& author_id, const std::string& new_name) override;
private:
    pqxx::connection& connection_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::connection& connection)
        : connection_{ connection } {
    }

    void Save(const domain::Book& book) override;

    void Delete(const domain::BookId& book_id) override;

    std::vector<std::pair<domain::Book, std::string>> GetAll() override;

    std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id);

    void EditBook(const domain::BookId& book_id, const std::optional<std::string>& new_title,
        const std::optional<int>& new_pub_year, const std::vector<std::string>& new_tags) override;
private:
    pqxx::connection& connection_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    AuthorRepositoryImpl& GetAuthors() & {
        return authors_;
    }

    BookRepositoryImpl& GetBooks() & {
        return books_;
    }

private:
    pqxx::connection connection_;
    AuthorRepositoryImpl authors_{connection_};
    BookRepositoryImpl books_{ connection_ };
};

}  // namespace postgres