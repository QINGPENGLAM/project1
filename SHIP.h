// SHIP.h

#ifndef SHIP_H
#define SHIP_H

#include <iostream>
#include <vector>
#include <deque>

struct Position {
    int level, row, col;
    Position(int l = -1, int r = -1, int c = -1) : level(l), row(r), col(c) {}
    bool operator==(const Position &rhs) const {
        return level == rhs.level && row == rhs.row && col == rhs.col;
    }
};

class SHIP {
public:
    SHIP();
    void setSearchMode(bool useStackMode);
    void loadInput(std::istream &input);
    bool planRoute();
    void outputResult(std::ostream &output, char outputMode) const;

private:
    bool loadMapMode(std::istream &input);
    bool loadCoordinateListMode(std::istream &input);
    void initializeSearchContainer();
    void discoverNeighbors(const Position &current);
    bool isValidMove(const Position &pos) const;
    void addToSearchContainer(const Position &pos, const Position &parentPos);
    void reconstructPath();
    size_t levels;
    size_t levelSize;
    bool useStack;
    Position startPos, hangarPos;
    std::deque<Position> searchContainer;
    std::vector<std::vector<std::vector<bool>>> discovered;
    std::vector<std::vector<std::string>> stationMap;
    std::vector<std::vector<std::vector<Position>>> parent;
    std::vector<Position> pathTaken;
};

#endif // SHIP_H
