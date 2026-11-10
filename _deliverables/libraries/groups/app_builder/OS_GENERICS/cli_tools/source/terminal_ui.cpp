#include "terminal_ui.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <poll.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace os_generics {
namespace cli {
namespace tui {
namespace {

void printable(const std::string& text) {
    for (unsigned char character : text) {
        if (character < 32 || character > 126) {
            throw std::invalid_argument("Terminal widgets require printable ASCII text");
        }
    }
}

void visible(std::size_t active, std::size_t count, std::size_t& start) {
    if (active < start) {
        start = active;
    } else if (count && active - start >= count) {
        start = active - count + 1;
    }
}

std::size_t down(std::size_t value, std::size_t amount, std::size_t maximum) {
    return value + std::min(amount, maximum - value);
}

const char* style_code(Style style) {
    switch (style) {
        case Style::selected: return "\x1b[0;30;46m";
        case Style::cursor: return "\x1b[0;7m";
        case Style::heading: return "\x1b[0;1m";
        case Style::status: return "\x1b[0;30;47m";
        default: return "\x1b[0m";
    }
}

void terminal_error(const char* operation) {
#ifdef _WIN32
    throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation);
#else
    throw std::system_error(errno, std::generic_category(), operation);
#endif
}

void output(const std::string& text) {
#ifdef _WIN32
    const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    std::size_t position = 0;
    while (position < text.size()) {
        DWORD written = 0;
        const DWORD count = static_cast<DWORD>(std::min<std::size_t>(text.size() - position, 65536));
        if (!WriteFile(handle, text.data() + position, count, &written, nullptr)) {
            terminal_error("Writing terminal output");
        }
        if (!written) {
            throw std::runtime_error("Terminal output made no progress");
        }
        position += written;
    }
#else
    std::size_t position = 0;
    while (position < text.size()) {
        const auto written = ::write(STDOUT_FILENO, text.data() + position, text.size() - position);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            terminal_error("Writing terminal output");
        }
        if (!written) {
            throw std::runtime_error("Terminal output made no progress");
        }
        position += static_cast<std::size_t>(written);
    }
#endif
}

} // namespace

KeyEvent decode_key(const std::string& sequence) {
    if (sequence.empty()) {
        throw std::invalid_argument("Key sequence cannot be empty");
    }
    KeyEvent event;
    if (sequence.size() == 1) {
        const auto character = static_cast<unsigned char>(sequence[0]);
        if (character == 27) event.key = Key::escape;
        else if (character == '\r' || character == '\n') event.key = Key::enter;
        else if (character == '\t') event.key = Key::tab;
        else if (character == 127 || character == 8) event.key = Key::backspace;
        else if (character >= 1 && character <= 26) {
            event = {Key::text, static_cast<char>('a' + character - 1), false, true, false};
        } else if (character >= 32 && character <= 126) {
            event = {Key::text, static_cast<char>(character)};
        }
        return event;
    }
    if (sequence[0] != '\x1b') {
        return event;
    }
    if (sequence.size() == 2 && sequence[1] != '[' && sequence[1] != 'O') {
        event = decode_key(sequence.substr(1));
        event.alt = true;
        return event;
    }
    if (sequence.size() < 3 || (sequence[1] != '[' && sequence[1] != 'O')) {
        return event;
    }
    const char final = sequence.back();
    std::string parameters = sequence.substr(2, sequence.size() - 3);
    const auto separator = parameters.find(';');
    if (separator != std::string::npos) {
        const auto modifier = parameters.substr(separator + 1);
        if (modifier.size() != 1 || modifier[0] < '1' || modifier[0] > '8') {
            return {};
        }
        const int flags = modifier[0] - '1';
        event.shift = (flags & 1) != 0;
        event.alt = (flags & 2) != 0;
        event.control = (flags & 4) != 0;
        parameters.resize(separator);
    }
    if (final == '~') {
        if (parameters == "1" || parameters == "7") event.key = Key::home;
        else if (parameters == "4" || parameters == "8") event.key = Key::end;
        else if (parameters == "3") event.key = Key::delete_key;
        else if (parameters == "5") event.key = Key::page_up;
        else if (parameters == "6") event.key = Key::page_down;
    } else if (parameters.empty() || parameters == "1") {
        if (final == 'A') event.key = Key::up;
        else if (final == 'B') event.key = Key::down;
        else if (final == 'C') event.key = Key::right;
        else if (final == 'D') event.key = Key::left;
        else if (final == 'H') event.key = Key::home;
        else if (final == 'F') event.key = Key::end;
        else if (final == 'Z') {
            event.key = Key::tab;
            event.shift = true;
        }
    }
    return event;
}

Frame::Frame(std::size_t width, std::size_t height) : width_(width), height_(height) {
    if (!width || !height || width > cells_.max_size() / height) {
        throw std::invalid_argument("Frame dimensions must be positive and fit in memory");
    }
    cells_.resize(width * height);
}

void Frame::put(std::size_t x, std::size_t y, char character, Style style) {
    printable(std::string(1, character));
    if (x < width_ && y < height_) {
        cells_[y * width_ + x] = {character, style};
    }
}

void Frame::clear() {
    std::fill(cells_.begin(), cells_.end(), Cell{});
}

void Frame::text(std::size_t x, std::size_t y, const std::string& text, Style style) {
    printable(text);
    if (x >= width_ || y >= height_) {
        return;
    }
    const auto count = std::min(width_ - x, text.size());
    for (std::size_t i = 0; i < count; ++i) {
        cells_[y * width_ + x + i] = {text[i], style};
    }
}

void Frame::status_bar(const std::string& text) {
    printable(text);
    for (std::size_t x = 0; x < width_; ++x) {
        put(x, height_ - 1, ' ', Style::status);
    }
    this->text(0, height_ - 1, text, Style::status);
}

std::string Frame::plain_text() const {
    std::string result;
    for (std::size_t y = 0; y < height_; ++y) {
        if (y) result += '\n';
        for (std::size_t x = 0; x < width_; ++x) {
            result += cells_[y * width_ + x].character;
        }
    }
    return result;
}

std::string Frame::ansi() const {
    std::string result = "\x1b[H";
    Style current = Style::normal;
    result += style_code(current);
    for (std::size_t y = 0; y < height_; ++y) {
        if (y) result += "\x1b[" + std::to_string(y + 1) + ";1H";
        for (std::size_t x = 0; x < width_; ++x) {
            const auto& cell = cells_[y * width_ + x];
            if (cell.style != current) {
                current = cell.style;
                result += style_code(current);
            }
            result += cell.character;
        }
    }
    return result + "\x1b[0m";
}

struct TerminalSession::State {
    bool active = false;
#ifdef _WIN32
    HANDLE input = INVALID_HANDLE_VALUE;
    HANDLE output = INVALID_HANDLE_VALUE;
    DWORD input_mode = 0;
    DWORD output_mode = 0;
    std::optional<KeyEvent> repeated_key;
    WORD repetitions = 0;
#else
    termios original{};
#endif
};

TerminalSession::TerminalSession() : state_(std::make_unique<State>()) {
#ifdef _WIN32
    state_->input = GetStdHandle(STD_INPUT_HANDLE);
    state_->output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!GetConsoleMode(state_->input, &state_->input_mode) ||
        !GetConsoleMode(state_->output, &state_->output_mode)) {
        terminal_error("TerminalSession requires console input and output");
    }
    const DWORD input_mode = (state_->input_mode | ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT) &
        ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE);
    if (!SetConsoleMode(state_->input, input_mode)) {
        terminal_error("Enabling terminal input");
    }
    if (!SetConsoleMode(state_->output, state_->output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING |
                        DISABLE_NEWLINE_AUTO_RETURN)) {
        const auto error = GetLastError();
        if (!SetConsoleMode(state_->input, state_->input_mode)) {
            std::fputs("Failed to restore terminal input during initialization\n", stderr);
        }
        throw std::system_error(static_cast<int>(error), std::system_category(), "Enabling ANSI output");
    }
#else
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        throw std::runtime_error("TerminalSession requires a terminal, not redirected streams");
    }
    if (tcgetattr(STDIN_FILENO, &state_->original) < 0) terminal_error("Reading terminal mode");
    auto raw = state_->original;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL | INLCR | IGNCR);
    raw.c_oflag &= ~OPOST;
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) terminal_error("Enabling raw terminal input");
#endif
    state_->active = true;
    try {
        output("\x1b[?1049h\x1b[?25l\x1b[?7l\x1b[2J\x1b[H");
    } catch (const std::exception&) {
        try {
            close();
        } catch (const std::exception& error) {
            std::fprintf(stderr, "Terminal restoration failed: %s\n", error.what());
        }
        throw;
    }
}

TerminalSession::~TerminalSession() {
    try {
        close();
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Terminal restoration failed: %s\n", error.what());
    }
}

void TerminalSession::close() {
    if (!state_->active) return;
    std::exception_ptr failure;
    try {
        output("\x1b[0m\x1b[?7h\x1b[?25h\x1b[?1049l");
    } catch (const std::exception&) {
        failure = std::current_exception();
    }
#ifdef _WIN32
    const bool restored_input = SetConsoleMode(state_->input, state_->input_mode) != 0;
    const DWORD input_error = restored_input ? 0 : GetLastError();
    const bool restored_output = SetConsoleMode(state_->output, state_->output_mode) != 0;
    if (!restored_input || !restored_output) {
        throw std::system_error(static_cast<int>(restored_input ? GetLastError() : input_error),
                                std::system_category(), "Restoring terminal modes");
    }
#else
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &state_->original) < 0) {
        terminal_error("Restoring terminal mode");
    }
#endif
    state_->active = false;
    if (failure) std::rethrow_exception(failure);
}

TerminalSize TerminalSession::size() const {
    if (!state_->active) throw std::logic_error("TerminalSession is closed");
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO information{};
    if (!GetConsoleScreenBufferInfo(state_->output, &information)) terminal_error("Reading terminal size");
    return {static_cast<std::size_t>(information.srWindow.Right - information.srWindow.Left + 1),
            static_cast<std::size_t>(information.srWindow.Bottom - information.srWindow.Top + 1)};
#else
    winsize information{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &information) < 0) terminal_error("Reading terminal size");
    if (!information.ws_col || !information.ws_row) throw std::runtime_error("Terminal size is zero");
    return {information.ws_col, information.ws_row};
#endif
}

void TerminalSession::present(const Frame& frame) {
    const auto dimensions = size();
    if (frame.width() > dimensions.columns || frame.height() > dimensions.rows) {
        throw std::invalid_argument("Frame exceeds terminal dimensions");
    }
    output("\x1b[2J" + frame.ansi());
}

std::optional<KeyEvent> TerminalSession::read_key(std::chrono::milliseconds timeout) {
    if (!state_->active) throw std::logic_error("TerminalSession is closed");
    if (timeout.count() < 0 || timeout.count() > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Key timeout must fit a nonnegative millisecond count");
    }
#ifdef _WIN32
    if (state_->repetitions) {
        --state_->repetitions;
        return state_->repeated_key;
    }
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
        const auto remaining = std::max<std::int64_t>(0,
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count());
        const auto wait = WaitForSingleObject(state_->input, static_cast<DWORD>(remaining));
        if (wait == WAIT_TIMEOUT) return {};
        if (wait != WAIT_OBJECT_0) terminal_error("Waiting for terminal input");
        INPUT_RECORD record{};
        DWORD read = 0;
        if (!ReadConsoleInputW(state_->input, &record, 1, &read)) terminal_error("Reading terminal key");
        if (!read) throw std::runtime_error("Console input ended");
        if (record.EventType != KEY_EVENT || !record.Event.KeyEvent.bKeyDown) {
            if (std::chrono::steady_clock::now() >= deadline) return {};
            continue;
        }
        const auto& key = record.Event.KeyEvent;
        KeyEvent event;
        event.shift = (key.dwControlKeyState & SHIFT_PRESSED) != 0;
        event.control = (key.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
        event.alt = (key.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
        switch (key.wVirtualKeyCode) {
            case VK_ESCAPE: event.key = Key::escape; break;
            case VK_RETURN: event.key = Key::enter; break;
            case VK_TAB: event.key = Key::tab; break;
            case VK_BACK: event.key = Key::backspace; break;
            case VK_DELETE: event.key = Key::delete_key; break;
            case VK_LEFT: event.key = Key::left; break;
            case VK_RIGHT: event.key = Key::right; break;
            case VK_UP: event.key = Key::up; break;
            case VK_DOWN: event.key = Key::down; break;
            case VK_HOME: event.key = Key::home; break;
            case VK_END: event.key = Key::end; break;
            case VK_PRIOR: event.key = Key::page_up; break;
            case VK_NEXT: event.key = Key::page_down; break;
            default:
                if (event.control && key.wVirtualKeyCode >= 'A' && key.wVirtualKeyCode <= 'Z') {
                    event.key = Key::text;
                    event.character = static_cast<char>('a' + key.wVirtualKeyCode - 'A');
                } else if (key.uChar.UnicodeChar >= 32 && key.uChar.UnicodeChar <= 126) {
                    event.key = Key::text;
                    event.character = static_cast<char>(key.uChar.UnicodeChar);
                }
        }
        state_->repeated_key = event;
        state_->repetitions = key.wRepeatCount ? key.wRepeatCount - 1 : 0;
        return event;
    }
#else
    const auto read_byte = [](int milliseconds) -> std::optional<char> {
        pollfd descriptor{STDIN_FILENO, POLLIN, 0};
        int ready;
        do {
            ready = poll(&descriptor, 1, milliseconds);
        } while (ready < 0 && errno == EINTR);
        if (ready < 0) terminal_error("Waiting for terminal input");
        if (!ready) return {};
        if (!(descriptor.revents & POLLIN)) throw std::runtime_error("Terminal input disconnected");
        char character = 0;
        ssize_t count;
        do {
            count = ::read(STDIN_FILENO, &character, 1);
        } while (count < 0 && errno == EINTR);
        if (count < 0) terminal_error("Reading terminal input");
        if (!count) throw std::runtime_error("Terminal input ended");
        return character;
    };
    const auto first = read_byte(static_cast<int>(timeout.count()));
    if (!first) return {};
    std::string sequence(1, *first);
    if (*first == '\x1b') {
        const auto second = read_byte(30);
        if (second) {
            sequence += *second;
            if (*second == '[' || *second == 'O') {
                while (sequence.size() < 32) {
                    const auto next = read_byte(30);
                    if (!next) break;
                    sequence += *next;
                    if (*next >= '@' && *next <= '~') break;
                }
            }
        }
    }
    return decode_key(sequence);
#endif
}

DocumentEditor::DocumentEditor(const std::string& text) {
    set_text(text);
}

void DocumentEditor::set_text(const std::string& text) {
    std::vector<std::string> lines(1);
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char character = text[i];
        if (character == '\r' && i + 1 < text.size() && text[i + 1] == '\n') continue;
        if (character == '\n') lines.emplace_back();
        else if (character == '\t') lines.back() += "    ";
        else {
            printable(std::string(1, character));
            lines.back() += character;
        }
    }
    lines_ = std::move(lines);
    cursor_ = {};
    top_ = left_ = 0;
    preferred_column_.reset();
    modified_ = false;
}

std::string DocumentEditor::text() const {
    std::string result = lines_.front();
    for (std::size_t i = 1; i < lines_.size(); ++i) result += "\n" + lines_[i];
    return result;
}

void DocumentEditor::load(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open document: " + path.string());
    const std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (input.bad()) throw std::runtime_error("Cannot read document: " + path.string());
    set_text(content);
}

void DocumentEditor::save(const std::filesystem::path& path) {
    const auto content = text();
    if (content.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        throw std::length_error("Document exceeds the supported file size");
    }
    std::ofstream output_file(path, std::ios::binary | std::ios::trunc);
    if (!output_file) throw std::runtime_error("Cannot open document for writing: " + path.string());
    output_file.write(content.data(), static_cast<std::streamsize>(content.size()));
    output_file.close();
    if (!output_file) throw std::runtime_error("Cannot save document: " + path.string());
    modified_ = false;
}

bool DocumentEditor::handle(const KeyEvent& event) {
    if (event.control || event.alt) return false;
    const bool vertical = event.key == Key::up || event.key == Key::down ||
        event.key == Key::page_up || event.key == Key::page_down;
    if (!vertical) preferred_column_.reset();
    if (vertical) {
        if (!preferred_column_) preferred_column_ = cursor_.column;
        const auto amount = event.key == Key::page_up || event.key == Key::page_down ? page_rows_ : 1;
        if (event.key == Key::up || event.key == Key::page_up) {
            cursor_.row -= std::min(cursor_.row, amount);
        } else cursor_.row = down(cursor_.row, amount, lines_.size() - 1);
        cursor_.column = std::min(*preferred_column_, lines_[cursor_.row].size());
        return true;
    }
    switch (event.key) {
        case Key::left:
            if (cursor_.column) --cursor_.column;
            else if (cursor_.row) {
                --cursor_.row;
                cursor_.column = lines_[cursor_.row].size();
            }
            return true;
        case Key::right:
            if (cursor_.column < lines_[cursor_.row].size()) ++cursor_.column;
            else if (cursor_.row + 1 < lines_.size()) {
                ++cursor_.row;
                cursor_.column = 0;
            }
            return true;
        case Key::home: cursor_.column = 0; return true;
        case Key::end: cursor_.column = lines_[cursor_.row].size(); return true;
        case Key::text:
            printable(std::string(1, event.character));
            lines_[cursor_.row].insert(cursor_.column, 1, event.character);
            ++cursor_.column;
            modified_ = true;
            return true;
        case Key::tab:
            lines_[cursor_.row].insert(cursor_.column, "    ");
            cursor_.column += 4;
            modified_ = true;
            return true;
        case Key::enter: {
            const auto suffix = lines_[cursor_.row].substr(cursor_.column);
            lines_.insert(lines_.begin() + cursor_.row + 1, suffix);
            lines_[cursor_.row].resize(cursor_.column);
            ++cursor_.row;
            cursor_.column = 0;
            modified_ = true;
            return true;
        }
        case Key::backspace:
            if (cursor_.column) {
                lines_[cursor_.row].erase(--cursor_.column, 1);
                modified_ = true;
            } else if (cursor_.row) {
                const auto previous_length = lines_[cursor_.row - 1].size();
                lines_[cursor_.row - 1] += lines_[cursor_.row];
                lines_.erase(lines_.begin() + cursor_.row);
                --cursor_.row;
                cursor_.column = previous_length;
                modified_ = true;
            }
            return true;
        case Key::delete_key:
            if (cursor_.column < lines_[cursor_.row].size()) {
                lines_[cursor_.row].erase(cursor_.column, 1);
                modified_ = true;
            } else if (cursor_.row + 1 < lines_.size()) {
                lines_[cursor_.row] += lines_[cursor_.row + 1];
                lines_.erase(lines_.begin() + cursor_.row + 1);
                modified_ = true;
            }
            return true;
        default: return false;
    }
}

void DocumentEditor::render(Frame& frame) {
    frame.clear();
    const auto rows = frame.height() - 1;
    page_rows_ = std::max<std::size_t>(rows, 1);
    visible(cursor_.row, rows, top_);
    visible(cursor_.column, frame.width(), left_);
    for (std::size_t y = 0; y < rows && top_ + y < lines_.size(); ++y) {
        const auto& line = lines_[top_ + y];
        if (left_ < line.size()) frame.text(0, y, line.substr(left_));
    }
    if (rows && cursor_.row >= top_ && cursor_.row - top_ < rows) {
        const auto& line = lines_[cursor_.row];
        frame.put(cursor_.column - left_, cursor_.row - top_,
                  cursor_.column < line.size() ? line[cursor_.column] : ' ', Style::cursor);
    }
    frame.status_bar(std::string(modified_ ? "* " : "") + "Ln " +
        std::to_string(cursor_.row + 1) + " Col " + std::to_string(cursor_.column + 1));
}

TableViewer::TableViewer(std::vector<std::string> headers,
                         std::vector<std::vector<std::string>> rows)
    : headers_(std::move(headers)), rows_(std::move(rows)) {
    if (headers_.empty()) throw std::invalid_argument("Table requires at least one column");
    for (const auto& header : headers_) {
        printable(header);
        widths_.push_back(std::min<std::size_t>(30, std::max<std::size_t>(3, header.size())));
    }
    for (const auto& row : rows_) {
        if (row.size() != headers_.size()) throw std::invalid_argument("Table rows must match column count");
        for (std::size_t column = 0; column < row.size(); ++column) {
            printable(row[column]);
            widths_[column] = std::min<std::size_t>(30, std::max(widths_[column], row[column].size()));
        }
    }
}

std::optional<TextPosition> TableViewer::active_cell() const {
    return rows_.empty() ? std::optional<TextPosition>{} : active_;
}

void TableViewer::clear_selection() {
    selected_.clear();
    anchor_.reset();
}

void TableViewer::set_selection_mode(SelectionMode mode) {
    if (mode_ != mode) {
        mode_ = mode;
        clear_selection();
    }
}

bool TableViewer::selected(std::size_t row, std::size_t column) const {
    if (row >= rows_.size() || column >= headers_.size()) {
        throw std::out_of_range("Table cell outside bounds");
    }
    return selected_.count({row, mode_ == SelectionMode::rows ? 0 : column}) != 0;
}

std::vector<TextPosition> TableViewer::selection() const {
    std::vector<TextPosition> result;
    for (const auto& cell : selected_) result.push_back({cell.first, cell.second});
    return result;
}

bool TableViewer::handle(const KeyEvent& event) {
    if (event.control || event.alt) return false;
    if (event.key == Key::escape) {
        clear_selection();
        return true;
    }
    if (rows_.empty()) return false;
    if (event.key == Key::text && event.character == ' ') {
        const auto cell = std::make_pair(active_.row, mode_ == SelectionMode::rows ? 0 : active_.column);
        if (!selected_.erase(cell)) selected_.insert(cell);
        anchor_.reset();
        return true;
    }
    const auto before = active_;
    switch (event.key) {
        case Key::up: active_.row -= std::min<std::size_t>(active_.row, 1); break;
        case Key::down: active_.row = down(active_.row, 1, rows_.size() - 1); break;
        case Key::left: active_.column -= std::min<std::size_t>(active_.column, 1); break;
        case Key::right: active_.column = down(active_.column, 1, headers_.size() - 1); break;
        case Key::home: active_.column = 0; break;
        case Key::end: active_.column = headers_.size() - 1; break;
        case Key::page_up: active_.row -= std::min(active_.row, page_rows_); break;
        case Key::page_down: active_.row = down(active_.row, page_rows_, rows_.size() - 1); break;
        default: return false;
    }
    if (event.shift) {
        if (!anchor_) anchor_ = before;
        selected_.clear();
        const auto first_column = mode_ == SelectionMode::rows ? 0 : std::min(anchor_->column, active_.column);
        const auto last_column = mode_ == SelectionMode::rows ? 0 : std::max(anchor_->column, active_.column);
        for (std::size_t row = std::min(anchor_->row, active_.row); row <= std::max(anchor_->row, active_.row); ++row) {
            for (std::size_t column = first_column; column <= last_column; ++column) {
                selected_.insert({row, column});
            }
        }
    } else anchor_.reset();
    return true;
}

void TableViewer::render(Frame& frame) {
    frame.clear();
    const auto visible_rows = frame.height() > 2 ? frame.height() - 2 : 0;
    page_rows_ = std::max<std::size_t>(visible_rows, 1);
    if (!rows_.empty()) visible(active_.row, visible_rows, top_);
    if (active_.column < first_column_) first_column_ = active_.column;
    for (;;) {
        std::size_t offset = 0;
        for (std::size_t column = first_column_; column < active_.column; ++column) {
            offset += widths_[column] + 1;
        }
        if (offset < frame.width() || first_column_ == active_.column) break;
        ++first_column_;
    }
    std::size_t x = 0;
    for (std::size_t column = first_column_; column < headers_.size() && x < frame.width(); ++column) {
        frame.text(x, 0, headers_[column].substr(0, widths_[column]), Style::heading);
        for (std::size_t y = 0; y < visible_rows && top_ + y < rows_.size(); ++y) {
            const auto row = top_ + y;
            const bool active = row == active_.row &&
                (mode_ == SelectionMode::rows || column == active_.column);
            const auto style = active ? Style::cursor : selected(row, column) ? Style::selected : Style::normal;
            auto content = rows_[row][column].substr(0, widths_[column]);
            content.resize(widths_[column], ' ');
            frame.text(x, y + 1, content, style);
        }
        x += widths_[column] + 1;
    }
    frame.status_bar(rows_.empty() ? "Empty table" :
        "Row " + std::to_string(active_.row + 1) + " Col " + std::to_string(active_.column + 1) +
        " Selected " + std::to_string(selected_.size()) +
        (mode_ == SelectionMode::rows ? " rows" : " cells"));
}

Menu::Menu(std::vector<std::string> items) : items_(std::move(items)) {
    for (const auto& item : items_) printable(item);
}

std::optional<std::size_t> Menu::active_item() const {
    return items_.empty() ? std::optional<std::size_t>{} : active_;
}

std::optional<std::size_t> Menu::handle(const KeyEvent& event) {
    if (items_.empty() || event.control || event.alt) return {};
    if (event.key == Key::up) active_ -= std::min<std::size_t>(active_, 1);
    else if (event.key == Key::down) active_ = down(active_, 1, items_.size() - 1);
    else if (event.key == Key::home) active_ = 0;
    else if (event.key == Key::end) active_ = items_.size() - 1;
    else if (event.key == Key::enter) return active_;
    return {};
}

void Menu::render(Frame& frame) {
    frame.clear();
    visible(active_, frame.height(), top_);
    for (std::size_t y = 0; y < frame.height() && top_ + y < items_.size(); ++y) {
        frame.text(0, y, items_[top_ + y], top_ + y == active_ ? Style::cursor : Style::normal);
    }
}

} // namespace tui
} // namespace cli
} // namespace os_generics
