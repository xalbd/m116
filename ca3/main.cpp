#include <fstream>
#include <iostream>
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
    bool dirty;
};

struct Cache {
    Line lines[4];

    Cache() {
        for (int i = 0; i < 4; i++) {
            lines[i].lru = i;
        }
    }

    void updateLRU(const int old) {  // change entry with given old LRU state to be most recent
        for (int i = 0; i < 4; i++) {
            if (lines[i].lru > old)
                lines[i].lru--;
            else if (lines[i].lru == old)
                lines[i].lru = 3;
        }
    }

    bool install(const int tag, const Coherency c,
                 const bool dirty) {  // returns whether writeback occurred
        for (int i = 0; i < 4; i++) {
            if (lines[i].c == Invalid) {
                return installIndex(tag, c, dirty, i);
            }
        }

        for (int i = 0; i < 4; i++) {
            if (lines[i].lru == 0) {
                return installIndex(tag, c, dirty, i);
            }
        }

        return false;
    }

    bool installIndex(const int tag, const Coherency c, const bool dirty, const int index) {
        const bool out = lines[index].dirty;

        lines[index].tag = tag;
        setCoherency(tag, c);
        setDirty(tag, dirty);
        updateLRU(lines[index].lru);
        return out;
    }

    void access(const int tag) {  // helper function to update LRU for given tag
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

    void setDirty(const int tag, const bool dirty) {
        for (int i = 0; i < 4; i++) {
            if (lines[i].tag == tag) {
                lines[i].dirty = dirty;
                return;
            }
        }
    }

    bool getDirty(const int tag) const {
        for (int i = 0; i < 4; i++) {
            if (lines[i].tag == tag) return lines[i].dirty;
        }
        return false;
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
        bool isDirty = false;

        switch (cores[core].getCoherency(tag)) {
            case Modified:
            case Owned:
            case Forward:
            case Shared:
                broadcast++;
            case Exclusive:
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
                            isDirty = true;
                            break;
                        case Exclusive:
                            cores[i].setCoherency(tag, Forward);
                            transferred = true;
                            break;
                        case Forward:
                            transferred = true;
                            break;
                        case Owned:
                            transferred = true;
                            isDirty = true;
                            break;
                        default:;
                    }
                }
                transfer += transferred;

                writeback += cores[core].install(tag, transferred ? Shared : Exclusive, isDirty);
                break;
            default:;
        }
    }

    void handleWrite(const int core, const int tag) {
        bool needWriteback = false;

        switch (cores[core].getCoherency(tag)) {
            case Modified:
                hit++;
                cores[core].access(tag);
                break;
            case Owned:
                hit++;
                needWriteback = true;

                broadcast++;
                for (int i = 0; i < 4; i++) {
                    if (i == core) continue;

                    switch (cores[i].getCoherency(tag)) {
                        case Shared:
                            cores[i].setCoherency(tag, Invalid);
                            break;
                        default:;
                    }
                }
                cores[core].setCoherency(tag, Modified);
                cores[core].access(tag);
                cores[core].setDirty(tag, true);
            case Exclusive:
                hit++;
                cores[core].setCoherency(tag, Modified);
                cores[core].access(tag);
                cores[core].setDirty(tag, true);
                break;
            case Shared:
                hit++;

                broadcast++;
                for (int i = 0; i < 4; i++) {
                    if (i == core) continue;

                    switch (cores[i].getCoherency(tag)) {
                        case Owned:
                            needWriteback = true;
                        case Forward:
                        case Shared:
                            cores[i].setCoherency(tag, Invalid);
                            break;
                        default:;
                    }
                }

                cores[core].setCoherency(tag, Modified);
                cores[core].access(tag);
                cores[core].setDirty(tag, true);
                break;
            case Invalid:
                miss++;

                broadcast++;
                for (int i = 0; i < 4; i++) {
                    if (i == core) continue;

                    switch (cores[i].getCoherency(tag)) {
                        case Owned:
                            needWriteback = true;
                        case Modified:
                        case Forward:
                        case Shared:
                            cores[i].setCoherency(tag, Invalid);
                            break;
                        default:;
                    }
                }

                writeback += cores[core].install(tag, Modified, true);
                break;
            default:;
        }
        writeback += needWriteback;
    }

    void report() const {
        cout << hit << endl
             << miss << endl
             << writeback << endl
             << broadcast << endl
             << transfer;
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
        const int core = stoi(action.substr(1, 1)) - 1;
        const bool isRead = action[4] == 'r';

        const size_t tagStart = action.find('<') + 1;
        const int tag = stoi(action.substr(tagStart, action.length() - 1 - tagStart));

        cpu.handle(core, isRead, tag);
    }

    cpu.report();
    return 0;
}
