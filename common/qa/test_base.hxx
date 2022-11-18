#ifndef TEST_BASE_HXX
#define TEST_BASE_HXX
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <vector>
#include <string>

class TestBase : public CppUnit::TestFixture {

protected:
    std::vector<std::string> getDataFiles(std::string suffix = "");
    std::string getSource(const std::string& path);
    std::vector<std::string> getLines(const std::string& source);
    std::string getDataPath();
private:
    std::vector<std::string> data_files_;
};
#endif