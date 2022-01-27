#include "file.h"
#include "gtest/gtest.h"
#include <string>
namespace
{
  TEST(LibreWebTest, TestGetFilename)
  {
    // Given
    std::string path = "/path/to/a/filename.sh";
    std::string expectedFilename = "filename.sh";
    // When
    std::string filename = File::getFilename(path);
    // Then
    ASSERT_EQ(filename, expectedFilename);
  }
} // namespace