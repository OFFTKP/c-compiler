#include <common/test_base.hxx>
#include <filesystem>

const std::vector<std::string>& TestBase::getDataFiles() {
    #ifndef TEST_DATA_FILEPATH
    CPPUNIT_ASSERT(false && "TEST_DATA_FILEPATH not defined");
    #else
    static bool cached = false;
    if (!cached) {
        cached = true;
        for (const auto& entry : std::filesystem::directory_iterator(TEST_DATA_FILEPATH))
            data_files_.push_back(entry.path().string());
    }
    #endif
    return data_files_;
}