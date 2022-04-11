#include "remove_duplicates.h"
#include "search_server.h"

std::vector<int> FindDuplicates(SearchServer& search_server) {
	std::vector<int> to_remove;
	for (auto it_first = search_server.begin(); it_first != search_server.end() - 1; ++it_first) {
		if (count(to_remove.begin(), to_remove.end(), *it_first) != 0)
			continue;
		std::map<std::string, double> first = search_server.GetWordFrequencies(*it_first);
		for (auto it_second = it_first + 1; it_second != search_server.end(); ++it_second) {
			std::map<std::string, double> second = search_server.GetWordFrequencies(*it_second);
			bool is_equal = true;
			if (first.size() != second.size())
				continue;
			for (const auto& [key, _] : first) {
				if (second.count(key) == 0) {
					is_equal = false;
					break;
				}
			}
			if (is_equal)
				to_remove.push_back(*it_second);
		}
	}
	return to_remove;
}

void RemoveDuplicates(SearchServer& search_server) {
	for (int id : FindDuplicates(search_server)) {
		std::cout << "Found duplicate document id " << id << std::endl;
		search_server.RemoveDocument(id);
	}
}
