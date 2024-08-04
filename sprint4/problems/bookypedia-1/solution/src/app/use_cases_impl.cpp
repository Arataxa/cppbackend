#include "use_cases_impl.h"

#include "../domain/book.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    if (name.empty()) {
        throw std::runtime_error("Empty author name");
    }

    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(const std::string& author_id, const std::string& title, int publication_year) {
    auto author_id_obj = domain::AuthorId::FromString(author_id);
    books_.Save({ BookId::New(), author_id_obj, title, publication_year });
}

std::vector<domain::Author> UseCasesImpl::GetAuthorsList() {
    return authors_.GetAll();
}

std::vector<domain::Book> UseCasesImpl::GetBooksList() {
    return books_.GetAll();
}

std::vector<domain::Book> UseCasesImpl::GetAuthorBooks(const std::string& author_id) {
    return books_.GetAuthorBooks(domain::AuthorId::FromString(author_id));
}

}  // namespace app
