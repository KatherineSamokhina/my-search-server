#include "test_example_functions.h"

void PrintDocument_(const Document& document) {
    std::cout << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating << " }" << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status) {
    std::cout << "{ "
        << "document_id = " << document_id << ", "
        << "status = " << static_cast<int>(status) << ", "
        << "words =";
    for (const auto word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}" << std::endl;
}

void FindTopDocuments(const SearchServer& search_server, const std::string_view raw_query) {
    std::cout << "Search results for: " << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument_(document);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error during searching: " << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string_view query) {
    try {
        std::cout << "Matching documents for the request: " << query << std::endl;
        for (const int document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
        /*
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
        */
    }
    catch (const std::exception& e) {
        std::cout << "Error during matching documents for the request " << query << ": " << e.what() << std::endl;
    }
}