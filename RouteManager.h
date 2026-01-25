#ifndef ROUTE_MANAGER_H
#define ROUTE_MANAGER_H

#include <stdint.h>
#include "Config.h"

// A stored route between two devices (identified by VID:PID)
struct Route {
    uint16_t sourceVid;
    uint16_t sourcePid;
    uint16_t destVid;
    uint16_t destPid;
    char sourceName[24];
    char destName[24];
    bool active;
};

// Manages MIDI routes and persists them to EEPROM
class RouteManager {
public:
    RouteManager();

    // Load routes from EEPROM
    void load();

    // Save routes to EEPROM
    void save();

    // Add a route (returns true if added, false if already exists or full)
    bool addRoute(uint16_t srcVid, uint16_t srcPid, const char* srcName,
                  uint16_t dstVid, uint16_t dstPid, const char* dstName);

    // Remove a route (returns true if found and removed)
    bool removeRoute(uint16_t srcVid, uint16_t srcPid, uint16_t dstVid, uint16_t dstPid);

    // Remove route by index
    bool removeRouteByIndex(int index);

    // Check if a route exists
    bool hasRoute(uint16_t srcVid, uint16_t srcPid, uint16_t dstVid, uint16_t dstPid) const;

    // Check if source should route to destination (by VID:PID)
    bool shouldRoute(uint16_t srcVid, uint16_t srcPid, uint16_t dstVid, uint16_t dstPid) const;

    // Get all routes for iteration
    const Route* getRoute(int index) const;
    int getRouteCount() const;

    // Clear all routes
    void clearAll();

private:
    Route routes[MAX_ROUTES];
    int routeCount;

    int findRoute(uint16_t srcVid, uint16_t srcPid, uint16_t dstVid, uint16_t dstPid) const;
};

#endif
