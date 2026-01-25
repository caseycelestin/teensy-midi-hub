#include "RouteManager.h"
#include <EEPROM.h>
#include <string.h>

// EEPROM layout:
// [0-1]: Magic bytes (EEPROM_MAGIC)
// [2]:   Version
// [3]:   Route count
// [4+]:  Routes (56 bytes each: srcVid, srcPid, dstVid, dstPid, srcName[24], dstName[24])

const int ROUTE_SIZE = 8 + 24 + 24;  // VID:PID pairs + names

RouteManager::RouteManager() : routeCount(0) {
    for (int i = 0; i < MAX_ROUTES; i++) {
        routes[i].active = false;
        routes[i].sourceName[0] = '\0';
        routes[i].destName[0] = '\0';
    }
}

void RouteManager::load() {
    // Check magic bytes
    uint16_t magic = EEPROM.read(EEPROM_START_ADDR) | (EEPROM.read(EEPROM_START_ADDR + 1) << 8);
    if (magic != EEPROM_MAGIC) {
        // No valid data, start fresh
        routeCount = 0;
        return;
    }

    // Check version
    uint8_t version = EEPROM.read(EEPROM_START_ADDR + 2);
    if (version != EEPROM_VERSION) {
        // Incompatible version, start fresh
        routeCount = 0;
        return;
    }

    // Read route count
    routeCount = EEPROM.read(EEPROM_START_ADDR + 3);
    if (routeCount > MAX_ROUTES) {
        routeCount = 0;
        return;
    }

    // Read routes
    int addr = EEPROM_START_ADDR + 4;
    for (int i = 0; i < routeCount; i++) {
        routes[i].sourceVid = EEPROM.read(addr) | (EEPROM.read(addr + 1) << 8);
        routes[i].sourcePid = EEPROM.read(addr + 2) | (EEPROM.read(addr + 3) << 8);
        routes[i].destVid = EEPROM.read(addr + 4) | (EEPROM.read(addr + 5) << 8);
        routes[i].destPid = EEPROM.read(addr + 6) | (EEPROM.read(addr + 7) << 8);

        // Read source name
        for (int j = 0; j < 24; j++) {
            routes[i].sourceName[j] = EEPROM.read(addr + 8 + j);
        }
        routes[i].sourceName[23] = '\0';

        // Read dest name
        for (int j = 0; j < 24; j++) {
            routes[i].destName[j] = EEPROM.read(addr + 32 + j);
        }
        routes[i].destName[23] = '\0';

        routes[i].active = true;
        addr += ROUTE_SIZE;
    }
}

void RouteManager::save() {
    // Write magic bytes
    EEPROM.write(EEPROM_START_ADDR, EEPROM_MAGIC & 0xFF);
    EEPROM.write(EEPROM_START_ADDR + 1, (EEPROM_MAGIC >> 8) & 0xFF);

    // Write version
    EEPROM.write(EEPROM_START_ADDR + 2, EEPROM_VERSION);

    // Write route count
    EEPROM.write(EEPROM_START_ADDR + 3, routeCount);

    // Write routes
    int addr = EEPROM_START_ADDR + 4;
    for (int i = 0; i < routeCount; i++) {
        EEPROM.write(addr, routes[i].sourceVid & 0xFF);
        EEPROM.write(addr + 1, (routes[i].sourceVid >> 8) & 0xFF);
        EEPROM.write(addr + 2, routes[i].sourcePid & 0xFF);
        EEPROM.write(addr + 3, (routes[i].sourcePid >> 8) & 0xFF);
        EEPROM.write(addr + 4, routes[i].destVid & 0xFF);
        EEPROM.write(addr + 5, (routes[i].destVid >> 8) & 0xFF);
        EEPROM.write(addr + 6, routes[i].destPid & 0xFF);
        EEPROM.write(addr + 7, (routes[i].destPid >> 8) & 0xFF);

        // Write source name
        for (int j = 0; j < 24; j++) {
            EEPROM.write(addr + 8 + j, routes[i].sourceName[j]);
        }

        // Write dest name
        for (int j = 0; j < 24; j++) {
            EEPROM.write(addr + 32 + j, routes[i].destName[j]);
        }

        addr += ROUTE_SIZE;
    }
}

bool RouteManager::addRoute(uint16_t srcVid, uint16_t srcPid, const char* srcName,
                            uint16_t dstVid, uint16_t dstPid, const char* dstName) {
    // Check if already exists
    if (findRoute(srcVid, srcPid, dstVid, dstPid) >= 0) {
        return false;
    }

    // Check if full
    if (routeCount >= MAX_ROUTES) {
        return false;
    }

    // Add new route
    routes[routeCount].sourceVid = srcVid;
    routes[routeCount].sourcePid = srcPid;
    routes[routeCount].destVid = dstVid;
    routes[routeCount].destPid = dstPid;

    strncpy(routes[routeCount].sourceName, srcName, sizeof(routes[routeCount].sourceName) - 1);
    routes[routeCount].sourceName[sizeof(routes[routeCount].sourceName) - 1] = '\0';

    strncpy(routes[routeCount].destName, dstName, sizeof(routes[routeCount].destName) - 1);
    routes[routeCount].destName[sizeof(routes[routeCount].destName) - 1] = '\0';

    routes[routeCount].active = true;
    routeCount++;

    save();
    return true;
}

bool RouteManager::removeRoute(uint16_t srcVid, uint16_t srcPid, uint16_t dstVid, uint16_t dstPid) {
    int index = findRoute(srcVid, srcPid, dstVid, dstPid);
    if (index < 0) {
        return false;
    }
    return removeRouteByIndex(index);
}

bool RouteManager::removeRouteByIndex(int index) {
    if (index < 0 || index >= routeCount) {
        return false;
    }

    // Shift remaining routes down
    for (int i = index; i < routeCount - 1; i++) {
        routes[i] = routes[i + 1];
    }
    routes[routeCount - 1].active = false;
    routeCount--;

    save();
    return true;
}

bool RouteManager::hasRoute(uint16_t srcVid, uint16_t srcPid, uint16_t dstVid, uint16_t dstPid) const {
    return findRoute(srcVid, srcPid, dstVid, dstPid) >= 0;
}

bool RouteManager::shouldRoute(uint16_t srcVid, uint16_t srcPid, uint16_t dstVid, uint16_t dstPid) const {
    // If no routes configured, don't route anything (explicit routing only)
    if (routeCount == 0) {
        return false;
    }
    return hasRoute(srcVid, srcPid, dstVid, dstPid);
}

const Route* RouteManager::getRoute(int index) const {
    if (index < 0 || index >= routeCount) {
        return nullptr;
    }
    return &routes[index];
}

int RouteManager::getRouteCount() const {
    return routeCount;
}

void RouteManager::clearAll() {
    routeCount = 0;
    for (int i = 0; i < MAX_ROUTES; i++) {
        routes[i].active = false;
    }
    save();
}

int RouteManager::findRoute(uint16_t srcVid, uint16_t srcPid, uint16_t dstVid, uint16_t dstPid) const {
    for (int i = 0; i < routeCount; i++) {
        if (routes[i].sourceVid == srcVid && routes[i].sourcePid == srcPid &&
            routes[i].destVid == dstVid && routes[i].destPid == dstPid) {
            return i;
        }
    }
    return -1;
}
