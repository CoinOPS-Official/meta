#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "rapidxml.hpp"

using namespace rapidxml;

struct GameInfo {
    std::string name;
    std::string indexAttr;
    std::string imageAttr;
};

static std::string loadFileToString(const std::string& filename) {
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

static void computeLineCol(const char* begin, const char* where, int& outLine, int& outCol) {
    // 1-based line/col
    int line = 1;
    const char* lastLineStart = begin;

    for (const char* p = begin; p && p < where && *p; ++p) {
        if (*p == '\n') {
            ++line;
            lastLineStart = p + 1;
        }
    }

    int col = static_cast<int>(where - lastLineStart) + 1;

    outLine = line;
    outCol  = col;
}

static std::string extractLineText(const std::string& text, int lineNumber) {
    std::istringstream iss(text);
    std::string line;
    int lineNo = 0;
    while (std::getline(iss, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (lineNo == lineNumber) return line;
    }
    return {};
}

static std::vector<GameInfo> parseGamesFromDoc(xml_document<>& doc) {
    xml_node<>* menuNode = doc.first_node("menu");
    if (!menuNode) {
        throw std::runtime_error("No <menu> root node found");
    }

    std::vector<GameInfo> games;

    for (xml_node<>* gameNode = menuNode->first_node("game");
         gameNode;
         gameNode = gameNode->next_sibling("game"))
    {
        xml_attribute<>* nameAttr = gameNode->first_attribute("name");
        if (!nameAttr) continue;

        xml_attribute<>* indexAttr = gameNode->first_attribute("index");
        xml_attribute<>* imageAttr = gameNode->first_attribute("image");

        GameInfo info;
        info.name      = nameAttr->value();
        info.indexAttr = indexAttr ? indexAttr->value() : "";
        info.imageAttr = imageAttr ? imageAttr->value() : "";

        games.push_back(std::move(info));
    }

    return games;
}

// Heuristic: scan original XML text line-by-line to find which lines contain name="XYZ".
// This is formatting-dependent (won't catch line-wrapped attributes, single quotes, name = "XYZ", etc.).
static std::vector<int> findLinesForName(const std::string& xmlContent,
                                         const std::string& name,
                                         std::size_t maxMatches) {
    std::vector<int> lines;
    const std::string pattern = "name=\"" + name + "\"";

    std::istringstream iss(xmlContent);
    std::string line;
    int lineNumber = 0;

    while (std::getline(iss, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.find(pattern) != std::string::npos) {
            lines.push_back(lineNumber);
            if (lines.size() >= maxMatches) break;
        }
    }

    return lines;
}

static bool reportDuplicateNames(const std::vector<GameInfo>& games,
                                 const std::string& xmlContent) {
    // name -> count
    std::unordered_map<std::string, int> counts;
    for (const auto& g : games) ++counts[g.name];

    // collect only duplicates
    std::vector<std::string> duplicateNames;
    duplicateNames.reserve(counts.size());
    for (const auto& kv : counts) {
        if (kv.second > 1) duplicateNames.push_back(kv.first);
    }

    if (duplicateNames.empty()) {
        std::cout << "No duplicate game names found.\n";
        return false;
    }

    std::sort(duplicateNames.begin(), duplicateNames.end());

    for (const auto& name : duplicateNames) {
        const int count = counts[name];

        auto lines = findLinesForName(xmlContent, name, static_cast<std::size_t>(count));

        std::ostringstream lineStr;
        lineStr << "(";
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) {
                if (i + 1 == lines.size()) lineStr << " and ";
                else lineStr << ", ";
            }
            lineStr << lines[i];
        }
        lineStr << ")";

        std::cout << "Name \"" << name << "\" appears "
                  << count << " times at lines "
                  << lineStr.str() << "\n";
    }

    return true;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: XmlGameDuplicateFinder <menu.xml>\n";
        return 1;
    }

    const std::string filename = argv[1];

    try {
        const std::string xml = loadFileToString(filename);

        // Keep parse buffer alive so parse_error::where() stays valid.
        std::vector<char> buffer(xml.begin(), xml.end());
        buffer.push_back('\0');

        xml_document<> doc;

        try {
            doc.parse<0>(buffer.data());
        }
        catch (const rapidxml::parse_error& e) {
            int line = 0, col = 0;
            computeLineCol(buffer.data(), e.where<char>(), line, col);

            std::cerr << "XML parse error: " << e.what() << "\n";
            std::cerr << "At line " << line << ", column " << col << "\n";

            const std::string lineText = extractLineText(xml, line);
            if (!lineText.empty()) {
                std::cerr << lineText << "\n";
                if (col > 0) std::cerr << std::string(static_cast<size_t>(col - 1), ' ') << "^\n";
            }
            return 1;
        }

        auto games = parseGamesFromDoc(doc);
        const bool hasDupes = reportDuplicateNames(games, xml);
        return hasDupes ? 2 : 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
