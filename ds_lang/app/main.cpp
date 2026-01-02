#include <print>
#include <string>
#include <vector>

#include "util.hpp"
#include "types.hpp"

int main()
{
    using namespace ds_lang;

    load_code("examples/new.ds2");

    std::string code = "LET x = 1\nPRINT x";

    std::vector<std::vector<std::string>> lines;

    usize current_pos = 0;
    while (current_pos < code.size())
    {
        std::vector<std::string> words;
        // Process current line
        while (current_pos < code.size() && code[current_pos] != '\n')
        {
            // Skip till new word
            while (current_pos < code.size() && code[current_pos] != '\n' && is_whitespace(code[current_pos]))
            {
                ++current_pos;
            }
            if (current_pos >= code.size() || code[current_pos] == '\n') break;

            usize start = current_pos;
            while (current_pos < code.size() && code[current_pos] != ' ' && code[current_pos] != '\n')
            {
                ++current_pos;
            }

            words.push_back(code.substr(start, current_pos - start));
        }
        lines.push_back(std::move(words));
        if (current_pos < code.size() && code[current_pos] == '\n') ++current_pos;
    }

    std::println("{}", lines);
}