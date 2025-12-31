
#include <print>
#include <string>
#include <vector>
#include <cctype>
#include <cassert>

std::string get_code()
{
    return "let x = 10;\nif x > 5 then x + 1 else 0;";
}

bool is_whitespace(char c)
{
    return std::isspace(static_cast<unsigned char>(c));
}

int main()
{
    using ExpressionWords = std::vector<std::string>;

    std::string code = get_code();
    std::println("The code:\n{}\n", code);

    std::vector<ExpressionWords> expressions;
    // TODO: Prolly can just have views / ranges instead of copying?
    {
        ExpressionWords words;
        std::println("Starting to parse words:");
        size_t start_idx = 0;
        while (start_idx < code.length()) // Each iteration is one word, whitespace trimmed
        {
            while (start_idx < code.length() && is_whitespace(code[start_idx]))
            {
                ++start_idx;
            }
            if (start_idx >= code.length()) break;
            size_t curr_idx = start_idx;

            [[maybe_unused]] bool found_semicolon = false;
            while (curr_idx < code.length())
            {
                char c = code[curr_idx];
                if (is_whitespace(c))
                {
                    break;
                }
                else if (c == ';')
                {
                    found_semicolon = true;
                    break;
                }
                ++curr_idx;
            }
            assert(curr_idx > start_idx);
            auto word_diff = curr_idx - start_idx;
            auto word = code.substr(start_idx, word_diff);
            words.push_back(word);

            if(found_semicolon) {
                ++curr_idx;
                expressions.push_back(std::move(words));
                words = {};
            }
            start_idx = curr_idx;
        }
        std::println("Finished parsing words!\n");
        std::println("Found {} expressions with {}", expressions.size(), expressions);
    }
    std::println("The Token strings:");
}