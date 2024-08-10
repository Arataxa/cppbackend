#pragma once

#include <string>
#include <vector>

#include "../domain/author.h"
#include "../util/tagged_uuid.h"

namespace domain {

namespace detail {
struct BookTag {};
}  // namespace detail

using BookId = util::TaggedUUID<detail::BookTag>;

class Book {
public:
    Book(BookId book_id, AuthorId author_id, std::string title, int publication_year, std::vector<std::string> tags)
        : book_id_(std::move(book_id))
        , author_id_(std::move(author_id))
        , title_(std::move(title))
        , publication_year_(publication_year)
        , tags_(std::move(tags)){
    }

    const BookId& GetBookId() const noexcept {
        return book_id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    int GetPublicationYear() const noexcept {
        return publication_year_;
    }

    const std::vector<std::string>& GetTags() const noexcept {
        return tags_;
    }

private:
    BookId book_id_;
    AuthorId author_id_;
    std::string title_;
    int publication_year_ = 0;
    std::vector<std::string> tags_;
};

class BookRepository {
public:
    virtual void Save(const Book& book) = 0;

    virtual void Delete(const BookId& book_id) = 0;

    virtual std::vector<std::pair<domain::Book, std::string>> GetAll() = 0;

    virtual std::vector<Book> GetAuthorBooks(const AuthorId& author_id) = 0;

    virtual void EditBook(const BookId& book_id, const std::optional<std::string>& new_title,
        const std::optional<int>& new_pub_year, const std::vector<std::string>& new_tags) = 0;
protected:
    ~BookRepository() = default;
};

}  // namespace domain
