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
    draw.setText(text);

    // Then
    ASSERT_EQ(draw.getText(), text);
  }

  TEST_F(DrawFixture, TestDrawDocument)
  {
    // Given
    std::string markdown = "**Hello** *world*";
    cmark_node* doc = Parser::parseContent(markdown);

    MockMiddleware middleware;
    Draw draw(middleware);

    // When
    draw.setDocument(doc);

    // Then
    std::string result = draw.getText();
    ASSERT_EQ(result, "Hello world\n\n");
  }

  TEST_F(DrawFixture, TestDrawTextTags)
  {
    // Given
    std::string italicTagName = "italic";
    std::string boldTagName = "bold";
    std::string heading1TagName = "heading1";
    std::string heading2TagName = "heading2";
    std::string heading3TagName = "heading3";
    std::string heading4TagName = "heading4";
    std::string heading5TagName = "heading5";
    std::string heading6TagName = "heading6";
    std::string strikethroughTagName = "strikethrough";
    std::string superscriptTagName = "superscript";
    std::string subscriptTagName = "subscript";
    std::string codeTagName = "code";
    std::string quoteTagName = "quote";
    std::string highlightTagName = "highlight";
    MockMiddleware middleware;
    Draw draw(middleware);

    // When
    auto tagTable = draw.get_buffer()->get_tag_table();
    auto properyItalic = tagTable->lookup(italicTagName);
    auto properyBold = tagTable->lookup(boldTagName);
    auto properyHeading1 = tagTable->lookup(heading1TagName);
    auto properyHeading2 = tagTable->lookup(heading2TagName);
    auto properyHeading3 = tagTable->lookup(heading3TagName);
    auto properyHeading4 = tagTable->lookup(heading4TagName);
    auto properyHeading5 = tagTable->lookup(heading5TagName);
    auto properyHeading6 = tagTable->lookup(heading6TagName);
    auto properyStrikethrough = tagTable->lookup(strikethroughTagName);
    auto properySuperscript = tagTable->lookup(superscriptTagName);
    auto properySubscript = tagTable->lookup(subscriptTagName);
    auto properyCode = tagTable->lookup(codeTagName);
    auto properyQuote = tagTable->lookup(quoteTagName);
    auto properyHighlight = tagTable->lookup(highlightTagName);

    // Then
    ASSERT_NE(properyItalic.get(), nullptr);
    ASSERT_EQ(properyItalic->property_name().get_value(), italicTagName);
    ASSERT_NE(properyBold.get(), nullptr);
    ASSERT_EQ(properyBold->property_name().get_value(), boldTagName);
    ASSERT_NE(properyHeading1.get(), nullptr);
    ASSERT_EQ(properyHeading1->property_name().get_value(), heading1TagName);
    ASSERT_NE(properyHeading2.get(), nullptr);
    ASSERT_EQ(properyHeading2->property_name().get_value(), heading2TagName);
    ASSERT_NE(properyHeading3.get(), nullptr);
    ASSERT_EQ(properyHeading3->property_name().get_value(), heading3TagName);
    ASSERT_NE(properyHeading4.get(), nullptr);
    ASSERT_EQ(properyHeading4->property_name().get_value(), heading4TagName);
    ASSERT_NE(properyHeading5.get(), nullptr);
    ASSERT_EQ(properyHeading5->property_name().get_value(), heading5TagName);
    ASSERT_NE(properyHeading6.get(), nullptr);
    ASSERT_EQ(properyHeading6->property_name().get_value(), heading6TagName);
    ASSERT_NE(properyStrikethrough.get(), nullptr);
    ASSERT_EQ(properyStrikethrough->property_name().get_value(), strikethroughTagName);
    ASSERT_NE(properySuperscript.get(), nullptr);
    ASSERT_EQ(properySuperscript->property_name().get_value(), superscriptTagName);
    ASSERT_NE(properySubscript.get(), nullptr);
    ASSERT_EQ(properySubscript->property_name().get_value(), subscriptTagName);
    ASSERT_NE(properyCode.get(), nullptr);
    ASSERT_EQ(properyCode->property_name().get_value(), codeTagName);
    ASSERT_NE(properyQuote.get(), nullptr);
    ASSERT_EQ(properyQuote->property_name().get_value(), quoteTagName);
    ASSERT_NE(properyHighlight.get(), nullptr);
    ASSERT_EQ(properyHighlight->property_name().get_value(), highlightTagName);
  }

  TEST_F(DrawFixture, TestDrawTextAttributes)
  {
    // Given
    std::string markdown = "**bold**~~strikethrough~~^up^%down%`code`";
    gsize length = 0;
    cmark_node* doc = Parser::parseContent(markdown);
    MockMiddleware middleware;
    Draw draw(middleware);

    // When
    draw.setDocument(doc);
    auto buffer = draw.get_buffer();
    // Using the built-in formatter
    guint8* data = buffer->serialize(buffer, "application/x-gtk-text-buffer-rich-text", buffer->begin(), buffer->end(), length);
    // Convert data to string
    std::string stringData(data, data + length);

    // Then
    // Bold attribute
    std::string expectAttr1 = "<attr name=\"weight\" type=\"gint\" value=\"700\" />";
    // Strikethrough attribute
    std::string expectAttr2 = "<attr name=\"strikethrough\" type=\"gboolean\" value=\"TRUE\" />";
    // Super-/subcript attribute
    std::string expectAttr3 =
        "<attr name=\"scale\" type=\"gdouble\" value="; // Depending on platform the value could be in UK or US format (so we removed the value)
    // Superscript attributes
    std::string expectAttr4 = "<attr name=\"rise\" type=\"gint\" value=\"-6144\" />";
    // Subscript attributes
    std::string expectAttr5 = "<attr name=\"rise\" type=\"gint\" value=\"6144\" />";
    // Code attributes
    std::string expectAttr6 = "<attr name=\"background-gdk\" type=\"GdkColor\" value=\"e0e0:e0e0:e0e0\" />";
    std::string expectAttr7 = "<attr name=\"foreground-gdk\" type=\"GdkColor\" value=\"3232:3232:3232\" />";
    std::string expectAttr8 = "<attr name=\"family\" type=\"gchararray\" value=\"monospace\" />";
    EXPECT_THAT(stringData, testing::HasSubstr(expectAttr1));
    EXPECT_THAT(stringData, testing::HasSubstr(expectAttr2));
    EXPECT_THAT(stringData, testing::HasSubstr(expectAttr3));
    EXPECT_THAT(stringData, testing::HasSubstr(expectAttr4));
    EXPECT_THAT(stringData, testing::HasSubstr(expectAttr5));
    EXPECT_THAT(stringData, testing::HasSubstr(expectAttr6));
    EXPECT_THAT(stringData, testing::HasSubstr(expectAttr7));
    EXPECT_THAT(stringData, testing::HasSubstr(expectAttr8));
  }
} // namespace
