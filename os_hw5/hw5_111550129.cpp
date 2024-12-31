/*
Student No.: 111550129
Student Name: 林彥亨
Email: yanheng.cs11@nycu.edu.tw
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not 
supposed to be posted to a public server, such as a 
public GitHub repository or a public web page.
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <chrono>
#include <iomanip>
#include <sys/time.h>

#define PAGE_SIZE 4096
#define TABLE_SIZE 500003  

using namespace std;

struct HashNode {
    unsigned long long page_number;
    list<unsigned long long>::iterator list_itr; 
    bool is_dirty;                               
    HashNode *next;
};


class HashTable {
private:
    vector<HashNode *> table;

public:
    HashTable() : table(TABLE_SIZE, nullptr) {}

    HashNode *find(unsigned long long page_number) {
        int hash = page_number % TABLE_SIZE;
        HashNode *node = table[hash];
        while (node) {
            if (node->page_number == page_number) return node;
            node = node->next;
        }
        return nullptr; 
    }

    void insert(unsigned long long page_number, list<unsigned long long>::iterator list_itr, bool is_dirty) {
        int hash = page_number % TABLE_SIZE;
        HashNode *new_node = new HashNode{page_number, list_itr, is_dirty, table[hash]};
        table[hash] = new_node; 
    }

    void erase(unsigned long long page_number) {
        int hash = page_number % TABLE_SIZE;
        HashNode *node = table[hash], *prev = nullptr;

        while (node) {
            if (node->page_number == page_number) {
                if (prev) prev->next = node->next;
                else table[hash] = node->next;
                delete node;
                return;
            }
            prev = node;
            node = node->next;
        }
    }
};

// LRU
class LRUCache {
private:
    int capacity;
    list<unsigned long long> cache; 
    HashTable page_table;           

    int hit_count = 0;
    int miss_count = 0;
    int write_back_count = 0;       

public:
    LRUCache(int cap) : capacity(cap) {}

    void accessPage(unsigned long long page_number, bool is_write) {
        HashNode *node = page_table.find(page_number);

        if (node) {
            
            cache.erase(node->list_itr);
            cache.push_back(page_number);
            node->list_itr = --cache.end();
            if (is_write) node->is_dirty = true; 
            hit_count++;
        } else {
            
            miss_count++;
            if (cache.size() == capacity) {
                
                unsigned long long oldest_page = cache.front();
                cache.pop_front();

                
                HashNode *oldest_node = page_table.find(oldest_page);
                if (oldest_node && oldest_node->is_dirty) {
                    write_back_count++;
                }

                page_table.erase(oldest_page);
            }
            cache.push_back(page_number);
            page_table.insert(page_number, --cache.end(), is_write);
        }
    }

    int getHits() const { return hit_count; }
    int getMisses() const { return miss_count; }
    int getWriteBacks() const { return write_back_count; }
    double getFaultRatio() const { return static_cast<double>(miss_count) / (hit_count + miss_count); }
};

// CFLRU
class CFLRUCache {
private:
    int capacity;
    list<unsigned long long> cache; 
    HashTable page_table;           
    int window_size;

    int hit_count = 0;
    int miss_count = 0;
    int write_back_count = 0;       

public:
    CFLRUCache(int cap, int win_size) : capacity(cap), window_size(win_size) {}

    void accessPage(unsigned long long page_number, bool is_write) {
        HashNode *node = page_table.find(page_number);

        if (node) {
            cache.erase(node->list_itr);
            cache.push_back(page_number);
            node->list_itr = --cache.end();
            if (is_write) node->is_dirty = true; 
            hit_count++;
        } else {
            
            miss_count++;
            if (cache.size() == capacity) {
                
                auto it = cache.begin();
                for (int i = 0; i < window_size && it != cache.end(); ++i) {
                    HashNode *temp_node = page_table.find(*it);
                    if (temp_node && !temp_node->is_dirty) {
                        cache.erase(it);
                        page_table.erase(*it);
                        goto inserted;
                    }
                    ++it;
                }
                
                unsigned long long oldest_page = cache.front();
                cache.pop_front();
                HashNode *oldest_node = page_table.find(oldest_page);
                if (oldest_node && oldest_node->is_dirty) {
                    write_back_count++;
                }
                page_table.erase(oldest_page);
            }
        inserted:
            cache.push_back(page_number);
            page_table.insert(page_number, --cache.end(), is_write);
        }
    }

    int getHits() const { return hit_count; }
    int getMisses() const { return miss_count; }
    int getWriteBacks() const { return write_back_count; }
    double getFaultRatio() const { return static_cast<double>(miss_count) / (hit_count + miss_count); }
};


void simulateLRU(const vector<pair<char, unsigned long long>> &trace, int frame_size) {
    LRUCache lru(frame_size);

    for (const auto &entry : trace) {
        char op = entry.first;
        unsigned long long byte_offset = entry.second;
        unsigned long long page_number = byte_offset / PAGE_SIZE;
        bool is_write = (op == 'W');
        lru.accessPage(page_number, is_write);
    }

    
    cout << frame_size << "\t" << lru.getHits() << "\t" << lru.getMisses() << "\t\t"
         << fixed << setprecision(10) << lru.getFaultRatio() << "\t\t"
         << lru.getWriteBacks() << endl;
}


void simulateCFLRU(const vector<pair<char, unsigned long long>> &trace, int frame_size) {
    CFLRUCache cflru(frame_size, frame_size / 4);

    for (const auto &entry : trace) {
        char op = entry.first;
        unsigned long long byte_offset = entry.second;
        unsigned long long page_number = byte_offset / PAGE_SIZE;
        bool is_write = (op == 'W');
        cflru.accessPage(page_number, is_write);
    }

    
    cout << frame_size << "\t" << cflru.getHits() << "\t" << cflru.getMisses() << "\t\t"
         << fixed << setprecision(10) << cflru.getFaultRatio() << "\t\t"
         << cflru.getWriteBacks() << endl;
}


int main(int argc, char *argv[]) {
    struct timeval start, end;

    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <trace_file>" << endl;
        return EXIT_FAILURE;
    }

    string trace_file = argv[1];
    
    vector<pair<char, unsigned long long>> trace;

    ifstream infile(trace_file);
    if (!infile) {
        cerr << "Failed to open trace file: " << trace_file << endl;
        return EXIT_FAILURE;
    }

    char op;
    unsigned long long byte_offset;
    while (infile >> op >> hex >> byte_offset) {
        trace.emplace_back(op, byte_offset);
    }

    cout << "LRU policy: \n";
    cout << "Frame\tHit\t\tMiss\t\tPage Fault Ratio\tWrite Back Count" << endl;


    double total_elapsed_time = 0.0;
    gettimeofday(&start, 0);

    simulateLRU(trace, 4096);
    simulateLRU(trace, 8192);
    simulateLRU(trace, 16384);
    simulateLRU(trace, 32768);
    simulateLRU(trace, 65536);
    
    gettimeofday(&end, 0);

    total_elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    cout << "Total elapsed time: " << fixed << setprecision(6) << total_elapsed_time << " sec" << endl;

    cout << "\nCFLRU policy: \n";
    cout << "Frame\tHit\t\tMiss\t\tPage Fault Ratio\tWrite Back Count" << endl;

    gettimeofday(&start, 0);

    simulateCFLRU(trace, 4096);
    simulateCFLRU(trace, 8192);
    simulateCFLRU(trace, 16384);
    simulateCFLRU(trace, 32768);
    simulateCFLRU(trace, 65536);

    gettimeofday(&end, 0);
    total_elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    cout << "Total elapsed time: " << fixed << setprecision(6) << total_elapsed_time << " sec" << endl;

    return 0;
}
