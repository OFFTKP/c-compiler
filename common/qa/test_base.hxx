#ifndef TEST_BASE_HXX
#define TEST_BASE_HXX
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <vector>
#include <string>

class TestBase : public CppUnit::TestFixture {

protected:
    const std::vector<std::string>& getDataFiles();
    const std::vector<std::string>& getErrorFiles();
private:
    std::vector<std::string> data_files_;
};
#endif