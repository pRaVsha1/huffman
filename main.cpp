//#define _GLIBCXX_DEBUG
#include <fstream>
#include <map>
#include <queue>
#include <vector>
#include <string>
#include <cstdint>
#include <random>

using namespace std;


typedef long long ll;
typedef ll ll;
const ll mod = 998244353;

const ll inf = ll(1e18) + 7;
//typedef long double ld;
mt19937 rnd(1919);

using namespace std;
typedef unsigned char uc;
static const int BITS_PER_BYTE = 8;

struct Node {
    uc ch;
    ll freq;
    Node *left;
    Node *right;

    Node(uc c, ll f, Node *l = nullptr, Node *r = nullptr)
        : ch(c), freq(f), left(l), right(r) {
    }
};

struct Compare {
    bool operator()(Node *a, Node *b) {
        return a->freq > b->freq;
    }
};

Node *build(const map<uc, ll> &freq) {
    priority_queue<Node *, vector<Node *>, Compare> pq;
    for (auto &kv: freq) pq.push(new Node(kv.first, kv.second));
    if (pq.size() == 1) {
        Node *n = pq.top();
        pq.pop();
        pq.push(new Node('\0', n->freq, n, nullptr));
    }
    while (pq.size() > 1) {
        Node *l = pq.top();
        pq.pop();
        Node *r = pq.top();
        pq.pop();
        pq.push(new Node('\0', l->freq + r->freq, l, r));
    }
    return pq.top();
}

void generateCodes(Node *root, const string &prefix, map<uc, string> &codes) {
    if (!root) {
        return;
    }
    if (!root->left && !root->right) {
        codes[root->ch] = (prefix.empty() ? "0" : prefix);
        return;
    }
    generateCodes(root->left, prefix + '0', codes);
    generateCodes(root->right, prefix + '1', codes);
}

void freeTree(Node *root) {
    if (!root) {
        return;
    }
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

void writeFreqMap(ostream &os, map<uc, ll> &freq) {
    ll distinct = freq.size();
    os.write((char *)(&distinct), sizeof(distinct));
    for (auto &kv: freq) {
        os.put((char) kv.first);
        ll f = kv.second;
        os.write((char *)(&f), sizeof(f));
    }
}

map<uc, ll> readFreqMap(istream &is) {
    map<uc, ll> freq;
    ll distinct;
    is.read((char *)(&distinct), sizeof(distinct));
    for (ll i = 0; i < distinct; i++) {
        uc c = is.get();
        ll f;
        is.read((char *)(&f), sizeof(f));
        freq[c] = f;
    }
    return freq;
}

void encodeFile(const string &inputFile, const string &archiveFile) {
    ifstream ifs(inputFile, ios::binary);
    if (!ifs.is_open()) return;
    map<uc, ll> freq;
    char c;
    while (ifs.get(c)) freq[(uc) c]++;
    if (freq.empty()) {
        ofstream o(archiveFile, ios::binary);
        ll zero = 0;
        o.write((char *)(&zero), sizeof(zero));
        o.close();
        ifs.close();
        return;
    }
    ifs.clear();
    ifs.seekg(0, ios::beg);
    Node *root = build(freq);
    map<uc, string> codes;
    generateCodes(root, "", codes);
    ofstream ofs(archiveFile, ios::binary);
    if (!ofs.is_open()) {
        freeTree(root);
        ifs.close();
        return;
    }
    writeFreqMap(ofs, freq);
    ifs.seekg(0, ios::end);
    ll originalSize = ifs.tellg();
    ifs.seekg(0, ios::beg);
    ofs.write((char *)(&originalSize), sizeof(originalSize));
    uc buf = 0;
    int bitCount = 0;
    while (ifs.get(c)) {
        const string &code = codes[(uc) c];
        for (char b: code) {
            buf <<= 1;
            if (b == '1') buf |= 1;
            bitCount++;
            if (bitCount == BITS_PER_BYTE) {
                ofs.put((char) buf);
                buf = 0;
                bitCount = 0;
            }
        }
    }
    if (bitCount > 0) {
        buf <<= (BITS_PER_BYTE - bitCount);
        ofs.put((char) buf);
    }
    ofs.put((char) bitCount);
    freeTree(root);
    ifs.close();
    ofs.close();
}

void decodeFile(const string &archiveFile, const string &outputFile) {
    ifstream ifs(archiveFile, ios::binary);
    if (!ifs.is_open()) return;
    map<uc, ll> freq = readFreqMap(ifs);
    if (freq.empty()) {
        ofstream emptyOut(outputFile, ios::binary);
        emptyOut.close();
        ifs.close();
        return;
    }
    Node *root = build(freq);
    ofstream ofs(outputFile, ios::binary);
    if (!ofs.is_open()) {
        freeTree(root);
        ifs.close();
        return;
    }
    ll originalSize = 0;
    ifs.read((char *)(&originalSize), sizeof(originalSize));
    ifs.seekg(0, ios::end);
    streamoff fileSize = ifs.tellg();
    ifs.seekg(fileSize - 1, ios::beg);
    uc usedBitsChar = 0;
    ifs.read((char *)(&usedBitsChar), 1);
    int usedBits = (int) usedBitsChar;
    ifs.seekg(0, ios::beg);
    ll distinct = 0;
    ifs.read((char *)(&distinct), sizeof(distinct));
    for (ll i = 0; i < distinct; i++) {
        ifs.ignore(1 + sizeof(ll));
    }
    ifs.ignore(sizeof(ll));
    streamoff dataStart = ifs.tellg();
    streamoff dataSize = fileSize - 1 - dataStart;
    ll decodedCount = 0;
    Node *current = root;
    for (streamoff i = 0; i < dataSize; i++) {
        char byte;
        if (!ifs.get(byte)) break;
        uc mask = 1 << (BITS_PER_BYTE - 1);
        for (int b = 0; b < BITS_PER_BYTE; b++) {
            bool bit = (byte & mask);
            mask >>= 1;
            current = bit ? current->right : current->left;
            if (!current->left && !current->right) {
                ofs.put((char) current->ch);
                decodedCount++;
                current = root;
                if (decodedCount == originalSize) break;
            }
        }
        if (decodedCount == originalSize) break;
    }
    if (decodedCount < originalSize && usedBits > 0) {
        ifs.clear();
        ifs.seekg(dataStart + dataSize, ios::beg);
        char lastByte;
        if (ifs.get(lastByte)) {
            uc mask = 1 << (BITS_PER_BYTE - 1);
            for (int b = 0; b < usedBits; b++) {
                bool bit = (lastByte & mask);
                mask >>= 1;
                current = bit ? current->right : current->left;
                if (!current->left && !current->right) {
                    ofs.put((char) current->ch);
                    decodedCount++;
                    current = root;
                    if (decodedCount == originalSize) break;
                }
            }
        }
    }
    freeTree(root);
    ifs.close();
    ofs.close();
}

int32_t main(int argc, char *argv[]) {
    string mode = argv[1];
    string archive = argv[2];
    string file = argv[3];
    if (mode == "encode") {
        encodeFile(file, archive);
    } else if (mode == "decode") {
        decodeFile(archive, file);
    } else {
        return 1;
    }
    return 0;
}
