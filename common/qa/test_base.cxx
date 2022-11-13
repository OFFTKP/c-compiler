#include <common/qa/test_base.hxx>
#include <filesystem>

const std::vector<std::string>& TestBase::getDataFiles() {
    #ifndef TEST_DATA_FILEPATH
    CPPUNIT_ASSERT(false && "TEST_DATA_FILEPATH not defined");
    #else
    static bool cached = false;
    if (!cached) {
        cached = true;
        for (const auto& entry : std::filesystem::directory_iterator(TEST_DATA_FILEPATH))
            if (std::filesystem::is_regular_file(entry.path()))
                data_files_.push_back(entry.path().string());
    }
    CPPUNIT_ASSERT(data_files_.size() > 0);
    #endif
    return data_files_;
}

const std::vector<std::string>& TestBase::getErrorFiles() {
    #ifndef TEST_DATA_FILEPATH
    CPPUNIT_ASSERT(false && "TEST_DATA_FILEPATH not defined");
    #else
    static bool cached = false;
    if (!cached) {
        cached = true;
        for (const auto& entry : std::filesystem::directory_iterator(TEST_DATA_FILEPATH "/must_error"))
            if (std::filesystem::is_regular_file(entry.path()))
                data_files_.push_back(entry.path().string());
    }
    CPPUNIT_ASSERT(data_files_.size() > 0);
    #endif
    return data_files_;
}