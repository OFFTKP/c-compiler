#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>
#include <iostream>
#include <common/state.hxx>

int main() {
    CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();
    CppUnit::TextUi::TestRunner runner;
    runner.addTest(suite);
    runner.setOutputter(new CppUnit::CompilerOutputter(&runner.result(), std::cerr));
    bool wasSuccessful = runner.run();
    auto& log = Variables::GetLog();
    std::cout << "Dumping log:" << std::endl;
    for (const auto& entry : log) {
        std::cout << entry << std::endl;
    }
    auto& warnings = Variables::GetWarnings();
    std::cout << "Dumping warnings:" << std::endl;
    for (const auto& entry : warnings) {
        std::cout << entry << std::endl;
    }
    auto& errors = Variables::GetErrors();
    std::cout << "Dumping errors:" << std::endl;
    for (const auto& entry : errors) {
        std::cout << entry << std::endl;
    }
    return wasSuccessful ? 0 : 1;
}