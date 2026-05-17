#include "tyst_framework.hpp"

#include "xml_screen_descriptor_parser.hpp"

TEST(XmlScreenDescriptorParserTest, ParsesScreenMetadataAndNestedLayout) {
    const std::string xml = R"XML(
<screen title="CAD Studio" width="1280" height="720" platform="linux">
  <grid columns="2" hspacing="2" vspacing="1">
    <button label="Open" width="12" />
    <editable_text text="Sketch001" cursor="6" focused="true" width="20" />
    <checkbox label="Snap" checked="true" />
    <layout label="Toolbox" layout="vertical">
      <radio label="Arc" selected="true" />
      <radio label="Rectangle" selected="false" />
    </layout>
    <menu_bar>
      <menu title="File">
        <item label="Open" shortcut="Ctrl+O" />
        <item separator="true" />
        <item label="Exit" />
      </menu>
    </menu_bar>
  </grid>
</screen>
)XML";

    os_generics::xml_screen_descriptor::XmlScreenDescriptorParser parser;
    const auto parsed = parser.parse_string(xml);

    ASSERT_TRUE(parsed.ok());
    EXPECT_EQ(parsed.descriptor.title, "CAD Studio");
    EXPECT_EQ(parsed.descriptor.width, static_cast<std::size_t>(1280));
    EXPECT_EQ(parsed.descriptor.height, static_cast<std::size_t>(720));
    EXPECT_EQ(parsed.descriptor.layout.layout(), graphics::components::LayoutType::Grid);

    const auto& components = parsed.descriptor.layout.components();
    ASSERT_EQ(components.size(), static_cast<std::size_t>(5));
    EXPECT_EQ(components[0].type(), graphics::components::ComponentType::Button);
    EXPECT_EQ(components[1].type(), graphics::components::ComponentType::EditableTextView);
    EXPECT_EQ(components[2].type(), graphics::components::ComponentType::CheckBox);
    EXPECT_EQ(components[3].type(), graphics::components::ComponentType::LayoutGroup);
    EXPECT_EQ(components[4].type(), graphics::components::ComponentType::MenuBar);
}

TEST(XmlScreenDescriptorParserTest, ReportsMalformedXml) {
    const std::string xml = R"XML(
<screen title="Broken">
  <button label="Open">
</screen>
)XML";

    os_generics::xml_screen_descriptor::XmlScreenDescriptorParser parser;
    const auto parsed = parser.parse_string(xml);

    ASSERT_FALSE(parsed.ok());
    ASSERT_FALSE(parsed.errors.empty());
}

TEST(XmlScreenDescriptorParserTest, ReportsMissingScreenRoot) {
    const std::string xml = R"XML(
<layout>
  <button label="Open" />
</layout>
)XML";

    os_generics::xml_screen_descriptor::XmlScreenDescriptorParser parser;
    const auto parsed = parser.parse_string(xml);

    ASSERT_FALSE(parsed.ok());
    ASSERT_FALSE(parsed.errors.empty());
    EXPECT_NE(parsed.errors.front().find("Missing <screen>"), std::string::npos);
}

TEST(XmlScreenDescriptorParserTest, ParsesTabbedViewAndEditableOptions) {
    const std::string xml = R"XML(
<screen title="Builder" width="1024" height="700" platform="windows">
  <vertical>
    <tabbed_view selected="1">
      <tab title="Design">
        <vertical>
          <button label="Draw" />
        </vertical>
      </tab>
      <tab title="Inspect">
        <editable_text_view text="alpha beta gamma delta"
                            cursor="5"
                            focused="true"
                            width="16"
                            wrap="true"
                            resizable="true"
                            max-height="3"
                            font-size="18"
                            autocomplete="alpha,beta,gamma" />
      </tab>
    </tabbed_view>
  </vertical>
</screen>
)XML";

    os_generics::xml_screen_descriptor::XmlScreenDescriptorParser parser;
    const auto parsed = parser.parse_string(xml);

    ASSERT_TRUE(parsed.ok());
    const auto& root_components = parsed.descriptor.layout.components();
    ASSERT_EQ(root_components.size(), static_cast<std::size_t>(1));
    EXPECT_EQ(root_components[0].type(), graphics::components::ComponentType::TabbedView);

    const auto* tabbed = root_components[0].tabbed_view_model();
    ASSERT_NE(tabbed, nullptr);
    ASSERT_EQ(tabbed->tabs.size(), static_cast<std::size_t>(2));
    EXPECT_EQ(tabbed->selected, static_cast<std::size_t>(1));
    EXPECT_EQ(tabbed->tabs[1].title, "Inspect");

    const auto& inspect_components = tabbed->tabs[1].subview.components();
    ASSERT_EQ(inspect_components.size(), static_cast<std::size_t>(1));
    const auto& editable = inspect_components[0];
    EXPECT_EQ(editable.type(), graphics::components::ComponentType::EditableTextView);
    EXPECT_EQ(editable.wrap_text(), true);
    EXPECT_EQ(editable.resizable(), true);
    EXPECT_EQ(editable.max_height(), static_cast<std::size_t>(3));
    EXPECT_EQ(editable.font_size(), static_cast<std::size_t>(18));
    ASSERT_EQ(editable.autocomplete_suggestions().size(), static_cast<std::size_t>(3));
    EXPECT_EQ(editable.autocomplete_suggestions()[0], "alpha");
}

TEST(XmlScreenDescriptorParserTest, ParsesTextBordersAndLineNumbers) {
    const std::string xml = R"XML(
<screen title="Borders" width="900" height="600" platform="windows">
  <vertical>
    <text_view text="one two three four five six"
               width="10"
               wrap="true"
               border-style="double"
               border-thickness="1"
               line-numbers="true" />
    <editable_text_view text="alpha beta gamma"
                        cursor="4"
                        width="12"
                        wrap="true"
                        border-style="dashed"
                        border-thickness="2"
                        line-numbers="true" />
  </vertical>
</screen>
)XML";

    os_generics::xml_screen_descriptor::XmlScreenDescriptorParser parser;
    const auto parsed = parser.parse_string(xml);

    ASSERT_TRUE(parsed.ok());
    const auto& components = parsed.descriptor.layout.components();
    ASSERT_EQ(components.size(), static_cast<std::size_t>(2));
    EXPECT_EQ(components[0].border_style(), graphics::components::BorderStyle::Double);
    EXPECT_EQ(components[0].border_thickness(), static_cast<std::size_t>(1));
    EXPECT_EQ(components[0].show_line_numbers(), true);

    EXPECT_EQ(components[1].border_style(), graphics::components::BorderStyle::Dashed);
    EXPECT_EQ(components[1].border_thickness(), static_cast<std::size_t>(2));
    EXPECT_EQ(components[1].show_line_numbers(), true);
}
