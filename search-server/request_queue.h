#pragma once
#include "search_server.h"
#include "document.h"

#include <vector>
#include <deque>
#include <string>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        timer_ = (timer_ + 1) % min_in_day_;
        if (requests_.size() == min_in_day_) {
            if (requests_.front().isEmpty) {
                --no_result_count_;
            }
            requests_.pop_front();
        }
        const std::vector<Document> result = server_.FindTopDocuments(raw_query, document_predicate);
        if (result.size() == 0)
            ++no_result_count_;
        requests_.push_back(QueryResult{ timer_, result, result.size() == 0 });
        return result;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        int time;
        std::vector<Document> result_;
        bool isEmpty;
    };
    const SearchServer& server_;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int no_result_count_;
    int timer_;
};