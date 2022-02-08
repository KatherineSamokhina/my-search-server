#include "search_server.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const double EPSILON = 1e-6;

// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
    }
}
void TestAddDocument() {
    SearchServer server;
    server.AddDocument(1, "cat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "dog with pretty eyes"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(4, "bird eugene"s, DocumentStatus::BANNED, { 1, 2, 3 });
    server.AddDocument(5, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    vector<int> result;
    for (const auto& document : server.FindTopDocuments("cat in the city"s))
        result.push_back(document.id);
    ASSERT_HINT(result.size() == 3, "Not all added documents were found by request"s);
    vector<int> expected = { 1, 5, 2 };
    ASSERT(result == expected);
}
void TestSetStopWords() {
    SearchServer server;
    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto result = server.FindTopDocuments("cat"s);
    ASSERT_HINT(result.size() == 1, "Not all added documents were found by request"s);
    server.SetStopWords("in the"s);
    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
}
void TestMinusWords() {
    SearchServer server;
    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    ASSERT_HINT(server.FindTopDocuments("-in"s).empty(), "Documents with minus-words must be excluded from the results");
}
void TestMatchingDocuments() {
    SearchServer server;
    server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "dog out the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "dog in big box"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(4, "bird eugene in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const string query = "in the city -eugene";
    const auto result0 = server.MatchDocument(query, 4);
    ASSERT_EQUAL_HINT(get<0>(result0).size(), 0, "Documents with minus-words must be excluded from the results");
    const auto result1 = server.MatchDocument(query, 3);
    ASSERT_HINT(get<0>(result1).size() == 1, "Not all added documents were found by request"s);
    const auto result2 = server.MatchDocument(query, 2);
    ASSERT_HINT(get<0>(result2).size() == 2, "Not all added documents were found by request"s);
    const auto result3 = server.MatchDocument(query, 1);
    ASSERT_HINT(get<0>(result3).size() == 3, "Not all added documents were found by request"s);
}
void TestRelevanceSort() {
    SearchServer server;
    server.AddDocument(1, "cat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "dog in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(3, "dog with pretty eyes"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(4, "bird eugene"s, DocumentStatus::BANNED, { 1, 2, 3 });
    server.AddDocument(5, "cat in the city"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    const auto result = server.FindTopDocuments("bird in the city"s);
    ASSERT(result.size() == 2);
    ASSERT_HINT((result[0]).id == 2, "Results should be sorted in descending order of relevance"s);
    ASSERT_HINT((result[1]).id == 5, "Results should be sorted in descending order of relevance"s);
}
void TestComputeAverageRating() {
    SearchServer server;
    server.AddDocument(1, "cat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "dog in the city"s, DocumentStatus::ACTUAL, { 4, 5, 6 });
    server.AddDocument(3, "dog with pretty eyes"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 });
    server.AddDocument(4, "bird eugene"s, DocumentStatus::BANNED, { 1, 2, 3 });
    server.AddDocument(5, "cat in the city"s, DocumentStatus::ACTUAL, { 7, 8, 9 });
    const string query = "cat in the city"s;
    vector<int> result;
    for (const auto& document : server.FindTopDocuments(query, DocumentStatus::ACTUAL))
        result.push_back(document.rating);
    ASSERT_HINT(result.size() == 3, "Not all added documents were found by request"s);
    vector<int> expected = { 8, 2, 5 };
    ASSERT_HINT(result == expected, "The rating of the added document must be equal to the arithmetic mean of the ratings of the document"s);
}
void TestUserPredicate() {
    SearchServer server;
    server.AddDocument(1, "cat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "dog in the city"s, DocumentStatus::ACTUAL, { 4, 5, 6 });
    server.AddDocument(3, "dog with the pretty eye"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 });
    server.AddDocument(4, "bird eugene in the city"s, DocumentStatus::BANNED, { 1, 2, 3 });
    server.AddDocument(5, "cat in the city"s, DocumentStatus::ACTUAL, { 7, 8, 9 });
    const string query = "cat in the city"s;
    vector<Document> result1 = server.FindTopDocuments(query, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL_HINT(result1.size(), 1, "Documents with IRRELEVANT status are processed incorrectly"s);
    ASSERT_HINT((result1[0]).id == 3, "Documents with IRRELEVANT status are processed incorrectly"s);
    vector<Document> result2 = server.FindTopDocuments(query, DocumentStatus::BANNED);
    ASSERT_EQUAL_HINT(result2.size(), 1, "Documents with BANNED status are processed incorrectly"s);
    ASSERT_HINT((result2[0]).id == 4, "Documents with BANNED status are processed incorrectly"s);
    vector<int> result3;
    for (const auto& document : server.FindTopDocuments(query, DocumentStatus::ACTUAL))
        result3.push_back(document.id);
    ASSERT_EQUAL_HINT(result3.size(), 3, "Documents with ACTUAL status are processed incorrectly"s);
    vector<int> expected = { 1, 5, 2 };
    ASSERT_HINT(result3 == expected, "Documents with ACTUAL status are processed incorrectly"s);
}
void TestFixedPredicate() {
    SearchServer server;
    server.AddDocument(1, "cat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "dog in the city"s, DocumentStatus::ACTUAL, { 4, 5, 6 });
    server.AddDocument(3, "dog with pretty eyes"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 });
    server.AddDocument(4, "bird eugene in the city"s, DocumentStatus::BANNED, { 1, 2, 3 });
    server.AddDocument(5, "cat in the city"s, DocumentStatus::ACTUAL, { 7, 8, 9 });
    const string query = "cat in the city"s;
    const auto result = server.FindTopDocuments(query);
    ASSERT_HINT(result.size() == 3, "Documents with fixed status are processed incorrectly"s);
}
void TestComputeRelevance() {
    SearchServer server;
    server.AddDocument(1, "cat"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    server.AddDocument(2, "dog in the city"s, DocumentStatus::ACTUAL, { 4, 5, 6 });
    server.AddDocument(3, "dog with pretty eyes"s, DocumentStatus::IRRELEVANT, { 1, 2, 3 });
    server.AddDocument(4, "bird eugene in the city"s, DocumentStatus::BANNED, { 1, 2, 3 });
    server.AddDocument(5, "cat in the city"s, DocumentStatus::ACTUAL, { 7, 8, 9 });
    vector<Document> result = server.FindTopDocuments("cat in the city"s, DocumentStatus::ACTUAL);
    ASSERT(result.size() == 3);
    ASSERT_HINT(abs(result[0].relevance - 0.916290) < EPSILON, "Document relevance is calculated incorrectly"s);
    ASSERT_HINT(abs(result[1].relevance - 0.612191) < EPSILON, "Document relevance is calculated incorrectly"s);
    ASSERT_HINT(abs(result[2].relevance - 0.383119) < EPSILON, "Document relevance is calculated incorrectly"s);
}
// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddDocument);
    RUN_TEST(TestSetStopWords);
    RUN_TEST(TestMinusWords);
    RUN_TEST(TestMatchingDocuments);
    RUN_TEST(TestRelevanceSort);
    RUN_TEST(TestComputeAverageRating);
    RUN_TEST(TestUserPredicate);
    RUN_TEST(TestFixedPredicate);
    RUN_TEST(TestComputeRelevance);
}

// --------- Окончание модульных тестов поисковой системы -----------