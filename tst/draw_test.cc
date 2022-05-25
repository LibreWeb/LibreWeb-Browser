#include "draw.h"
#include "md-parser.h"
#include "mock-middleware.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <gtkmm/application.h>
#include <string>
namespace
{
  class DrawFixture : public testing::Test
  {
  protected:
    void SetUp() override
    {
      Gtk::Application::create();
    }
  };

  TEST_F(DrawFixture, TestDrawSetText)
  {
    // Given
    std::string text = "Hello world!";
    MockMiddleware middleware;
    Draw draw(middleware);

    // When
    draw.set_text(text);

    // Then
    ASSERT_EQ(draw.get_text(), text);
  }

  TEST_F(DrawFixture, TestDrawDocument)
  {
    // Given
    std::string markdown = "**Hello** *world*";
    cmark_node* doc = Parser::parse_content(markdown);

    MockMiddleware middleware;
    Draw draw(middleware);

    // When
    draw.set_document(doc);

    // Then
    std::string result = draw.get_text();
    ASSERT_EQ(result, "Hello world\n\n");
  }

  TEST_F(DrawFixture, TestDrawTextTags)
  {
    // Given
    std::string italic_tag_name = "italic";
    std::string bold_tag_name = "bold";
    std::string heading1_tag_name = "heading1";
    std::string heading2_tag_name = "heading2";
    std::string heading3_tag_name = "heading3";
    std::string heading4_tag_name = "heading4";
    std::string heading5_tag_name = "heading5";
    std::string heading6_tag_name = "heading6";
    std::string strikethrough_tag_name = "strikethrough";
    std::string superscript_tag_name = "superscript";
    std::string subscript_tag_name = "subscript";
    std::string code_tag_name = "code";
    std::string quote_tag_name = "quote";
    std::string highlight_tag_name = "highlight";
    MockMiddleware middleware;
    Draw draw(middleware);

    // When
    auto tagTable = draw.get_buffer()->get_tag_table();
    auto propery_italic = tagTable->lookup(italic_tag_name);
    auto propery_bold = tagTable->lookup(bold_tag_name);
    auto propery_heading1 = tagTable->lookup(heading1_tag_name);
    auto propery_heading2 = tagTable->lookup(heading2_tag_name);
    auto propery_heading3 = tagTable->lookup(heading3_tag_name);
    auto propery_heading4 = tagTable->lookup(heading4_tag_name);
    auto propery_heading5 = tagTable->lookup(heading5_tag_name);
    auto propery_heading6 = tagTable->lookup(heading6_tag_name);
    auto propery_strikethrough = tagTable->lookup(strikethrough_tag_name);
    auto propery_superscript = tagTable->lookup(superscript_tag_name);
    auto propery_subscript = tagTable->lookup(subscript_tag_name);
    auto propery_code = tagTable->lookup(code_tag_name);
    auto propery_quote = tagTable->lookup(quote_tag_name);
    auto propery_highlight = tagTable->lookup(highlight_tag_name);

    // Then
    ASSERT_NE(propery_italic.get(), nullptr);
    ASSERT_EQ(propery_italic->property_name().get_value(), italic_tag_name);
    ASSERT_NE(propery_bold.get(), nullptr);
    ASSERT_EQ(propery_bold->property_name().get_value(), bold_tag_name);
    ASSERT_NE(propery_heading1.get(), nullptr);
    ASSERT_EQ(propery_heading1->property_name().get_value(), heading1_tag_name);
    ASSERT_NE(propery_heading2.get(), nullptr);
    ASSERT_EQ(propery_heading2->property_name().get_value(), heading2_tag_name);
    ASSERT_NE(propery_heading3.get(), nullptr);
    ASSERT_EQ(propery_heading3->property_name().get_value(), heading3_tag_name);
    ASSERT_NE(propery_heading4.get(), nullptr);
    ASSERT_EQ(propery_heading4->property_name().get_value(), heading4_tag_name);
    ASSERT_NE(propery_heading5.get(), nullptr);
    ASSERT_EQ(propery_heading5->property_name().get_value(), heading5_tag_name);
    ASSERT_NE(propery_heading6.get(), nullptr);
    ASSERT_EQ(propery_heading6->property_name().get_value(), heading6_tag_name);
    ASSERT_NE(propery_strikethrough.get(), nullptr);
    ASSERT_EQ(propery_strikethrough->property_name().get_value(), strikethrough_tag_name);
    ASSERT_NE(propery_superscript.get(), nullptr);
    ASSERT_EQ(propery_superscript->property_name().get_value(), superscript_tag_name);
    ASSERT_NE(propery_subscript.get(), nullptr);
    ASSERT_EQ(propery_subscript->property_name().get_value(), subscript_tag_name);
    ASSERT_NE(propery_code.get(), nullptr);
    ASSERT_EQ(propery_code->property_name().get_value(), code_tag_name);
    ASSERT_NE(propery_quote.get(), nullptr);
    ASSERT_EQ(propery_quote->property_name().get_value(), quote_tag_name);
    ASSERT_NE(propery_highlight.get(), nullptr);
    ASSERT_EQ(propery_highlight->property_name().get_value(), highlight_tag_name);
  }

  TEST_F(DrawFixture, TestDrawTextAttributes)
  {
    // Given
    std::string markdown = "**bold**~~strikethrough~~^up^%down%`code`";
    gsize length = 0;
    cmark_node* doc = Parser::parse_content(markdown);
    MockMiddleware middleware;
    Draw draw(middleware);

    // When
    draw.set_document(doc);
    auto buffer = draw.get_buffer();
    // Using the built-in formatter
    guint8* data = buffer->serialize(buffer, "application/x-gtk-text-buffer-rich-text", buffer->begin(), buffer->end(), length);
    // Convert data to string
    std::string string_data(data, data + length);

    // Then
    // Bold attribute
    std::string expect_attr1 = "<attr name=\"weight\" type=\"gint\" value=\"700\" />";
    // Strikethrough attribute
    std::string expect_attr2 = "<attr name=\"strikethrough\" type=\"gboolean\" value=\"TRUE\" />";
    // Super-/subcript attribute
    std::string expect_attr3 =
        "<attr name=\"scale\" type=\"gdouble\" value="; // Depending on platform the value could be in UK or US format (so we removed the value)
    // Superscript attributes
    std::string expect_attr4 = "<attr name=\"rise\" type=\"gint\" value=\"-6144\" />";
    // Subscript attributes
    std::string expect_attr5 = "<attr name=\"rise\" type=\"gint\" value=\"6144\" />";
    // Code attributes
    std::string expect_attr6 = "<attr name=\"background-gdk\" type=\"GdkColor\" value=\"e0e0:e0e0:e0e0\" />";
    std::string expect_attr7 = "<attr name=\"foreground-gdk\" type=\"GdkColor\" value=\"3232:3232:3232\" />";
    std::string expect_attr8 = "<attr name=\"family\" type=\"gchararray\" value=\"monospace\" />";
    EXPECT_THAT(string_data, testing::HasSubstr(expect_attr1));
    EXPECT_THAT(string_data, testing::HasSubstr(expect_attr2));
    EXPECT_THAT(string_data, testing::HasSubstr(expect_attr3));
    EXPECT_THAT(string_data, testing::HasSubstr(expect_attr4));
    EXPECT_THAT(string_data, testing::HasSubstr(expect_attr5));
    EXPECT_THAT(string_data, testing::HasSubstr(expect_attr6));
    EXPECT_THAT(string_data, testing::HasSubstr(expect_attr7));
    EXPECT_THAT(string_data, testing::HasSubstr(expect_attr8));
  }
} // namespace
