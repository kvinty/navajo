#include "query_parser.hpp"

void split(const std::string &str, std::vector<std::string> &parts) {
    parts = {};
    size_t start = 0, finish, size = str.size();
    for (;;) {
        while (start != str.size() && isspace(str[start])) {
            ++start;
        }
        if (start == size) {
            break;
        }
        finish = start;
        while (finish != str.size() && !isspace(str[finish])) {
            ++finish;
        }
        parts.push_back(str.substr(start, finish - start));
        start = finish;
    }
}

bool parse_method(
    const std::string &input,
    std::string &method,
    std::string &version,
    std::string &resource,
    std::string &query) {
    std::vector<std::string> parts;
    split(input, parts);
    if (parts.size() != 3) {
        return false;
    }
    method = parts[0];
    if (method != "GET" && method != "POST") {
        return false;
    }
    size_t question_mark = parts[1].find('?');
    resource = parts[1].substr(1, question_mark - 1);
    if (question_mark != std::string::npos) {
        query = parts[1].substr(
            question_mark + 1,
            parts[1].size() - question_mark - 1);
    } else {
        query = "";
    }
    if (parts[2] == "HTTP/1.1") {
        version = "1.1";
    } else {
        return false;
    }
    return true;
}

bool parse_field(
    const std::string &input,
    std::string &a,
    std::string &b) {
    std::vector<std::string> parts;
    split(input, parts);
    if (parts.size() >= 2 && parts[0][parts[0].size() - 1] == ':') {
        parts[0].pop_back();
        a = parts[0];
        b = parts[1];
        return true;
    }
    return false;
}
