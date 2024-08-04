#include <catch2/catch_test_macros.hpp>
#include "../src/app/use_cases_impl.h"

#include "../src/domain/book.h"

namespace {

    struct MockAuthorRepository : domain::AuthorRepository {
        std::vector<domain::Author> saved_authors;

        void Save(const domain::Author& author) override {
            saved_authors.emplace_back(author);
        }

        std::vector<domain::Author> GetAll() override {
            return saved_authors;
        }
    };

    struct MockBookRepository : domain::BookRepository {
        std::vector<domain::Book> saved_books;

        void Save(const domain::Book& book) override {
            saved_books.emplace_back(book);
        }

        std::vector<domain::Book> GetAll() override {
            return saved_books;
        }

        std::vector<domain::Book> GetAuthorBooks(const domain::AuthorId& author_id) override {
            std::vector<domain::Book> books_by_author;
            for (const auto& book : saved_books) {
                if (book.GetAuthorId() == author_id) {
                    books_by_author.push_back(book);
                }
            }
            return books_by_author;
        }
    };

    struct Fixture {
        MockAuthorRepository authors;
        MockBookRepository books;
    };

}  // namespace

SCENARIO_METHOD(Fixture, "Book Adding") {
    GIVEN("Use cases") {
        app::UseCasesImpl use_cases{ authors, books };

        WHEN("Adding an author") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == author_name);
                CHECK(authors.saved_authors.at(0).GetId() != domain::AuthorId{});
            }
        }

        WHEN("Adding a book") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);
            auto saved_authors = authors.GetAll();
            REQUIRE(saved_authors.size() == 1);

            const auto book_title = "Harry Potter and the Philosopher's Stone";
            const auto publication_year = 1997;
            use_cases.AddBook(saved_authors.at(0).GetId().ToString(), book_title, publication_year);

            THEN("book with the specified title is saved to repository") {
                REQUIRE(books.saved_books.size() == 1);
                CHECK(books.saved_books.at(0).GetTitle() == book_title);
                CHECK(books.saved_books.at(0).GetPublicationYear() == publication_year);
                CHECK(books.saved_books.at(0).GetAuthorId() == saved_authors.at(0).GetId());
            }
        }
    }
}