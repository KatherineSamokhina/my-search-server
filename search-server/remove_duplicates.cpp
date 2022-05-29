#include "remove_duplicates.h"
#include "search_server.h"

std::vector<int> FindDuplicates(SearchServer& search_server) {
	std::vector<int> to_remove;
	std::set<std::set<std::string_view>> unique_words;
	for (auto it = search_server.begin(); it != search_server.end(); ++it) {
		std::map<std::string_view, double> word_to_freq = search_server.GetWordFrequencies(*it);
		std::set<std::string_view> words;
		for (const auto& [word, _] : word_to_freq)
			words.insert(word);
		if (unique_words.count(words) == 0)
			unique_words.insert(words);
		else
			to_remove.push_back(*it);
	}
	return to_remove;
}

void RemoveDuplicates(SearchServer& search_server) {
	for (int id : FindDuplicates(search_server)) {
		std::cout << "Found duplicate document id " << id << std::endl;
		search_server.RemoveDocument(id);
	}
}
