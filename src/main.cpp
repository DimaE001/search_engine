#include <iostream>
#include "ConverterJSON.h"
#include "InvertedIndex.h"
#include "SearchServer.h"
#include "constantsSearchServer.h"


int main() {
    
    try {
        // 1. Читаем config
        ConverterJSON converter;
        converter.getConfigJson(CONFIG_FILE);

        // 2. Получаем документы
        std::vector<std::string> docs = converter.getTextDocuments();
        
        // 3. Строим индекс
        InvertedIndex index;
        index.UpdateDocumentBase(docs);
        
        // 4. Создаём SearchServer
        SearchServer searchServer(index, converter.getResponsesLimit());

        // 5. Читаем запросы
        std::vector<std::string> requests = converter.getRequests(REQUESTS_FILE);
            
        // 6. Выполняем поиск
        auto answers = searchServer.search(requests);
        
        // 7. Записываем ответы
        converter.putAnswers(answers, ANSWERS_FILE);

        for (size_t i = 0; i < answers.size(); ++i) {
            std::cout << "Request " << i + 1 << ":\n";

            if (answers[i].empty()) {
                std::cout << "  No results\n";
                continue;
            }

            for (const auto& item : answers[i]) {
                std::cout << "  docid: " << item.doc_id
                          << ", rank: " << item.rank << "\n";
            }
        }

        std::cout << "Search completed successfully." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    

    return 0;
}