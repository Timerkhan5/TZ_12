#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

struct ListNode {
    ListNode* prev = nullptr;
    ListNode* next = nullptr;
    ListNode* rand = nullptr;
    std::string data;
};

class List {
public:
    ListNode* head = nullptr;
    ListNode* tail = nullptr;
    int count = 0;

    ~List() {
        clear();
    }

    void clear() {
        ListNode* cur = head;
        while (cur != nullptr) {
            ListNode* next = cur->next;
            delete cur;
            cur = next;
        }
        head = nullptr;
        tail = nullptr;
        count = 0;
    }

    ListNode* pushBack(const std::string& s) {
        ListNode* node = new ListNode();
        node->data = s;

        if (head == nullptr) {
            head = node;
            tail = node;
        }
        else {
            tail->next = node;
            node->prev = tail;
            tail = node;
        }

        ++count;
        return node;
    }

    void serialize(std::ofstream& out) const {
        if (!out.is_open()) {
            throw std::runtime_error("cannot open output file");
        }

        std::vector<ListNode*> nodes;
        nodes.reserve(count);

        std::unordered_map<ListNode*, int> indexByPtr;
        indexByPtr.reserve(count);

        ListNode* cur = head;
        int idx = 0;
        while (cur != nullptr) {
            nodes.push_back(cur);
            indexByPtr[cur] = idx++;
            cur = cur->next;
        }

        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (int i = 0; i < count; ++i) {
            ListNode* node = nodes[i];

            int len = static_cast<int>(node->data.size());
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            if (len > 0) {
                out.write(node->data.data(), len);
            }

            int randIndex = -1;
            if (node->rand != nullptr) {
                auto it = indexByPtr.find(node->rand);
                if (it == indexByPtr.end()) {
                    throw std::runtime_error("rand points outside the list");
                }
                randIndex = it->second;
            }

            out.write(reinterpret_cast<const char*>(&randIndex), sizeof(randIndex));
        }

        if (!out.good()) {
            throw std::runtime_error("failed to write binary file");
        }
    }

    void deserialize(std::ifstream& in) {
        if (!in.is_open()) {
            throw std::runtime_error("cannot open input file");
        }

        clear();

        int nodeCount = 0;
        in.read(reinterpret_cast<char*>(&nodeCount), sizeof(nodeCount));
        if (!in.good()) {
            throw std::runtime_error("failed to read node count");
        }

        if (nodeCount < 0) {
            throw std::runtime_error("invalid node count");
        }

        std::vector<ListNode*> nodes;
        std::vector<int> randIndexes;
        nodes.reserve(nodeCount);
        randIndexes.reserve(nodeCount);

        for (int i = 0; i < nodeCount; ++i) {
            int len = 0;
            in.read(reinterpret_cast<char*>(&len), sizeof(len));
            if (!in.good()) {
                throw std::runtime_error("failed to read string length");
            }

            if (len < 0) {
                throw std::runtime_error("invalid string length");
            }

            std::string s;
            s.resize(len);
            if (len > 0) {
                in.read(&s[0], len);
                if (!in.good()) {
                    throw std::runtime_error("failed to read string data");
                }
            }

            int randIndex = -1;
            in.read(reinterpret_cast<char*>(&randIndex), sizeof(randIndex));
            if (!in.good()) {
                throw std::runtime_error("failed to read rand index");
            }

            ListNode* node = pushBack(s);
            nodes.push_back(node);
            randIndexes.push_back(randIndex);
        }

        for (int i = 0; i < nodeCount; ++i) {
            if (randIndexes[i] == -1) {
                nodes[i]->rand = nullptr;
            }
            else {
                if (randIndexes[i] < 0 || randIndexes[i] >= nodeCount) {
                    throw std::runtime_error("rand index out of range");
                }
                nodes[i]->rand = nodes[randIndexes[i]];
            }
        }
    }
};

static std::pair<std::string, int> parseLine(const std::string& line, int lineNo) {
    std::size_t pos = line.rfind(';');
    if (pos == std::string::npos) {
        throw std::runtime_error("bad format in line " + std::to_string(lineNo));
    }

    std::string data = line.substr(0, pos);
    std::string indexPart = line.substr(pos + 1);

    if (indexPart.empty()) {
        throw std::runtime_error("empty rand index in line " + std::to_string(lineNo));
    }

    int randIndex = 0;
    try {
        std::size_t used = 0;
        randIndex = std::stoi(indexPart, &used);
        if (used != indexPart.size()) {
            throw std::runtime_error("bad rand index");
        }
    }
    catch (...) {
        throw std::runtime_error("bad rand index in line " + std::to_string(lineNo));
    }

    return { data, randIndex };
}

static List loadTextFile(const std::string& fileName) {
    std::ifstream in(fileName);
    if (!in.is_open()) {
        throw std::runtime_error("cannot open " + fileName);
    }

    std::vector<std::pair<std::string, int>> lines;
    std::string line;
    int lineNo = 0;

    while (std::getline(in, line)) {
        ++lineNo;

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        lines.push_back(parseLine(line, lineNo));
    }

    List list;
    std::vector<ListNode*> nodes;
    nodes.reserve(lines.size());

    for (const auto& item : lines) {
        nodes.push_back(list.pushBack(item.first));
    }

    for (std::size_t i = 0; i < lines.size(); ++i) {
        int randIndex = lines[i].second;

        if (randIndex == -1) {
            nodes[i]->rand = nullptr;
        }
        else {
            if (randIndex < 0 || randIndex >= static_cast<int>(nodes.size())) {
                throw std::runtime_error("rand index out of range in line " + std::to_string(i + 1));
            }
            nodes[i]->rand = nodes[randIndex];
        }
    }

    return list;
}

int main() {
    try {
        List list = loadTextFile("inlet.in");

        std::ofstream out("outlet.out", std::ios::binary);
        if (!out.is_open()) {
            throw std::runtime_error("cannot open outlet.out");
        }

        list.serialize(out);
        out.close();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}