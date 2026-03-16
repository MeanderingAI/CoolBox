#include <gtest/gtest.h>

#include "components.hpp"

TEST(GraphicsComponentsTest, RendersBasicWidgets) {
    const auto button = graphics::components::Component::button("Run", true, false, 10).render();
    const auto radio = graphics::components::Component::radio_button("Option A", true).render();
    const auto checkbox = graphics::components::Component::check_box("Enable Logs", true).render();

    ASSERT_EQ(button.size(), 1U);
    EXPECT_NE(button.front().find("Run"), std::string::npos);
    EXPECT_EQ(radio.front(), "(o) Option A");
    EXPECT_EQ(checkbox.front(), "[x] Enable Logs");
}

TEST(GraphicsComponentsTest, RendersTextAndEditableViews) {
    const auto text_lines = graphics::components::Component::text_view(
        "A text view should wrap descriptive content for panels.", 18).render();
    const auto editable_lines = graphics::components::Component::editable_text_view("Hello", 5, true, 12).render();

    EXPECT_GE(text_lines.size(), 2U);
    EXPECT_NE(editable_lines.front().find("Hello|"), std::string::npos);
}

TEST(GraphicsComponentsTest, RendersVerticalHorizontalAndGridHolders) {
    const auto vertical = graphics::components::ComponentHolder::vertical({
        graphics::components::Component::button("One"),
        graphics::components::Component::check_box("Two", true)
    });
    const auto horizontal = graphics::components::ComponentHolder::horizontal({
        graphics::components::Component::radio_button("Left", true),
        graphics::components::Component::radio_button("Right", false)
    }, 2);
    const auto grid = graphics::components::ComponentHolder::grid({
        graphics::components::Component::button("A"),
        graphics::components::Component::button("B"),
        graphics::components::Component::button("C")
    }, 2, 2, 1);

    const auto vertical_lines = vertical.render();
    const auto horizontal_lines = horizontal.render();
    const auto grid_lines = grid.render();

    EXPECT_GE(vertical_lines.size(), 3U);
    EXPECT_NE(vertical_lines.front().find("One"), std::string::npos);
    EXPECT_NE(vertical_lines.back().find("Two"), std::string::npos);

    ASSERT_FALSE(horizontal_lines.empty());
    EXPECT_NE(horizontal_lines.front().find("Left"), std::string::npos);
    EXPECT_NE(horizontal_lines.front().find("Right"), std::string::npos);

    EXPECT_GE(grid_lines.size(), 3U);
    EXPECT_NE(grid_lines.front().find("A"), std::string::npos);
    EXPECT_NE(grid_lines.front().find("B"), std::string::npos);
    EXPECT_NE(grid_lines.back().find("C"), std::string::npos);
}

TEST(GraphicsComponentsTest, RendersLayoutGroupAsComponentType) {
    const auto nested_holder = graphics::components::ComponentHolder::horizontal({
        graphics::components::Component::button("Yes"),
        graphics::components::Component::button("No")
    }, 2);

    const auto layout_group = graphics::components::Component::layout_group(nested_holder, "Decision Row");
    const auto rendered = layout_group.render();

    EXPECT_EQ(graphics::components::component_type_name(layout_group.type()), "layoutGroup");
    ASSERT_GE(rendered.size(), 2U);
    EXPECT_NE(rendered.front().find("Decision Row"), std::string::npos);
    EXPECT_NE(rendered.back().find("Yes"), std::string::npos);
    EXPECT_NE(rendered.back().find("No"), std::string::npos);
}
