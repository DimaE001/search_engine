#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "ConverterJSON.h"
#include "utilsSearchServer.h" 

void ConverterJSON::getConfigJson(std::string& path){
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error("config file is missing: " + path);
    }
    nlohmann::json config;
    file >> config;

    if (!config.contains("config") || !config["config"].is_object()) {
        throw std::invalid_argument("config file is empty");
    }
 
    // проверить 
    if (!config["config"].contains("version") || 
            !config["config"]["version"].is_string()) {
        throw std::invalid_argument("version missing");
    }
    std::string version = config["config"]["version"].get<std::string>();
    if (version != ENGINE_VERSION) {
        throw std::runtime_error("config.json has incorrect file version");
    }
    

    if (config["config"].contains("max_responses")) {

    if (!config["config"]["max_responses"].is_number_integer()) {
        throw std::invalid_argument("'max_responses' must be integer");
    }

    maxResponses_ = config["config"]["max_responses"].get<int>();
    }
    
    

    if (!config.contains("files") || !config["files"].is_array()) {
        throw std::invalid_argument("missing 'files' array");
    }

    // очистить старые данные
    textDocuments_.clear();

    // загрузить текст файлов по порядку docid индекс
    for (const auto& item : config["files"]) {

        if (!item.is_string()) {
            throw std::invalid_argument("file path must be string");
        }

        std::string filePath = item.get<std::string>();

        std::ifstream doc(filePath);
        if (!doc.is_open()) {
            std::cerr << "Error: text file not found: " << filePath << std::endl;
            // сохраняем doc_id
            textDocuments_.push_back("");
            continue;
        }

        std::string content(
            (std::istreambuf_iterator<char>(doc)),
             std::istreambuf_iterator<char>()
        );

        if (!isValidDocument(content)) {
            std::cerr << "Error: invalid document format: " << filePath << std::endl;
            // сохраняем doc_id
            textDocuments_.push_back(""); 
            continue;
        }

        textDocuments_.push_back(content);
    }
}

std::vector<std::string> ConverterJSON::getTextDocuments(){
    return textDocuments_;
}

size_t ConverterJSON::getResponsesLimit(){
    return maxResponses_;
}

std::vector<std::string> ConverterJSON::getRequests(std::string requestsPath_){
    std::ifstream file(requestsPath_);

    if (!file.is_open()) {
        throw std::runtime_error("requests file is missing: " + requestsPath_);
    }
    nlohmann::json requests;
    file >> requests;

    if (!requests.contains("requests") || !requests["requests"].is_array()) {
        throw std::invalid_argument("requests file is empty or invalid");
    }

    if (requests["requests"].size() > MAX_REQUESTS) {
        throw std::invalid_argument("too many requests");
    }

    std::vector<std::string> result;  
    
    for (const auto& item : requests["requests"]) {

        if (!item.is_string()) {
            throw std::invalid_argument("requests must be string");
        }

        std::string req = item.get<std::string>();

        if (!isValidRequest(req)) {
            std::cerr << "Error: invalid request format\n";
            result.push_back("");
            continue;
        }
        result.push_back(req);

    }

    return result; 
}

void ConverterJSON::putAnswers(const std::vector<std::vector<RelativeIndex>>& answers, std::string answersPath_) {
    nlohmann::json result;
    result["answers"] = nlohmann::json::object();

    for (size_t i = 0; i < answers.size(); ++i) {
        std::ostringstream key;
        key << "request" << std::setw(3) << std::setfill('0') << (i + 1);
        const std::string requestKey = key.str();

        const auto& vec = answers[i];

        if (vec.empty()) {
            result["answers"][requestKey]["result"] = false;
            continue;
        }
        result["answers"][requestKey]["result"] = true;
        result["answers"][requestKey]["relevance"] = nlohmann::json::array();


        for (const auto& ri : vec) {
            result["answers"][requestKey]["relevance"].push_back({
                {"docid", ri.doc_id},
                {"rank",  ri.rank}
            });
        }
    }

    // создать или очистить файл std::ofstream по умолчанию открывает с trunc
    std::ofstream out(answersPath_, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot open answers.json for writing: " + answersPath_);
    }

    out << result.dump(4);
}
