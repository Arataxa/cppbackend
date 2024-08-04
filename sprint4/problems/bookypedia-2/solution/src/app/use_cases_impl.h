#pragma once

#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

#include <stdexcept>

namespace app {

class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books);

    void AddAuthor(const std::string& name) override;

    void DeleteAuthor(const std::string& author_id) override;

    void EditAuthorName(const std::string& author_id, const std::string& new_name) override;

    void AddBook(const std::string& author_id, const std::string& title, int publication_year, std::vector<std::string> tags) override;

    void DeleteBook(const std::string& book_id) override;

    void EditBook(const std::string& book_id, const std::optional<std::string>& new_title,
        const std::optional<int>& new_pub_year, const std::vector<std::string>& new_tags) override;

    std::vector<domain::Author> GetAuthorsList() override;

    std::vector<std::pair<domain::Book, std::string>> GetBooksList() override;

    std::vector<domain::Book> GetAuthorBooks(const std::string& author_id) override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app
