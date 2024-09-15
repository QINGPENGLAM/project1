// Project Identifier: 950181F63D0A883F183EC0A5CC67B19928FE896A

#include <iostream>
#include <getopt.h>
#include <vector>
#include <deque>
#include <string>
#include <sstream>
#include <limits>
#include <algorithm>
#include "SHIP.h"

using namespace std;

SHIP::SHIP() {
    levels = 0;
    levelSize = 0;
    useStack = false;
    startPos = Position();
    hangarPos = Position();
}

void SHIP::setSearchMode(bool useStackMode) {
    useStack = useStackMode;
}

void SHIP::loadInput(istream &input) {
    string line;
    getline(input, line);

    // Check for empty input
    if (line.empty()) {
        cerr << "Error: Empty input.\n";
        exit(1);
    }

    char inputMode = line[0];

    if (inputMode == 'M') {
        if (!loadMapMode(input)) {
            exit(1);
        }
    } else if (inputMode == 'L') {
        if (!loadCoordinateListMode(input)) {
            exit(1);
        }
    } else {
        cerr << "Error: Invalid input mode.\n";
        exit(1);
    }
}

bool SHIP::planRoute() {
    initializeSearchContainer();
    while (!searchContainer.empty()) {
        Position current = searchContainer.front();
        searchContainer.pop_front();

        if (current == hangarPos) {
            reconstructPath();
            return true;
        }

        discoverNeighbors(current);
    }
    return false;
}

void SHIP::initializeSearchContainer() {
    searchContainer.clear();
    searchContainer.push_back(startPos);

    discovered = vector<vector<vector<bool>>>(
        levels,
        vector<vector<bool>>(levelSize, vector<bool>(levelSize, false)));

    parent = vector<vector<vector<Position>>>(
        levels,
        vector<vector<Position>>(levelSize, vector<Position>(levelSize, Position(-1, -1, -1))));

    discovered[static_cast<size_t>(startPos.level)]
              [static_cast<size_t>(startPos.row)]
              [static_cast<size_t>(startPos.col)] = true;
}

void SHIP::outputResult(ostream &output, char outputMode) const {
    // Always print the starting position
    output << "Start in level " << startPos.level << ", row " << startPos.row
           << ", column " << startPos.col << "\n";

    if (outputMode == 'M') {
        for (size_t l = 0; l < levels; ++l) {
            output << "//level " << l << '\n';
            for (size_t r = 0; r < levelSize; ++r) {
                output << stationMap[l][r] << '\n';
            }
        }
    } else if (outputMode == 'L') {
        if (pathTaken.empty()) {
            output << "No valid route found.\n";
            return;
        }
        output << "//path taken\n";
        for (const auto &pos : pathTaken) {
            // Cast indices to size_t to match array indexing
            char ch = stationMap[static_cast<size_t>(pos.level)]
                                [static_cast<size_t>(pos.row)]
                                [static_cast<size_t>(pos.col)];
            output << "(" << pos.level << "," << pos.row << "," << pos.col << "," << ch << ")\n";
        }
    }
}

bool SHIP::loadMapMode(istream &input) {
    if (!(input >> levels >> levelSize)) {
        cerr << "Error: Invalid map size format.\n";
        return false;
    }
    input.ignore(numeric_limits<streamsize>::max(), '\n');

    stationMap.resize(levels, vector<string>(levelSize));

    string line;
    bool startFound = false;
    bool hangarFound = false;

    for (size_t l = 0; l < levels; ++l) {
        for (size_t r = 0; r < levelSize; ++r) {
            do {
                if (!getline(input, line)) {
                    cerr << "Error: Unexpected end of input.\n";
                    return false;
                }
                line.erase(0, line.find_first_not_of(" \t\n\r"));
                line.erase(line.find_last_not_of(" \t\n\r") + 1);

                if (line.empty() || line.substr(0, 2) == "//") {
                    continue;
                }

                if (line.size() != levelSize) {
                    cerr << "Error: Incorrect map width on level " << l << ", row " << r << endl;
                    return false;
                }

                break;
            } while (true);

            stationMap[l][r] = line;

            for (size_t c = 0; c < levelSize; ++c) {
                char currentChar = stationMap[l][r][c];

                if (currentChar != 'S' && currentChar != 'H' && currentChar != 'E' &&
                    currentChar != '#' && currentChar != '.') {
                    cerr << "Error: Invalid character '" << currentChar << "' at ("
                         << l << "," << r << "," << c << ")" << endl;
                    return false;
                }

                if (currentChar == 'S') {
                    if (startFound) {
                        cerr << "Error: Multiple start positions found.\n";
                        return false;
                    }
                    startPos = Position(static_cast<int>(l), static_cast<int>(r), static_cast<int>(c));
                    startFound = true;
                } else if (currentChar == 'H') {
                    if (hangarFound) {
                        cerr << "Error: Multiple hangar positions found.\n";
                        return false;
                    }
                    hangarPos = Position(static_cast<int>(l), static_cast<int>(r), static_cast<int>(c));
                    hangarFound = true;
                }
            }
        }
    }

    if (!startFound || !hangarFound) {
        cerr << "Error: Missing start or hangar position.\n";
        return false;
    }

    return true;
}

bool SHIP::loadCoordinateListMode(istream &input) {
    if (!(input >> levels >> levelSize)) {
        cerr << "Error: Invalid map size format.\n";
        return false;
    }
    input.ignore(numeric_limits<streamsize>::max(), '\n');

    stationMap.resize(levels, vector<string>(levelSize, string(levelSize, '.')));

    string line;
    bool startFound = false;
    bool hangarFound = false;

    while (getline(input, line)) {
        line.erase(0, line.find_first_not_of(" \t\n\r"));
        line.erase(line.find_last_not_of(" \t\n\r") + 1);

        if (line.empty() || line.substr(0, 2) == "//") {
            continue;
        }

        int level, row, col;
        char ch;

        if (sscanf(line.c_str(), "(%d,%d,%d,%c)", &level, &row, &col, &ch) != 4) {
            cerr << "Error: Invalid coordinate format.\n";
            return false;
        }

        if (level < 0 || static_cast<size_t>(level) >= levels ||
            row < 0 || static_cast<size_t>(row) >= levelSize ||
            col < 0 || static_cast<size_t>(col) >= levelSize) {
            cerr << "Error: Invalid coordinate in list mode.\n";
            return false;
        }

        if (ch != 'S' && ch != 'H' && ch != 'E' && ch != '#' && ch != '.') {
            cerr << "Error: Invalid character '" << ch << "' in list mode.\n";
            return false;
        }

        if (ch == 'S') {
            if (startFound) {
                cerr << "Error: Multiple start positions found.\n";
                return false;
            }
            startPos = Position(level, row, col);
            startFound = true;
        } else if (ch == 'H') {
            if (hangarFound) {
                cerr << "Error: Multiple hangar positions found.\n";
                return false;
            }
            hangarPos = Position(level, row, col);
            hangarFound = true;
        }

        stationMap[static_cast<size_t>(level)]
                  [static_cast<size_t>(row)]
                  [static_cast<size_t>(col)] = ch;
    }

    if (!startFound || !hangarFound) {
        cerr << "Error: Missing start or hangar position.\n";
        return false;
    }

    return true;
}

void SHIP::discoverNeighbors(const Position &current) {
    vector<pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    for (const auto &dir : directions) {
        int nextRow = current.row + dir.first;
        int nextCol = current.col + dir.second;

        if (nextRow >= 0 && static_cast<size_t>(nextRow) < levelSize &&
            nextCol >= 0 && static_cast<size_t>(nextCol) < levelSize) {
            Position next(current.level, nextRow, nextCol);
            if (isValidMove(next)) {
                addToSearchContainer(next, current);
            }
        }
    }

    if (stationMap[static_cast<size_t>(current.level)]
                  [static_cast<size_t>(current.row)]
                  [static_cast<size_t>(current.col)] == 'E') {
        for (size_t l = 0; l < levels; ++l) {
            if (static_cast<int>(l) != current.level &&
                stationMap[l][static_cast<size_t>(current.row)]
                              [static_cast<size_t>(current.col)] == 'E' &&
                !discovered[l][static_cast<size_t>(current.row)]
                              [static_cast<size_t>(current.col)]) {
                Position nextElevator(static_cast<int>(l), current.row, current.col);
                addToSearchContainer(nextElevator, current);
            }
        }
    }
}

bool SHIP::isValidMove(const Position &pos) const {
    if (pos.level < 0 || static_cast<size_t>(pos.level) >= levels ||
        pos.row < 0 || static_cast<size_t>(pos.row) >= levelSize ||
        pos.col < 0 || static_cast<size_t>(pos.col) >= levelSize) {
        return false;
    }

    if (stationMap[static_cast<size_t>(pos.level)]
                  [static_cast<size_t>(pos.row)]
                  [static_cast<size_t>(pos.col)] == '#' ||
        discovered[static_cast<size_t>(pos.level)]
                  [static_cast<size_t>(pos.row)]
                  [static_cast<size_t>(pos.col)]) {
        return false;
    }

    return true;
}

void SHIP::addToSearchContainer(const Position &pos, const Position &parentPos) {
    if (useStack) {
        searchContainer.push_front(pos);
    } else {
        searchContainer.push_back(pos);
    }
    discovered[static_cast<size_t>(pos.level)]
              [static_cast<size_t>(pos.row)]
              [static_cast<size_t>(pos.col)] = true;
    parent[static_cast<size_t>(pos.level)]
          [static_cast<size_t>(pos.row)]
          [static_cast<size_t>(pos.col)] = parentPos;
}

void SHIP::reconstructPath() {
    Position current = hangarPos;
    pathTaken.push_back(current);

    while (!(current == startPos)) {
        Position prev = parent[static_cast<size_t>(current.level)]
                              [static_cast<size_t>(current.row)]
                              [static_cast<size_t>(current.col)];
        if (stationMap[static_cast<size_t>(current.level)]
                      [static_cast<size_t>(current.row)]
                      [static_cast<size_t>(current.col)] == '.') {
            if (current.level == prev.level) {
                if (current.row == prev.row - 1)
                    stationMap[static_cast<size_t>(current.level)]
                              [static_cast<size_t>(current.row)]
                              [static_cast<size_t>(current.col)] = 'n';
                else if (current.row == prev.row + 1)
                    stationMap[static_cast<size_t>(current.level)]
                              [static_cast<size_t>(current.row)]
                              [static_cast<size_t>(current.col)] = 's';
                else if (current.col == prev.col - 1)
                    stationMap[static_cast<size_t>(current.level)]
                              [static_cast<size_t>(current.row)]
                              [static_cast<size_t>(current.col)] = 'w';
                else if (current.col == prev.col + 1)
                    stationMap[static_cast<size_t>(current.level)]
                              [static_cast<size_t>(current.row)]
                              [static_cast<size_t>(current.col)] = 'e';
            } else {
                stationMap[static_cast<size_t>(current.level)]
                          [static_cast<size_t>(current.row)]
                          [static_cast<size_t>(current.col)] = static_cast<char>('0' + current.level);
            }
        }
        current = prev;
        pathTaken.push_back(current);
    }

    reverse(pathTaken.begin(), pathTaken.end());
}

void commandLine(int argc, char *argv[], bool &useStack, bool &useQueue, char &outputMode) {
    static struct option long_options[] = {
        {"stack", no_argument, nullptr, 's'},
        {"queue", no_argument, nullptr, 'q'},
        {"output", required_argument, nullptr, 'o'},
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, '\0'}
    };

    useStack = false;
    useQueue = false;
    outputMode = 'M';

    int opt;
    while ((opt = getopt_long(argc, argv, "sqo:h", long_options, nullptr)) != -1) {
        switch (opt) {
            case 's':
                if (useQueue) {
                    cerr << "Error: Both stack and queue options specified.\n";
                    exit(1);
                }
                useStack = true;
                break;
            case 'q':
                if (useStack) {
                    cerr << "Error: Both stack and queue options specified.\n";
                    exit(1);
                }
                useQueue = true;
                break;
            case 'o':
                if (optarg[0] == 'M' || optarg[0] == 'L') {
                    outputMode = optarg[0];
                } else {
                    cerr << "Error: Invalid output mode.\n";
                    exit(1);
                }
                break;
            case 'h':
                cout << "Usage: ship [--stack | --queue] [--output M|L] < input_file > output_file\n";
                exit(0);
            default:
                cerr << "Error: Unknown option.\n";
                exit(1);
        }
    }

    if (useStack && useQueue) {
        cerr << "Error: Both stack and queue options specified.\n";
        exit(1);
    }

    // Default to queue mode if neither is specified
    if (!useStack && !useQueue) {
        useQueue = true;
    }
}

int main(int argc, char *argv[]) {
    bool useStack = false;
    bool useQueue = false;
    char outputMode = 'M';

    commandLine(argc, argv, useStack, useQueue, outputMode);

    SHIP ship;
    ship.setSearchMode(useStack);
    ship.loadInput(cin);

    bool routeFound = ship.planRoute();

    if (!routeFound) {
        cout << "No valid route found.\n"; // Changed from cerr to cout
    }

    ship.outputResult(cout, outputMode);

    return routeFound ? 0 : 1;
}
