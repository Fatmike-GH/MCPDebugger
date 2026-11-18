#include "Mocks/PEFile/Interfaces/MockPEFile.h"
#include "Mocks/PEFile/Interfaces/MockPEFileFactory.h"
#include "Native/PEFile/ExecutableValidator.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace MCP::Native::Tests::PEFile
{
  using MCP::Native::PEFile::ExecutableValidator;
  using MCP::Native::PEFile::Interfaces::IPEFile;
  using MCP::Native::Tests::Mocks::PEFile::Interfaces::MockPEFile;
  using MCP::Native::Tests::Mocks::PEFile::Interfaces::MockPEFileFactory;
  using ::testing::ByMove;
  using ::testing::NiceMock;
  using ::testing::Return;
  using ::testing::StrictMock;

  // The validator accepts executables of the bitness this test binary was built for, and names that bitness
  // in its error text.
#ifdef _WIN64
  constexpr bool BuildIsx64 = true;
  constexpr const char* RejectedText = "The provided file is not a valid x64 PE file.";
#else
  constexpr bool BuildIsx64 = false;
  constexpr const char* RejectedText = "The provided file is not a valid x86 PE file.";
#endif

  class ExecutableValidatorTests : public ::testing::Test
  {
  protected:
    ExecutableValidatorTests()
    {
      auto peFileFactory = std::make_unique<StrictMock<MockPEFileFactory>>();
      _peFileFactory = peFileFactory.get();
      _validator = std::make_unique<ExecutableValidator>(std::move(peFileFactory));
    }

    /// <summary>
    /// Lets the factory open one file with the given header answers. The file double is lenient about which
    /// answers are asked for: the validator may stop at the first one that rejects the file.
    /// </summary>
    void ExpectFile(bool isValid, bool isx64, bool isNative)
    {
      auto peFile = std::make_unique<NiceMock<MockPEFile>>();
      ON_CALL(*peFile, IsValid()).WillByDefault(Return(isValid));
      ON_CALL(*peFile, Isx64()).WillByDefault(Return(isx64));
      ON_CALL(*peFile, IsNative()).WillByDefault(Return(isNative));
      EXPECT_CALL(*_peFileFactory, Open(Path)).WillOnce(Return(ByMove(std::unique_ptr<IPEFile>(std::move(peFile)))));
    }

  protected:
    const std::string Path = "Example.exe";

    // Owned by the validator under test.
    StrictMock<MockPEFileFactory>* _peFileFactory = nullptr;

    std::unique_ptr<ExecutableValidator> _validator;
  };

  TEST_F(ExecutableValidatorTests, ValidateAcceptsANativeExecutableOfTheBitnessOfTheBuild)
  {
    // Arrange
    ExpectFile(true, BuildIsx64, true);
    std::string error = "unchanged";

    // Act
    bool accepted = _validator->Validate(Path, error);

    // Assert
    EXPECT_TRUE(accepted);
    EXPECT_EQ("unchanged", error);
  }

  TEST_F(ExecutableValidatorTests, ValidateRejectsAFileThatIsNotAPEImage)
  {
    // Arrange
    ExpectFile(false, BuildIsx64, true);
    std::string error;

    // Act
    bool accepted = _validator->Validate(Path, error);

    // Assert
    EXPECT_FALSE(accepted);
    EXPECT_EQ(RejectedText, error);
  }

  TEST_F(ExecutableValidatorTests, ValidateRejectsAnExecutableOfTheOtherBitness)
  {
    // Arrange
    ExpectFile(true, !BuildIsx64, true);
    std::string error;

    // Act
    bool accepted = _validator->Validate(Path, error);

    // Assert
    EXPECT_FALSE(accepted);
    EXPECT_EQ(RejectedText, error);
  }

  TEST_F(ExecutableValidatorTests, ValidateRejectsAnExecutableThatIsNotNative)
  {
    // Arrange
    ExpectFile(true, BuildIsx64, false);
    std::string error;

    // Act
    bool accepted = _validator->Validate(Path, error);

    // Assert
    EXPECT_FALSE(accepted);
    EXPECT_EQ(RejectedText, error);
  }
}
