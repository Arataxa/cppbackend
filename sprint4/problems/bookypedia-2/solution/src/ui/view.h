#pragma once
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>
#include <sstream>
#include <set>

namespace menu {
class Menu;
}

namespace app {
class UseCases;
}

namespace ui {
namespace detail {

struct AddBookParams {
    std::string title;
    std::string author_id;
    int publication_year = 0;
    std::vector<std::string> tags;
};

struct AuthorInfo {
    std::string id;
    std::string name;
};

struct BookInfo {
    std::string title;
    std::string uid;
    std::string author_name;
    int publication_year;
    std::vector<std::string> tags;
};

}  // namespace detail

class View {
public:
    View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output);

private:
    bool AddAuthor(std::istream& cmd_input) const;
    bool ShowAuthors() const;
    bool ShowAuthorBooks() const;
    bool DeleteAuthor(std::istream& cmd_input) const;
    bool EditAuthor(std::istream& cmd_input) const;

    bool AddBook(std::istream& cmd_input) const;
    bool DeleteBook(std::istream& cmd_input) const;
    bool ShowBooks() const;
    bool ShowBook(std::istream& cmd_input) const;
    bool EditBook(std::istream& cmd_input) const;

    std::optional<detail::AddBookParams> GetBookParams(std::istream& cmd_input) const;
    std::optional<int> SelectBook(std::vector<detail::BookInfo>& authors_info) const;
    std::optional<std::string> SelectAuthor() const;
    std::optional<std::string> SelectAuthor(std::vector<detail::AuthorInfo>& authors_info) const;
    std::optional<std::string> GetAuthorIdByName(const std::string& author_name) const;
    std::optional<std::string> GetAuthorIdByName(const std::string& author_name, std::vector<detail::AuthorInfo>& authors_info) const;
    std::vector<detail::AuthorInfo> GetAuthors() const;
    std::vector<detail::BookInfo> GetBooks() const;
    std::vector<detail::BookInfo> GetAuthorBooks(const std::string& author_id) const;

    menu::Menu& menu_;
    app::UseCases& use_cases_;
    std::istream& input_;
    std::ostream& output_;
};

}  // namespace ui