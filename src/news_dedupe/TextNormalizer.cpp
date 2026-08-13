#include "news_dedupe/TextNormalizer.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <sstream>

namespace news_dedupe {
namespace {

bool decodeCodePoint(const std::string& text, std::size_t& offset, char32_t& code_point) {
    const auto first = static_cast<unsigned char>(text[offset]);
    if (first < 0x80) {
        code_point = first;
        ++offset;
        return true;
    }

    std::size_t length = 0;
    char32_t value = 0;
    if ((first & 0xE0) == 0xC0) {
        length = 2;
        value = first & 0x1F;
    } else if ((first & 0xF0) == 0xE0) {
        length = 3;
        value = first & 0x0F;
    } else if ((first & 0xF8) == 0xF0) {
        length = 4;
        value = first & 0x07;
    } else {
        ++offset;
        return false;
    }

    if (offset + length > text.size()) {
        offset = text.size();
        return false;
    }

    for (std::size_t index = 1; index < length; ++index) {
        const auto next = static_cast<unsigned char>(text[offset + index]);
        if ((next & 0xC0) != 0x80) {
            ++offset;
            return false;
        }
        value = (value << 6) | (next & 0x3F);
    }

    offset += length;
    code_point = value;
    return true;
}

void appendCodePoint(std::string& output, char32_t code_point) {
    if (code_point <= 0x7F) {
        output.push_back(static_cast<char>(code_point));
    } else if (code_point <= 0x7FF) {
        output.push_back(static_cast<char>(0xC0 | (code_point >> 6)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else if (code_point <= 0xFFFF) {
        output.push_back(static_cast<char>(0xE0 | (code_point >> 12)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    } else {
        output.push_back(static_cast<char>(0xF0 | (code_point >> 18)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 12) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | ((code_point >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (code_point & 0x3F)));
    }
}

char32_t lowerCodePoint(char32_t code_point) {
    if (code_point >= U'A' && code_point <= U'Z') {
        return code_point + 32;
    }
    if (code_point >= 0x0410 && code_point <= 0x042F) {
        return code_point + 32;
    }
    switch (code_point) {
        case 0x0401: return 0x0435;  // Ё -> е
        case 0x0451: return 0x0435;  // ё -> е
        case 0x0404: return 0x0454;  // Є -> є
        case 0x0406: return 0x0456;  // І -> і
        case 0x0407: return 0x0457;  // Ї -> ї
        case 0x0490: return 0x0491;  // Ґ -> ґ
        default: return code_point;
    }
}

bool isWordCodePoint(char32_t code_point) {
    return (code_point >= U'a' && code_point <= U'z') ||
           (code_point >= U'0' && code_point <= U'9') ||
           (code_point >= 0x0430 && code_point <= 0x044F) ||
           code_point == 0x0454 || code_point == 0x0456 ||
           code_point == 0x0457 || code_point == 0x0491;
}

bool isNumber(const std::string& token) {
    return !token.empty() && std::all_of(token.begin(), token.end(), [](unsigned char value) {
        return std::isdigit(value) != 0;
    });
}

const std::set<std::string>& stopWords() {
    static const std::set<std::string> words = {
        "а", "але", "без", "был", "была", "были", "было", "будет", "в", "во",
        "вже", "для", "до", "его", "ее", "если", "есть", "еще", "за", "и", "из",
        "или", "как", "к", "на", "над", "не", "но", "о", "об", "от", "по", "под",
        "при", "про", "с", "со", "та", "так", "также", "там", "то", "у", "уже",
        "что", "щоб", "що", "це", "это", "як", "the", "and", "for", "with"
    };
    return words;
}

const std::map<std::string, std::string>& aliases() {
    static const std::map<std::string, std::string> values = {
        {"києві", "киев"}, {"києва", "киев"}, {"киеве", "киев"}, {"киева", "киев"},
        {"оголосили", "объявить"}, {"оголошено", "объявить"}, {"объявили", "объявить"},
        {"объявлена", "объявить"}, {"объявлено", "объявить"},
        {"повітряна", "воздушный"}, {"повітряну", "воздушный"},
        {"воздушная", "воздушный"}, {"воздушную", "воздушный"},
        {"тривога", "тревога"}, {"тривогу", "тревога"}, {"тревоги", "тревога"},
        {"вибух", "взрыв"}, {"вибухи", "взрыв"}, {"взрывы", "взрыв"},
        {"обстріл", "обстрел"}, {"обстріляли", "обстрел"}, {"обстреляли", "обстрел"},
        {"атакували", "атака"}, {"атаковали", "атака"}, {"дрони", "дрон"},
        {"шахеди", "shahed"}, {"шахеды", "shahed"}, {"ппо", "пво"},
        {"зсу", "всу"}, {"рф", "россия"}, {"погибли", "погиб"},
        {"погибла", "погиб"}, {"загинули", "погиб"}, {"ранены", "ранен"},
        {"поранені", "ранен"}, {"человека", "человек"}, {"человеков", "человек"}
    };
    return values;
}

}  // namespace

std::vector<std::string> TextNormalizer::tokenize(const std::string& text) const {
    std::vector<std::string> tokens;
    std::string current;
    std::size_t offset = 0;

    const auto flush = [&tokens, &current]() {
        if (current.empty()) {
            return;
        }
        const auto alias = aliases().find(current);
        const std::string token = alias == aliases().end() ? current : alias->second;
        if (stopWords().find(token) == stopWords().end()) {
            tokens.push_back(token);
        }
        current.clear();
    };

    while (offset < text.size()) {
        char32_t code_point = 0;
        if (!decodeCodePoint(text, offset, code_point)) {
            flush();
            continue;
        }
        code_point = lowerCodePoint(code_point);
        if (isWordCodePoint(code_point)) {
            appendCodePoint(current, code_point);
        } else {
            flush();
        }
    }
    flush();
    return tokens;
}

std::string TextNormalizer::normalize(const std::string& text) const {
    const auto tokens = tokenize(text);
    std::ostringstream output;
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (index > 0) {
            output << ' ';
        }
        output << tokens[index];
    }
    return output.str();
}

std::vector<std::string> TextNormalizer::extractNumbers(const std::string& text) const {
    std::vector<std::string> numbers;
    for (const auto& token : tokenize(text)) {
        if (isNumber(token)) {
            numbers.push_back(token);
        }
    }
    return numbers;
}

}  // namespace news_dedupe
