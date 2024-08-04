#include "use_cases_impl.h"

#include "../domain/book.h"

namespace app {
using namespace domain;

UseCasesImpl::UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
    : authors_{ authors }, books_{ books } {
}

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::DeleteAuthor(const std::string& author_id) {
    authors_.Delete(domain::AuthorId::FromString(author_id));
}

void UseCasesImpl::EditAuthorName(const std::string& author_id, const std::string& new_name) {
    authors_.EditName(domain::AuthorId::FromString(author_id), new_name);
}

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year, std::vector<std::string> tags) {
    auto author_id_obj = domain::AuthorId::FromString(author_id);
    books_.Save({ BookId::New(), author_id_obj, title, publication_year , tags});
}

void UseCasesImpl::DeleteBook(const std::string& book_id) {
    books_.Delete(domain::BookId::FromString(book_id));
}

void UseCasesImpl::EditBook(const std::string& book_id, const std::optional<std::string>& new_title,
    const std::optional<int>& new_pub_year, const std::vector<std::string>& new_tags) {
    books_.EditBook(domain::BookId::FromString(book_id), new_title, new_pub_year, new_tags);
}

std::vector<domain::Author> UseCasesImpl::GetAuthorsList() {
    return authors_.GetAll();
}

std::vector<std::pair<domain::Book, std::string>> UseCasesImpl::GetBooksList() {
    return books_.GetAll();
}

std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    return books_.GetAuthorBooks(domain::AuthorId::FromString(author_id));
}

}  // namespace app
