#include "file.h"
#include "gtest/gtest.h"
#include <string>
namespace
{
  TEST(LibreWebTest, TestGetFilename)
  {
    // Given
    std::string path = "/path/to/a/filename.sh";
    std::string expected_filename = "filename.sh";
    // When
    std::string filename = File::get_filename(path);
    // Then
    ASSERT_EQ(filename, expected_filename);
  }
} // namespace