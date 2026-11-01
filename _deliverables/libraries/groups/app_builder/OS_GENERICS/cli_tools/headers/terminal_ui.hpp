#ifndef COOLBOX_TERMINAL_UI_HPP
#define COOLBOX_TERMINAL_UI_HPP

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace os_generics {
namespace cli {
namespace tui {

enum class Key {
    text, escape, enter, tab, backspace, delete_key, left, right, up, down,
    home, end, page_up, page_down, unknown
};

struct KeyEvent {
    Key key = Key::unknown;
    char character = 0;
    bool shift = false;
    bool control = false;
    bool alt = false;
};

// A complete ASCII/ANSI key sequence, including xterm modifier parameters.
KeyEvent decode_key(const std::string& sequence);

enum class Style { normal, selected, cursor, heading, status };

class Frame {
public:
    Frame(std::size_t width, std::size_t height);
    std::size_t width() const { return width_; }
    std::size_t height() const { return height_; }
    void clear();
    void put(std::size_t x, std::size_t y, char character, Style style = Style::normal);
    void text(std::size_t x, std::size_t y, const std::string& text, Style style = Style::normal);
    void status_bar(const std::string& text);
    std::string plain_text() const;
    std::string ansi() const;

private:
    struct Cell {
        char character = ' ';
        Style style = Style::normal;
    };
    std::size_t width_;
    std::size_t height_;
    std::vector<Cell> cells_;
};

struct TerminalSize {
    std::size_t columns;
    std::size_t rows;
};

class TerminalSession {
public:
    TerminalSession();
    ~TerminalSession();
    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;
    TerminalSize size() const;
    std::optional<KeyEvent> read_key(std::chrono::milliseconds timeout = std::chrono::milliseconds(100));
    void present(const Frame& frame);
    // Call before destruction to report restoration failures explicitly.
    void close();

private:
    struct State;
    std::unique_ptr<State> state_;
};

struct TextPosition {
    std::size_t row = 0;
    std::size_t column = 0;
};

class DocumentEditor {
public:
    explicit DocumentEditor(const std::string& text = "");
    void set_text(const std::string& text);
    std::string text() const;
    void load(const std::filesystem::path& path);
    void save(const std::filesystem::path& path);
    bool handle(const KeyEvent& event);
    void render(Frame& frame);
    TextPosition cursor() const { return cursor_; }
    bool modified() const { return modified_; }

private:
    std::vector<std::string> lines_;
    TextPosition cursor_;
    std::size_t top_ = 0;
    std::size_t left_ = 0;
    std::size_t page_rows_ = 10;
    std::optional<std::size_t> preferred_column_;
    bool modified_ = false;
};

enum class SelectionMode { cells, rows };

class TableViewer {
public:
    TableViewer(std::vector<std::string> headers,
                std::vector<std::vector<std::string>> rows);
    bool handle(const KeyEvent& event);
    void render(Frame& frame);
    void set_selection_mode(SelectionMode mode);
    SelectionMode selection_mode() const { return mode_; }
    std::optional<TextPosition> active_cell() const;
    bool selected(std::size_t row, std::size_t column) const;
    std::vector<TextPosition> selection() const;
    void clear_selection();

private:
    std::vector<std::string> headers_;
    std::vector<std::vector<std::string>> rows_;
    std::vector<std::size_t> widths_;
    TextPosition active_;
    std::optional<TextPosition> anchor_;
    std::set<std::pair<std::size_t, std::size_t>> selected_;
    SelectionMode mode_ = SelectionMode::cells;
    std::size_t top_ = 0;
    std::size_t first_column_ = 0;
    std::size_t page_rows_ = 10;
};

class Menu {
public:
    explicit Menu(std::vector<std::string> items);
    std::optional<std::size_t> handle(const KeyEvent& event);
    void render(Frame& frame);
    std::optional<std::size_t> active_item() const;

private:
    std::vector<std::string> items_;
    std::size_t active_ = 0;
    std::size_t top_ = 0;
};

} // namespace tui
} // namespace cli
} // namespace os_generics

#endif
