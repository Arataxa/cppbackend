#include "view.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string.hpp>
#include <cassert>
#include <iostream>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << " by "s << book.author_name << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
        // либо
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s, std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s, std::bind(&View::ShowAuthorBooks, this));
    menu_.AddAction("DeleteAuthor"s, "name"s, "Delete author and all his books"s, std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "name"s, "Edit author name"s, std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("ShowBook"s, "title"s, "Show book information"s, std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "title"s, "Delete book"s, std::bind(&View::DeleteBook, this, ph::_1));
    menu.AddAction("EditBook"s, "title"s, "Edit book's information"s, std::bind(&View::EditBook, this, ph::_1));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);

        if (name.empty()) {
            throw std::runtime_error("Empty author name");
        }

        boost::algorithm::trim(name);
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        if (auto params = GetBookParams(cmd_input)) {
            if (!params.has_value()) {
                throw std::runtime_error("Error");
            }

            auto params_value = params.value();
            use_cases_.AddBook(params_value.author_id, params_value.title, params_value.publication_year, params_value.tags);
        }
    } catch (const std::exception& e) {
        output_ << "Failed to add book: "sv << e.what() << std::endl;
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        std::string deleted_book_id;
        auto books = GetBooks();
        std::optional<int> book_idx;

        if (title.empty()) {
            book_idx = SelectBook(books);

            if (!book_idx.has_value()) {
                return true;
            }

            deleted_book_id = books[book_idx.value()].uid;
        }
        else {
            std::vector<detail::BookInfo> finding_books;

            std::copy_if(books.begin(), books.end(), std::back_inserter(finding_books), [&title](const detail::BookInfo& book) {
                return book.title == title;
                });

            if (finding_books.size() == 0) {
                output_ << "Book not found" << std::endl;
                return true;
            }

            if (finding_books.size() == 1) {
                deleted_book_id = finding_books.front().uid;
            }
            else {
                book_idx = SelectBook(finding_books);

                if (!book_idx.has_value()) {
                    return true;
                }

                deleted_book_id = finding_books[book_idx.value()].uid;
            }
        }

        use_cases_.DeleteBook(deleted_book_id);
    }
    catch (const std::exception& e) {
        output_ << "Failed to delete book: " << e.what()  << std::endl;
    }

    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        detail::BookInfo book;
        auto books = GetBooks();
        std::optional<int> book_idx;

        if (title.empty()) {
            book_idx = SelectBook(books);

            if (!book_idx.has_value()) {
                throw std::runtime_error("Wrong option selected 1");
            }

            book = books[book_idx.value()];
        }
        else {
            std::vector<detail::BookInfo> finding_books;

            std::copy_if(books.begin(), books.end(), std::back_inserter(finding_books), [&title](const detail::BookInfo& book) {
                return book.title == title;
                });

            if (finding_books.size() == 0) {
                throw std::runtime_error("Book not found");
            }

            if (finding_books.size() == 1) {
                book = finding_books.front();
            }
            else {
                book_idx = SelectBook(finding_books);

                if (!book_idx.has_value()) {
                    throw std::runtime_error("Wrong option selected 2");
                }

                book = finding_books[book_idx.value()];
            }
        }

        std::optional<std::string> new_title = book.title;
        std::optional<int> new_publication_year = book.publication_year;
        std::vector<std::string> new_tags;

        {
            output_ << "Enter new title or empty line to use the current one (" << book.title << "):" << std::endl;
            std::string line;
            std::getline(input_, line);
            boost::algorithm::trim(line);

            if (!line.empty()) {
                new_title = line;
            }
        }
        {
            output_ << "Enter publication year or empty line to use the current one (" << book.publication_year << "):" << std::endl;
            std::string line;
            std::getline(input_, line);
            boost::algorithm::trim(line);

            try {
                if (!line.empty()) {
                    new_publication_year = std::stoi(line);
                }
            }
            catch (std::exception const&) {
                throw std::runtime_error("Invalid publication year");
            }
        }
        {
            output_ << "Enter tags (current tags: "; 
            bool is_first = true;
            for (const auto& tag : book.tags) {
                if (is_first) {
                    is_first = false;
                } else {
                    output_ << ", "s;
                }

                output_ << tag;
            }
            output_ << "):" << std::endl;

            std::string tags_input;
            std::getline(input_, tags_input);

            std::istringstream tags_stream(tags_input);
            std::string tag;

            std::set<std::string> tags_set;
            while (std::getline(tags_stream, tag, ',')) {
                boost::algorithm::trim(tag);
                if (!tag.empty()) {
                    tags_set.insert(tag);
                }
            }

            if (!tags_set.empty()) {
                new_tags.assign(tags_set.begin(), tags_set.end());
            }
        }

        use_cases_.EditBook(book.uid, new_title, new_publication_year, new_tags);
        
    }
    catch (const std::exception& e) {
        output_ << "Book not found: " << e.what() << std::endl;
    }

    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowAuthorBooks() const {
    try {
        if (auto author_id = SelectAuthor()) {
            if (!author_id.has_value()) {
                return false;
            }

            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    try {
        std::string author_name;
        std::getline(cmd_input, author_name);
        boost::algorithm::trim(author_name);

        auto authors = GetAuthors();
        std::optional<std::string> author_id;

        if (author_name.empty()) {
            author_id = SelectAuthor(authors);
        }
        else {
            author_id = GetAuthorIdByName(author_name, authors);
        }

        if (!author_id.has_value()) {
            throw std::runtime_error("Fail");
        }

        use_cases_.DeleteAuthor(author_id.value());
    }
    catch (const std::exception& e) {
        output_ << "Failed to delete author" << std::endl;
    }

    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    try {
        std::string author_name;
        std::getline(cmd_input, author_name);
        boost::algorithm::trim(author_name);

        auto authors = GetAuthors();
        std::optional<std::string> author_id;

        if (author_name.empty()) {
            author_id = SelectAuthor(authors);
        }
        else {
            author_id = GetAuthorIdByName(author_name, authors);
        }

        if (!author_id.has_value()) {
            throw std::runtime_error("");
        }
        
        output_ << "Enter new name:" << std::endl;

        std::string new_name;
        std::getline(input_, new_name);
        boost::algorithm::trim(new_name);

        use_cases_.EditAuthorName(author_id.value(), new_name);
    }
    catch (const std::exception&) {
        output_ << "Failed to edit author" << std::endl;
    }

    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);

        auto books = GetBooks();
        detail::BookInfo book;
        if (title.empty()) {
            auto book_idx = SelectBook(books);

            if (!book_idx.has_value()) {
                throw std::logic_error("");
            }

            book = books[book_idx.value()];
        }
        else {
            std::vector<detail::BookInfo> finding_books;

            std::copy_if(books.begin(), books.end(), std::back_inserter(finding_books), [&title](const detail::BookInfo& book){
                return book.title == title;
                });

            if (finding_books.size() != 0) {
                if (finding_books.size() > 1) {
                    auto book_idx = SelectBook(finding_books);
                    if (!book_idx.has_value()) {
                        throw std::logic_error("");
                    }

                    book = finding_books[book_idx.value()];
                }
                else {
                    book = finding_books.front();
                }
            }
            else {
                throw std::logic_error("book not found");
            }
        }
        
        output_ << "Title: " << book.title << std::endl
            << "Author: " << book.author_name << std::endl
            << "Publication year: " << book.publication_year << std::endl;

        auto tags = book.tags;
        if (tags.size() != 0) {
            output_ << "Tags: ";
            
            bool is_first = true;
            for (const auto& tag : tags) {
                if (is_first) {
                    is_first = false;
                }
                else {
                    output_ << ", "s;
                }

                output_ << tag;
            }

            output_ << std::endl;
        }
    }
    catch (const std::exception&) {
    }

    return true;
}

std::optional<std::string> View::GetAuthorIdByName(const std::string& author_name) const {
    auto authors = GetAuthors();
    return GetAuthorIdByName(author_name, authors);
}
std::optional<std::string> View::GetAuthorIdByName(const std::string& author_name, std::vector<detail::AuthorInfo>& authors) const {
    auto it = std::find_if(authors.begin(), authors.end(), [&author_name](const detail::AuthorInfo& author) {
        return author.name == author_name;
        });

    if (it != authors.end()) {
        return it->id;
    }

    return std::nullopt;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    // get author
    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string author_name;
    std::getline(input_, author_name);
    boost::algorithm::trim(author_name);
    
    std::optional<std::string> author_id;
    auto authors = GetAuthors();
    if (author_name.empty()) {
        author_id = SelectAuthor(authors);
    }
    else {
        author_id = GetAuthorIdByName(author_name, authors);

        if (!author_id.has_value()) {
            output_ << "No author found. Do you want to add " << author_name << " (y/n)?" << std::endl;
            std::string response;
            std::getline(input_, response);
            if (response != "y" && response != "Y") {
                throw std::runtime_error("Failed to add book.");
            }
            try {
                use_cases_.AddAuthor(author_name);

                author_id = GetAuthorIdByName(author_name);

                if (!author_id.has_value()) {
                    throw std::runtime_error("Failed to add author");
                }
            }
            catch (const std::exception& e) {
                throw std::runtime_error(e.what());
            }
        }
    }

    output_ << "Enter tags (comma separated):" << std::endl;
    std::string tags_input;
    std::getline(input_, tags_input);

    std::istringstream tags_stream(tags_input);
    std::string tag;

    std::set<std::string> tags_set;
    while (std::getline(tags_stream, tag, ',')) {
        boost::algorithm::trim(tag);
        if (!tag.empty()) {
            tags_set.insert(tag);
        }
    }

    if (!author_id) {
        throw std::runtime_error("Failed to add book: No author selected.");
    }

    if (not author_id.has_value())
        return std::nullopt;
    else {
        params.author_id = author_id.value();
        params.tags.assign(tags_set.begin(), tags_set.end());
        return params;
    }
}

std::optional<int> View::SelectBook(std::vector<detail::BookInfo>& books) const {
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel:" << std::endl;

    std::string str;
    std::getline(input_, str);
    boost::algorithm::trim(str);
    if (str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    }
    catch (std::exception const&) {
        throw std::runtime_error("Invalid book num 1");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid book num 2");
    }

    return book_idx;
}

std::optional<std::string> View::SelectAuthor() const {
    auto authors = GetAuthors();
    return SelectAuthor(authors);
}

std::optional<std::string> View::SelectAuthor(std::vector<detail::AuthorInfo>& authors) const {
    output_ << "Select author:" << std::endl;
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    std::getline(input_, str);
    boost::algorithm::trim(str);
    if (str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num 1: " + str);
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num 2");
    }

    return authors[author_idx].id;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> dst_authors;

    for (auto& author : use_cases_.GetAuthorsList()) {
        dst_authors.emplace_back(detail::AuthorInfo{ author.GetId().ToString(), author.GetName() });
    }

    return dst_authors;
}

std::vector<detail::BookInfo> View::GetBooks() const {
    std::vector<detail::BookInfo> books;

    for (auto& [book, author_name] : use_cases_.GetBooksList()) {
        books.emplace_back(detail::BookInfo{ book.GetTitle(), book.GetBookId().ToString() , author_name, book.GetPublicationYear(), book.GetTags()});
    }

    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;
    
    for (auto& book : use_cases_.GetAuthorBooks(author_id)) {
        books.emplace_back(detail::BookInfo{ book.GetTitle(),"","", book.GetPublicationYear()});
    }

    return books;
}

}  // namespace ui
