#include <vector>
#include <string>
#include <unordered_set>
#include <sstream>

#include "utilsSearchServer.h"
#include "constantsSearchServer.h"


std::vector<std::string> getUniqWords(const std::vector<std::string>& queries_input)
{
    std::vector<std::string> res;

    for (const auto& query : queries_input) {

        std::unordered_set<std::string> uniq;

        std::istringstream stream(query);
        std::string word;
        std::string str;

        while (stream >> word) {

            if (uniq.insert(word).second) {
                if (!str.empty()) str += ' ';
                str += word;
            }
        }

        res.push_back(str);
    }

    return res;
}

bool isValidDocument(const std::string& text)
{
    std::istringstream stream(text);
    std::string word;
    size_t word_count = 0;

    while (stream >> word) {
        ++word_count;

        if (word_count > MAX_DOCUMENT_WORDS) {
            return false;
        }

        if (word.size() > MAX_DOCUMENT_WORD_LENGTH) {
            return false;
        }

        for (char c : word) {
            if (c < 'a' || c > 'z') {
                return false;
            }
        }
    }

    return true;
}


bool isValidRequest(const std::string& request)
{
    std::istringstream stream(request);
    std::string word;
    size_t word_count = 0;

    while (stream >> word) {
        ++word_count;

        if (word_count > MAX_REQUEST_WORDS) {
            return false;
        }

        for (char c : word) {
            if (c < 'a' || c > 'z') {
                return false;
            }
        }
    }

    return word_count >= 1;
}