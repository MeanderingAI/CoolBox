#include "cli_tools.hpp"
#include "tyst_framework.hpp"

#include <vector>

TEST(CliTools, ParsesLongAndShortOptions) {
    os_generics::cli::CommandLineParser parser;
    parser.add_option({"verbose", 'v', false, false, "", "Verbose output"});
    parser.add_option({"output", 'o', true, false, "PATH", "Output file"});

    const std::vector<std::string> args = {"tool", "-v", "--output", "result.txt", "input.dat"};
    const auto result = parser.parse(args);

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.has_option("verbose"));
    EXPECT_EQ(result.option_value("output"), "result.txt");
    EXPECT_EQ(result.positionals.size(), 1U);
    EXPECT_EQ(result.positionals[0], "input.dat");
}

TEST(CliTools, ReportsMissingRequiredOption) {
    os_generics::cli::CommandLineParser parser;
    parser.add_option({"config", 'c', true, true, "FILE", "Required config file"});

    const std::vector<std::string> args = {"tool"};
    const auto result = parser.parse(args);

    EXPECT_FALSE(result.ok());
    EXPECT_FALSE(result.errors.empty());
}

TEST(CliTools, ParsesCompactShortFlags) {
    os_generics::cli::CommandLineParser parser;
    parser.add_option({"a", 'a', false, false, "", "Flag A"});
    parser.add_option({"b", 'b', false, false, "", "Flag B"});
    parser.add_option({"c", 'c', false, false, "", "Flag C"});

    const std::vector<std::string> args = {"tool", "-abc"};
    const auto result = parser.parse(args);

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.has_option("a"));
    EXPECT_TRUE(result.has_option("b"));
    EXPECT_TRUE(result.has_option("c"));
}

TEST(CliTools, RendersHelp) {
    os_generics::cli::CommandLineParser parser;
    parser.set_program_name("demo");
    parser.set_description("Demo parser");
    parser.add_option({"output", 'o', true, false, "PATH", "Output file"});

    const std::string help = parser.render_help();
    EXPECT_NE(help.find("Usage: demo"), std::string::npos);
    EXPECT_NE(help.find("--output PATH"), std::string::npos);
}
