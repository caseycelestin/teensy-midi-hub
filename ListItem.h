#ifndef LISTITEM_H
#define LISTITEM_H

// Maximum items in a list view
const int MAX_LIST_ITEMS = 16;

// Maximum visible items on OLED (4 rows fit on 64px height)
const int VISIBLE_ITEMS = 4;

// A single list item with optional left, center, right text
struct ListItem {
    const char* left;    // Optional left-aligned text
    const char* center;  // Optional centered text
    const char* right;   // Optional right-aligned text

    ListItem() : left(nullptr), center(nullptr), right(nullptr) {}
};

// A list of items with selection state
struct ListView {
    ListItem items[MAX_LIST_ITEMS];
    int count;
    int selectedIndex;

    ListView() : count(0), selectedIndex(0) {}

    void clear() {
        count = 0;
        selectedIndex = 0;
        for (int i = 0; i < MAX_LIST_ITEMS; i++) {
            items[i].left = nullptr;
            items[i].center = nullptr;
            items[i].right = nullptr;
        }
    }

    // Add item with center text only
    void add(const char* center) {
        if (count < MAX_LIST_ITEMS) {
            items[count].left = nullptr;
            items[count].center = center;
            items[count].right = nullptr;
            count++;
        }
    }

    // Add item with left, center, right text
    void add(const char* left, const char* center, const char* right) {
        if (count < MAX_LIST_ITEMS) {
            items[count].left = left;
            items[count].center = center;
            items[count].right = right;
            count++;
        }
    }

    // Move selection up
    void selectPrev() {
        if (selectedIndex > 0) {
            selectedIndex--;
        }
    }

    // Move selection down
    void selectNext() {
        if (selectedIndex < count - 1) {
            selectedIndex++;
        }
    }
};

#endif
