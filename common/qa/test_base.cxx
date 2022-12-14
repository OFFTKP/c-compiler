#include <common/qa/test_base.hxx>
#include <common/global.hxx>
#include <filesystem>

std::vector<std::string> TestBase::getDataFiles(std::string suffix) {
    // TODO: data_files_ shouldnt be member
    #ifndef TEST_DATA_FILEPATH
    CPPUNIT_ASSERT(false && "TEST_DATA_FILEPATH not defined");
    #else
    data_files_.clear();
    auto path = std::string(TEST_DATA_FILEPATH) + "/" + suffix;
    CPPUNIT_ASSERT(std::filesystem::is_directory(path));
    for (const auto& entry : std::filesystem::directory_iterator(path))
        if (std::filesystem::is_regular_file(entry.path()))
            data_files_.push_back(entry.path().string());
    CPPUNIT_ASSERT(data_files_.size() > 0);
    #endif
    return data_files_;
}

std::string TestBase::getSource(const std::string& path) {
    Global::GetCurrentPath() = path;
    CPPUNIT_ASSERT(std::filesystem::is_regular_file(path));
    std::ifstream ifs(path);
    CPPUNIT_ASSERT(ifs.is_open() && ifs.good());
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

std::vector<std::string> TestBase::getLines(const std::string& source) {
    std::vector<std::string> lines;
    std::stringstream stream(source);
    std::string line;
    while (std::getline(stream, line))
        lines.push_back(line);
    return lines;
}

std::string TestBase::getDataPath() {
    #ifndef TEST_DATA_FILEPATH
    CPPUNIT_ASSERT(false && "TEST_DATA_FILEPATH not defined");
    #else
    return TEST_DATA_FILEPATH;
    #endif
}

std::vector<std::string> TestBase::split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back(token);
    }

    res.push_back(s.substr (pos_start));
    return res;
}