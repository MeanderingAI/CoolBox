#include "credit_card_statement.h"
#include "full_application_window.hpp"
#include "ocr_engine.h"
#include "os_dialog.hpp"
#include "pdf_reader.h"
#include "xlsx_writer.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Button {
    int x0;
    int y0;
    int x1;
    int y1;
    std::string label;
};

bool point_in_button(const Button& button, int x, int y) {
    return x >= button.x0 && x <= button.x1 && y >= button.y0 && y <= button.y1;
}

struct AppState {
    std::string pdf_path;
    std::string status_message = "Open a PDF statement to preview its transactions.";
    std::vector<docs::Transaction> transactions;
    double total = 0.0;
    bool has_data = false;
};

std::string format_amount(double value) {
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(2);
    out << value;
    return out.str();
}

void load_pdf(AppState& state, const std::string& path) {
    state.pdf_path = path;
    state.transactions.clear();
    state.total = 0.0;
    state.has_data = false;

    std::string text;
    try {
        text = docs::extract_pdf_text(path);
    } catch (const std::exception& ex) {
        state.status_message = std::string("Failed to read PDF: ") + ex.what();
        return;
    }

    // No embedded text layer usually means a scanned page; try the pluggable OCR backend.
    if (text.find_first_not_of(" \t\r\n") == std::string::npos) {
        try {
            auto ocr = docs::default_ocr_engine();
            text = ocr->recognize_text({});
        } catch (const std::exception& ex) {
            state.status_message = std::string("No text layer found: ") + ex.what();
            return;
        }
    }

    state.transactions = docs::parse_transactions(text);
    for (const auto& tx : state.transactions) {
        state.total += tx.amount;
    }
    state.has_data = true;
    state.status_message = "Loaded " + std::to_string(state.transactions.size()) +
                           " transaction(s) from " + std::filesystem::path(path).filename().string();
}

void save_xlsx(AppState& state, const std::string& path) {
    docs::XlsxWorkbook workbook;
    workbook.set_header({"Date", "Description", "Amount"});
    for (const auto& tx : state.transactions) {
        workbook.add_row({
            docs::XlsxCell::text_value(tx.date),
            docs::XlsxCell::text_value(tx.description),
            docs::XlsxCell::number_value(tx.amount),
        });
    }
    workbook.add_row({
        docs::XlsxCell::text_value(""),
        docs::XlsxCell::text_value("Total"),
        docs::XlsxCell::number_value(state.total),
    });

    if (workbook.save(path)) {
        state.status_message = "Saved " + std::filesystem::path(path).filename().string();
    } else {
        state.status_message = "Failed to save " + path;
    }
}

std::string pick_pdf_path() {
    app_builder::os_generics::OsDialog dialog;
    app_builder::os_generics::DialogRequest request;
    request.action = app_builder::os_generics::DialogAction::OpenFile;
    request.title = "Select credit card bill PDF";
    request.initial_path = std::filesystem::current_path().string();
    request.allow_native_ui = true;
    request.filters = {
        {"PDF Files", {"*.pdf"}},
        {"All Files", {"*.*"}}
    };

    const auto result = dialog.show(request);
    return (result.accepted && !result.selected_paths.empty()) ? result.selected_paths.front() : std::string();
}

std::string pick_xlsx_save_path(const std::string& pdf_path) {
    app_builder::os_generics::OsDialog dialog;
    app_builder::os_generics::DialogRequest request;
    request.action = app_builder::os_generics::DialogAction::SaveFile;
    request.title = "Save transactions as .xlsx";
    request.initial_path = std::filesystem::path(pdf_path).parent_path().string();
    request.suggested_name = std::filesystem::path(pdf_path).stem().string() + ".xlsx";
    request.allow_native_ui = true;
    request.filters = {
        {"Excel Workbook", {"*.xlsx"}},
        {"All Files", {"*.*"}}
    };

    const auto result = dialog.show(request);
    return (result.accepted && !result.selected_paths.empty()) ? result.selected_paths.front() : std::string();
}

} // namespace

int main() {
    using namespace graphics::full_application_window;

    AppState state;

    WindowConfig config("pdf_cell - PDF to Excel preview", 900, 640, true, true);
    FullApplicationWindow window(config);

    const Button open_button{16, 16, 176, 44, "Open PDF..."};
    const Button save_button{192, 16, 372, 44, "Save as .xlsx..."};
    bool prev_left = false;
    bool needs_redraw = true;

    RenderHooks hooks;
    hooks.on_render = [&](const RenderEvent&) {
        int w = 0;
        int h = 0;
        if (!window.client_size(w, h)) {
            return;
        }

        window.clear_background(22, 26, 34);

        const auto draw_button = [&](const Button& button, bool enabled) {
            window.fill_rect(button.x0, button.y0, button.x1, button.y1,
                             enabled ? 70 : 45, enabled ? 108 : 52, enabled ? 158 : 66);
            window.draw_text_line(button.x0 + 10, button.y0 + 8, button.label);
        };
        draw_button(open_button, true);
        draw_button(save_button, state.has_data);

        window.draw_text_line(16, 56, state.pdf_path.empty() ? "No file selected." : ("File: " + state.pdf_path));

        int y = 84;
        window.fill_rect(10, y, w - 10, y + 2, 56, 68, 86);
        y += 12;
        window.draw_text_line(16, y, "Date");
        window.draw_text_line(120, y, "Description");
        window.draw_text_line(w - 130, y, "Amount");
        y += 20;
        window.fill_rect(10, y, w - 10, y + 1, 56, 68, 86);
        y += 8;

        const int row_height = 18;
        const int max_rows = std::max(0, (h - y - 60) / row_height);
        const std::size_t shown = std::min(state.transactions.size(), static_cast<std::size_t>(max_rows));
        for (std::size_t i = 0; i < shown; ++i) {
            const auto& tx = state.transactions[i];
            window.draw_text_line(16, y, tx.date);
            std::string desc = tx.description;
            if (desc.size() > 40) {
                desc = desc.substr(0, 37) + "...";
            }
            window.draw_text_line(120, y, desc);
            window.draw_text_line(w - 130, y, format_amount(tx.amount));
            y += row_height;
        }
        if (state.transactions.size() > shown) {
            window.draw_text_line(16, y, "... and " + std::to_string(state.transactions.size() - shown) + " more");
            y += row_height;
        }

        window.fill_rect(10, h - 48, w - 10, h - 46, 56, 68, 86);
        window.draw_text_line(120, h - 40, "Total");
        window.draw_text_line(w - 130, h - 40, format_amount(state.total));

        window.draw_text_line(16, h - 20, state.status_message);
    };

    window.set_render_hooks(std::move(hooks));
    if (!window.create()) {
        std::cerr << "Failed to create pdf_cell_studio window.\n";
        return 1;
    }

    window.show();
    window.request_redraw();
    while (window.is_open()) {
        if (!window.pump_events()) {
            break;
        }

        PointerState pointer{};
        if (window.query_pointer_state(pointer)) {
            const bool left = pointer.left_button_down;
            const bool click = left && !prev_left;
            if (click && pointer.inside) {
                if (point_in_button(open_button, pointer.x, pointer.y)) {
                    const std::string path = pick_pdf_path();
                    if (!path.empty()) {
                        load_pdf(state, path);
                        needs_redraw = true;
                    }
                } else if (state.has_data && point_in_button(save_button, pointer.x, pointer.y)) {
                    const std::string path = pick_xlsx_save_path(state.pdf_path);
                    if (!path.empty()) {
                        save_xlsx(state, path);
                        needs_redraw = true;
                    }
                }
            }
            prev_left = left;
        }

        if (needs_redraw) {
            window.request_redraw();
            needs_redraw = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
