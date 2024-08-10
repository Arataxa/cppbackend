#pragma once

#include <vector>
#include <string>
#include <optional>
#include "../domain/book.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void DeleteAuthor(const std::string& author_id) = 0;
    virtual void EditAuthorName(const std::string& author_id, const std::string& new_name) = 0;
    virtual void AddBook(const std::string& author_id, const std::string& title, int publication_year, std::vector<std::string> tags) = 0;
    virtual void EditBook(const std::string& book_id, const std::optional<std::string>& new_title,
        const std::optional<int>& new_pub_year, const std::vector<std::string>& new_tags) = 0;
    virtual void DeleteBook(const std::string& book_id) = 0;
    virtual std::vector<domain::Author> GetAuthorsList() = 0;
    virtual std::vector<std::pair<domain::Book, std::string>> GetBooksList() = 0;
    virtual std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app
