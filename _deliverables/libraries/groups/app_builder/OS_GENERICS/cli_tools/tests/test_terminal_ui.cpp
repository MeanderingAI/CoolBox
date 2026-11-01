#include "terminal_ui.hpp"
#include "tyst_framework.hpp"

#include <fstream>
#include <limits>
#include <stdexcept>

using namespace os_generics::cli::tui;

TEST(TerminalInput, CharactersControlAndNavigation) {
    EXPECT_EQ(decode_key("a").key, Key::text);
    EXPECT_EQ(decode_key("a").character, 'a');
    EXPECT_EQ(decode_key("\r").key, Key::enter);
    EXPECT_EQ(decode_key("\t").key, Key::tab);
    EXPECT_EQ(decode_key("\x7f").key, Key::backspace);
    EXPECT_EQ(decode_key("\x1b").key, Key::escape);
    EXPECT_TRUE(decode_key("\x13").control);
    EXPECT_EQ(decode_key("\x13").character, 's');
    EXPECT_EQ(decode_key("\x1b[A").key, Key::up);
    EXPECT_EQ(decode_key("\x1bOB").key, Key::down);
    EXPECT_EQ(decode_key("\x1b[C").key, Key::right);
    EXPECT_EQ(decode_key("\x1b[D").key, Key::left);
    EXPECT_EQ(decode_key("\x1b[3~").key, Key::delete_key);
    EXPECT_EQ(decode_key("\x1b[5~").key, Key::page_up);
    EXPECT_EQ(decode_key("\x1b[6~").key, Key::page_down);
    EXPECT_EQ(decode_key("\x1b[H").key, Key::home);
    EXPECT_EQ(decode_key("\x1b[F").key, Key::end);
    EXPECT_THROW(decode_key(""), std::invalid_argument);
    EXPECT_EQ(decode_key("abc").key, Key::unknown);
}

TEST(TerminalInput, XtermModifiersAndUnsupportedKeys) {
    const auto shift = decode_key("\x1b[1;2C");
    EXPECT_EQ(shift.key, Key::right);
    EXPECT_TRUE(shift.shift);
    EXPECT_FALSE(shift.control);
    const auto all = decode_key("\x1b[1;8D");
    EXPECT_TRUE(all.shift);
    EXPECT_TRUE(all.control);
    EXPECT_TRUE(all.alt);
    EXPECT_TRUE(decode_key("\x1b" "x").alt);
    EXPECT_TRUE(decode_key("\x1b[Z").shift);
    EXPECT_EQ(decode_key("\x1b[1;99A").key, Key::unknown);
    EXPECT_EQ(decode_key("\x1b[").key, Key::unknown);
    EXPECT_EQ(decode_key(std::string(1, static_cast<char>(0xff))).key, Key::unknown);
}

TEST(TerminalFrame, ClippingStylesAndEscapeSafety) {
    Frame frame(5, 3);
    frame.text(3, 0, "abcd", Style::heading);
    frame.put(0, 1, 'x', Style::selected);
    frame.status_bar("ready!");
    EXPECT_EQ(frame.plain_text(), "   ab\nx    \nready");
    EXPECT_NE(frame.ansi().find("\x1b[0;30;46m"), std::string::npos);
    EXPECT_THROW(frame.text(0, 0, "\x1b[2J"), std::invalid_argument);
    EXPECT_THROW(frame.put(0, 0, '\n'), std::invalid_argument);
    EXPECT_THROW((Frame(0, 1)), std::invalid_argument);
    EXPECT_THROW((Frame(std::numeric_limits<std::size_t>::max(), 2)), std::invalid_argument);
    frame.clear();
    EXPECT_EQ(frame.plain_text(), "     \n     \n     ");
}

TEST(TerminalEditor, MultilineInsertionSplitAndJoin) {
    DocumentEditor editor("abc");
    editor.handle({Key::right});
    editor.handle({Key::enter});
    EXPECT_EQ(editor.text(), "a\nbc");
    editor.handle({Key::text, 'X'});
    EXPECT_EQ(editor.text(), "a\nXbc");
    editor.handle({Key::backspace});
    editor.handle({Key::backspace});
    EXPECT_EQ(editor.text(), "abc");
    EXPECT_EQ(editor.cursor().column, 1U);
    editor.handle({Key::delete_key});
    EXPECT_EQ(editor.text(), "ac");
    editor.handle({Key::enter});
    editor.handle({Key::up});
    editor.handle({Key::end});
    editor.handle({Key::delete_key});
    EXPECT_EQ(editor.text(), "ac");
    EXPECT_TRUE(editor.modified());
}

TEST(TerminalEditor, VerticalMovementKeepsPreferredColumn) {
    DocumentEditor editor("abcdef\nx\n123456");
    editor.handle({Key::end});
    editor.handle({Key::down});
    EXPECT_EQ(editor.cursor().column, 1U);
    editor.handle({Key::down});
    EXPECT_EQ(editor.cursor().column, 6U);
    editor.handle({Key::up});
    editor.handle({Key::home});
    editor.handle({Key::down});
    EXPECT_EQ(editor.cursor().column, 0U);
    EXPECT_FALSE(editor.handle({Key::text, 's', false, true}));
}

TEST(TerminalEditor, NormalizationValidationAndBoundaries) {
    DocumentEditor editor("a\r\n\tb\n");
    EXPECT_EQ(editor.text(), "a\n    b\n");
    EXPECT_FALSE(editor.modified());
    EXPECT_THROW(editor.set_text("\x1b"), std::invalid_argument);
    EXPECT_EQ(editor.text(), "a\n    b\n");
    EXPECT_THROW(editor.set_text("caf\xc3\xa9"), std::invalid_argument);
    editor.set_text("");
    editor.handle({Key::backspace});
    editor.handle({Key::delete_key});
    EXPECT_FALSE(editor.modified());
    editor.handle({Key::tab});
    EXPECT_EQ(editor.text(), "    ");
    EXPECT_EQ(editor.cursor().column, 4U);
}

TEST(TerminalEditor, ScrollAndRenderTinyFrames) {
    DocumentEditor editor("longlongline\nsecond\nthird");
    Frame frame(4, 3);
    editor.handle({Key::end});
    editor.render(frame);
    EXPECT_NE(frame.plain_text().find("ine "), std::string::npos);
    editor.handle({Key::page_down});
    editor.render(frame);
    EXPECT_EQ(editor.cursor().row, 2U);
    EXPECT_TRUE(frame.ansi().find("\x1b[0;7m") != std::string::npos);
    Frame tiny(1, 1);
    EXPECT_NO_THROW(editor.render(tiny));
    editor.set_text("");
    editor.render(frame);
    EXPECT_EQ(frame.plain_text().substr(0, 4), "    ");
}

TEST(TerminalEditor, SaveLoadAndFileErrors) {
    const auto directory = std::filesystem::temp_directory_path() /
        ("coolbox-tui-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    ASSERT_TRUE(std::filesystem::create_directory(directory));
    struct Cleanup {
        std::filesystem::path directory;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove(directory / "document.txt", error);
            if (error) std::cerr << error.message() << '\n';
            std::filesystem::remove(directory, error);
            if (error) std::cerr << error.message() << '\n';
        }
    } cleanup{directory};
    DocumentEditor editor("first\nlast\n");
    editor.handle({Key::text, 'X'});
    EXPECT_TRUE(editor.modified());
    editor.save(directory / "document.txt");
    EXPECT_FALSE(editor.modified());
    DocumentEditor restored;
    restored.load(directory / "document.txt");
    EXPECT_EQ(restored.text(), editor.text());
    EXPECT_THROW(restored.load(directory / "missing.txt"), std::runtime_error);
    EXPECT_EQ(restored.text(), editor.text());
    editor.handle({Key::text, 'Y'});
    EXPECT_THROW(editor.save(directory / "missing" / "doc.txt"), std::runtime_error);
    EXPECT_TRUE(editor.modified());
}

TEST(TerminalTable, NavigationAndCellToggling) {
    TableViewer table({"A", "B"}, {{"1", "2"}, {"3", "4"}});
    ASSERT_TRUE(table.active_cell().has_value());
    table.handle({Key::right});
    table.handle({Key::down});
    table.handle({Key::text, ' '});
    EXPECT_TRUE(table.selected(1, 1));
    EXPECT_FALSE(table.selected(1, 0));
    EXPECT_EQ(table.selection().size(), 1U);
    table.handle({Key::text, ' '});
    EXPECT_TRUE(table.selection().empty());
    table.handle({Key::down});
    table.handle({Key::right});
    EXPECT_EQ(table.active_cell()->row, 1U);
    EXPECT_EQ(table.active_cell()->column, 1U);
    EXPECT_THROW(table.selected(2, 0), std::out_of_range);
}

TEST(TerminalTable, ShiftSelectsRectanglesAndShrinksThem) {
    TableViewer table({"A", "B", "C"}, {{"a", "b", "c"}, {"d", "e", "f"}, {"g", "h", "i"}});
    table.handle({Key::right, 0, true});
    table.handle({Key::down, 0, true});
    EXPECT_EQ(table.selection().size(), 4U);
    EXPECT_TRUE(table.selected(0, 0));
    EXPECT_TRUE(table.selected(1, 1));
    table.handle({Key::up, 0, true});
    EXPECT_EQ(table.selection().size(), 2U);
    table.handle({Key::escape});
    EXPECT_TRUE(table.selection().empty());
    table.handle({Key::down, 0, true});
    EXPECT_TRUE(table.selected(0, 1));
    EXPECT_TRUE(table.selected(1, 1));
}

TEST(TerminalTable, RowModeAndSelectionReset) {
    TableViewer table({"A", "B"}, {{"1", "2"}, {"3", "4"}, {"5", "6"}});
    table.set_selection_mode(SelectionMode::rows);
    table.handle({Key::right});
    table.handle({Key::text, ' '});
    EXPECT_TRUE(table.selected(0, 0));
    EXPECT_TRUE(table.selected(0, 1));
    EXPECT_EQ(table.selection()[0].column, 0U);
    table.handle({Key::down, 0, true});
    EXPECT_EQ(table.selection().size(), 2U);
    table.set_selection_mode(SelectionMode::cells);
    EXPECT_TRUE(table.selection().empty());
}

TEST(TerminalTable, EmptyAndMalformedData) {
    TableViewer empty({"A"}, {});
    EXPECT_FALSE(empty.active_cell().has_value());
    EXPECT_FALSE(empty.handle({Key::down}));
    Frame frame(10, 3);
    empty.render(frame);
    EXPECT_NE(frame.plain_text().find("Empty tabl"), std::string::npos);
    EXPECT_THROW((TableViewer({}, {})), std::invalid_argument);
    EXPECT_THROW((TableViewer({"A", "B"}, {{"1"}})), std::invalid_argument);
    EXPECT_THROW((TableViewer({"A"}, {{"\x1b[2J"}})), std::invalid_argument);
}

TEST(TerminalTable, VerticalAndHorizontalViewportTracking) {
    TableViewer table({"first", "second", "third"},
                      {{"A", "B", "C"}, {"D", "E", "F"}, {"G", "H", "I"}, {"J", "K", "L"}});
    Frame frame(5, 4);
    table.render(frame);
    table.handle({Key::end});
    table.handle({Key::page_down});
    table.render(frame);
    EXPECT_EQ(table.active_cell()->row, 2U);
    EXPECT_EQ(frame.plain_text().substr(0, 5), "third");
    EXPECT_NE(frame.plain_text().find("I"), std::string::npos);
    Frame tiny(1, 1);
    EXPECT_NO_THROW(table.render(tiny));
}

TEST(TerminalMenu, NavigateActivateAndScroll) {
    Menu menu({"Open", "Save", "Quit"});
    menu.handle({Key::down});
    EXPECT_EQ(menu.handle({Key::enter}).value(), 1U);
    menu.handle({Key::end});
    Frame frame(5, 2);
    menu.render(frame);
    EXPECT_NE(frame.plain_text().find("Quit"), std::string::npos);
    menu.handle({Key::home});
    EXPECT_EQ(menu.active_item().value(), 0U);
    Menu empty({});
    EXPECT_FALSE(empty.active_item().has_value());
    EXPECT_FALSE(empty.handle({Key::enter}).has_value());
    EXPECT_THROW((Menu({"\n"})), std::invalid_argument);
}
