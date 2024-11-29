#include <iostream>
#include <fstream>
#include <string>
using namespace std;

enum Coherency {
    Modified,
    Owned,
    Exclusive,
    Shared,
    Invalid,
    Forward,
};

struct Line {
    Line() {
        c = Invalid;
        lru = 0;
        tag = 0;
        dirty = false;
    }

    Coherency c;
    int lru;
    int tag;
    bool dirty; // does not seem necessary as coherency state can determine whether to writeback
};

struct Cache {
    Line lines[4];

    Cache() {
        for (int i = 0; i < 4; i++) {
            lines[i].lru = i;
        }
    }

    void updateLRU(const int old) { // set entry with given old LRU state -> most recent
        for (int i = 0; i < 4; i++) {
            if (lines[i].lru > old) lines[i].lru--;
            else if (lines[i].lru == old) lines[i].lru = 3;
        }
    }

    void install(const int tag, const Coherency c) { // attempt to install with certain tag/coherency state
        for (int i = 0; i < 4; i++) {
            if (lines[i].c == Invalid) {
                installIndex(tag, c, i);
                return;
            }
        }

        for (int i = 0; i < 4; i++) {
            if (lines[i].lru == 0) {
                installIndex(tag, c, i);
                return;
            }
        }
    }

    void installIndex(const int tag, const Coherency c, const int index) {
        lines[index].c = c;
        lines[index].tag = tag;
        updateLRU(lines[index].lru);
    }

    void access(const int tag) { // helper function to update LRU for given tag
        for (int i = 0; i < 4; i++) {
            if (lines[i].tag == tag) updateLRU(lines[i].lru);
        }
    }

    Coherency getCoherency(const int tag) const {
        for (int i = 0; i < 4; i++) {
            if (lines[i].tag == tag) return lines[i].c;
        }
        return Invalid;
    }

    void setCoherency(const int tag, const Coherency c) {
        for (int i = 0; i < 4; i++) {
            if (lines[i].tag == tag) {
                lines[i].c = c;
                break;
            }
        }
    }
};

struct CPU {
    Cache cores[4];
    int hit;
    int miss;
    int writeback;
    int broadcast;
    int transfer;

    CPU() {
        hit = 0;
        miss = 0;
        writeback = 0;
        broadcast = 0;
        transfer = 0;
    }

    void handle(const int core, const bool isRead, const int tag) {
        isRead ? handleRead(core, tag) : handleWrite(core, tag);
    }

    void handleRead(const int core, const int tag) {
        bool transferred = false;

        switch (cores[core].getCoherency(tag)) {
            case Modified:
            case Owned:
            case Exclusive:
            case Shared:
                hit++;
                cores[core].access(tag);
                break;
            case Invalid:
                miss++;

                broadcast++;
                for (int i = 0; i < 4; i++) {
                    if (i == core) continue;

                    switch (cores[i].getCoherency(tag)) {
                        case Modified:
                            cores[i].setCoherency(tag, Owned);
                            transferred = true;
                            break;
                        case Exclusive:
                            cores[i].setCoherency(tag, Shared);
                            transferred = true;
                            break;
                        case Shared:
                        case Owned:
                            transferred = true;
                            break;
                        default: ;
                    }
                }
                transfer += transferred;

                cores[core].install(tag, transferred ? Shared : Exclusive);
                break;
            default: ;
        }
    }

    void handleWrite(const int core, const int tag) {
        switch (cores[core].getCoherency(tag)) {
            case Modified:
                hit++;
                cores[core].access(tag);
                break;
            case Owned:
                hit++;

                broadcast++;
                for (int i = 0; i < 4; i++) {
                    if (i == core) continue;

                    switch (cores[i].getCoherency(tag)) {
                        case Shared:
                            cores[i].setCoherency(tag, Invalid);
                            break;
                        default: ;
                    }
                }
                cores[core].setCoherency(tag, Modified);
                cores[core].access(tag);
            case Exclusive:
                hit++;
                cores[core].setCoherency(tag, Modified);
                cores[core].access(tag);
                break;
            case Shared:
                hit++;

                broadcast++;
                for (int i = 0; i < 4; i++) {
                    if (i == core) continue;

                    switch (cores[i].getCoherency(tag)) {
                        case Owned:
                            writeback++;
                        case Shared:
                            cores[i].setCoherency(tag, Invalid);
                            break;
                        default: ;
                    }
                }

                cores[core].setCoherency(tag, Modified);
                cores[core].access(tag);
                break;
            case Invalid:
                miss++;

                broadcast++;
                for (int i = 0; i < 4; i++) {
                    if (i == core) continue;

                    switch (cores[i].getCoherency(tag)) {
                        case Modified:
                        case Owned:
                            writeback++;
                        case Shared:
                            cores[i].setCoherency(tag, Invalid);
                            break;
                        default: ;
                    }
                }

                cores[core].install(tag, Modified);
                break;
            default: ;
        }
    }

    void report() const {
        cout << hit << endl << miss << endl << writeback << endl << broadcast << endl << transfer << endl;
    }
};

int main(const int argc, char *argv[]) {
    if (argc != 2) {
        return 1;
    }

    CPU cpu;

    ifstream actions(argv[1]);
    string action;
    while (getline(actions, action)) {
        const int core = stoi(action.substr(1, 1));
        const bool isRead = action[4] == 'r';

        const size_t tagStart = action.find('<') + 1;
        const int tag = stoi(action.substr(tagStart, action.length() - 1 - tagStart));

        cpu.handle(core, isRead, tag);
    }

    cpu.report();
    return 0;
}
