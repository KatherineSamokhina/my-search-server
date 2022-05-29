#include "search_server.h"


SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{}

SearchServer::SearchServer(const std::string_view stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("The document id must be non-negative");
    }
    if (documents_.count(document_id) > 0) {
        throw std::invalid_argument("The document id must be unique, such id already exists");
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const auto word : words) {
        auto it = words_.insert(std::string(word));
        std::string_view word_pointer = *(it.first);
        word_to_document_freqs_[word_pointer][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word_pointer] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, status);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query);
}

int SearchServer::GetDocumentCount() const {
    return (int)documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

std::pair<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    if (document_id < 0 || document_ids_.count(document_id) == 0) {
        throw std::out_of_range("out_of_range");
    }

    const auto query = SearchServer::ParseQuery(std::execution::seq, raw_query);
    std::vector<std::string_view> matched_words;
    const std::map<std::string_view, double>& word_freq = GetWordFrequencies(document_id);

    if (any_of(query.minus_words.begin(), query.minus_words.end(),
        [&word_freq](const auto& word) { return word_freq.count(word); }
    ))
    {
        return { {}, documents_.at(document_id).status };
    }

    copy_if(query.plus_words.begin(), query.plus_words.end(), std::back_inserter(matched_words),
        [&word_freq, &matched_words](std::string_view word) {
            return word_freq.count(word);
        });

    sort(matched_words.begin(), matched_words.end());
    auto last = unique(matched_words.begin(), matched_words.end());
    matched_words.erase(last, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

std::pair<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const {
    
    return SearchServer::MatchDocument(raw_query, document_id);
}

std::pair<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const {
    if (document_id < 0 || document_ids_.count(document_id) == 0) {
        throw std::out_of_range("out_of_range");
    }

    const auto query = ParseQuery(std::execution::par, raw_query);
    std::vector<std::string_view> matched_words;
    matched_words.reserve(query.plus_words.size());

    const std::map<std::string_view, double>& word_freq = GetWordFrequencies(document_id);

    bool is_minus = any_of(query.minus_words.begin(), query.minus_words.end(),
        [&word_freq](const auto& word) { return word_freq.count(word); }
    );
    if (is_minus)
    {
        return { {}, documents_.at(document_id).status };
    }

    copy_if(query.plus_words.begin(), query.plus_words.end(),
        std::back_inserter(matched_words),
        [&word_freq, &matched_words](std::string_view word) {
            return word_freq.count(word);
        });

    sort(matched_words.begin(), matched_words.end());
    auto last = unique(matched_words.begin(), matched_words.end());
    matched_words.erase(last, matched_words.end());

    return { matched_words, documents_.at(document_id).status };
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Some words have invalid symbols");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Invalid search request");
    }

    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Invalid search request");
    }

    return QueryWord{ word, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    Query query;

    for (const auto word : SplitIntoWords(text)) {
        QueryWord query_word = ParseQueryWord(word);

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }

    std::sort(query.plus_words.begin(), query.plus_words.end());
    auto to_erase = std::unique(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(to_erase, query.plus_words.end());

    std::sort(query.minus_words.begin(), query.minus_words.end());
    to_erase = std::unique(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(to_erase, query.minus_words.end());

    return query;
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::sequenced_policy& policy, const std::string_view text) const {
    return SearchServer::ParseQuery(text);
}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy& policy, const std::string_view text) const {
    Query query;

    for (const auto word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);

        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                query.minus_words.push_back(query_word.data);
            }
            else {
                query.plus_words.push_back(query_word.data);
            }
        }
    }
    return query;
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static std::map<std::string_view, double> result;
    
    auto it = document_to_word_freqs_.find(document_id);
    if (it != document_to_word_freqs_.end())
    {
        return ((*it).second);
    }
    else
    {
        return result;
    }
}

void SearchServer::RemoveDocument(int document_id) {
    auto pos_to_remove = std::find(document_ids_.begin(), document_ids_.end(), document_id);
    if (pos_to_remove == document_ids_.end())
        return;
    document_ids_.erase(pos_to_remove);
    documents_.erase(document_id);
    for (auto& [word,_] : document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_[word].erase(document_id);
    }
    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id) {
    SearchServer::RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id) {
    if (!document_ids_.count(document_id)) {
        return;
    }

    const std::map<std::string_view, double>& m = GetWordFrequencies(document_id);
    std::vector<std::string_view> v(m.size());
    std::transform(policy, m.begin(), m.end(), v.begin(),
        [](auto& p) { return p.first; });
    std::for_each(policy, v.begin(), v.end(),
        [this, document_id](auto& word)
        { word_to_document_freqs_[word].erase(document_id); }
    );

    document_ids_.erase(document_id);
    documents_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}