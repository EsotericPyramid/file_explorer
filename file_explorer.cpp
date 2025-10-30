#include <ncurses.h>
#include <stdlib.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>


using namespace std;
using namespace __fs::filesystem;

class file_tree_string {
    vector<string> strs;
    vector<int> indents;

    public:

    file_tree_string() {
        strs = vector<string>();
        indents = vector<int>();
    }

    string to_string() {
        string out = "";
        for (int i = 0; i < strs.size(); i++) {
            out.append(get_entry(i));
        }
        return out;
    }

    int num_entries() {
        return strs.size();
    }

    string get_entry(int idx) {
        string out = "";
        for (int i = 0; i < indents[idx]; i++) {
            out.append("    ");
        }
        out.append(strs[idx]);
        return out;
    }

    int get_indent(int idx) {
        return indents[idx];
    }

    void append(file_tree_string other, int indent) {
        for (int i = 0; i < other.strs.size(); i++) {
            strs.push_back(other.strs[i]);
            indents.push_back(other.indents[i] + indent);
        }
    }

    void append(path path, int indent) {
        strs.push_back(path.filename().string());
        indents.push_back(indent);
    }

    void append(string str, int indent) {
        strs.push_back(str);
        indents.push_back(indent);
    }
};

class file_tree {
    path root;
    file_status state;
    bool expanded;
    vector<file_tree> children;

    public:

    file_tree() {} //???

    file_tree(path root) {
        this->root = root;
        state = status(root);
        expanded = false;
        children = vector<file_tree>();
    }

    bool expand() {
        try {
            if (state.type() == file_type::directory) {
                if (!expanded) {
                    children.clear();
                    auto iter = directory_iterator(root);
                    for (auto entry : iter) {
                        children.push_back(file_tree(entry.path()));
                    }
                    sort(children.begin(),children.end());
                    expanded = true;
                }
            }
            else return false;
            return true;
        } catch (...) {
            return false;
        }
    }

    bool close() {
        if (expanded) {
            expanded = false;
            children.clear();
            return true;
        } else return false;
    }

    bool toggle() {
        if (expanded) {
            return close();
        } else {
            return expand();
        }
    }

    int num_children() {
        if (expanded) {
            return children.size();
        } else {
            return -1;
        }
    }

    int num_entries() {
        int num = 1;
        for (file_tree& child : children) {
            num += child.num_entries();
        }
        return num;
    }

    file_tree* get(int idx) {
        return &children[idx];
    }

    vector<file_tree> get_children() {
        return children;
    }

    bool is_directory() {
        return state.type() == file_type::directory;
    }

    bool is_file() {
        return state.type() == file_type::regular;
    }

    path get_path() {
        return root;
    }

    file_status get_state() {
        return state;
    }

    bool is_expanded() {
        return expanded;
    }

    bool update() {
        file_tree old_self = *this;
        update(old_self);
    }

    bool update(file_tree& old_self) {
        state = status(root);
        if (state.type() == file_type::not_found) {
            return false;
        } else {
            if (old_self.expanded) {
                close();
                expand();
                for (file_tree& new_child : children) {
                    for (file_tree& old_child : old_self.children) {
                        if (new_child.root == old_child.root) {
                            new_child.update(old_child);
                        }
                    }
                }
            }
            return true;
        }
    }

    file_tree_string to_tree_string() {
        file_tree_string out = file_tree_string();
        string str = "";
        switch (state.type()) {
            case file_type::regular:
                str.append("-   ");
                break;
            case file_type::directory:
                if (expanded) str.append("V   "); else str.append(">   ");
                break;
            case file_type::symlink:
                str.append("~>  ");
                break;
            case file_type::block:
                str.append("[]  ");
                break;
            case file_type::fifo:
                str.append("|   ");
                break;
            case file_type::character:
                str.append("C   ");
                break;
            case file_type::socket:
                str.append("O-> ");
                break;
            case file_type::none:
                str.append("N/A ");
                break;
            case file_type::unknown:
                str.append("?   ");
                break;
            case file_type::not_found:
                str.append("ERR ");
                break;
        }
        str.append(root.filename().string());
        out.append(str, 0);
        for (file_tree child : children) {
            out.append(child.to_tree_string(), 1);
        }
        return out;
    }
    
    string to_string() {
        return to_tree_string().to_string();
    }

    bool operator<(const file_tree rhs) const {
        int dif = 10;
        switch (state.type()) {
            case file_type::directory:
                dif = 0;
                break;
            case file_type::symlink:
                dif = 1;
                break;
            case file_type::regular:
                dif = 2;
                break;
        }
        switch (rhs.state.type()) {
            case file_type::directory:
                dif -= 0;
                break;
            case file_type::symlink:
                dif -= 1;
                break;
            case file_type::regular:
                dif -= 2;
                break;
        }
        if (dif != 0) {
            return dif < 0;
        } else {
            return root < rhs.root;
        }
    }
};

class file_explorer {
    file_tree tree;
    vector<int> idx;

    public:
    file_explorer() {
        tree = file_tree(current_path());
        tree.expand();
        idx = vector<int>();

        initscr();
        cbreak();
        noecho();
        curs_set(0);
        keypad(stdscr, TRUE);
    }

    void print() {
        clear();
        file_tree_string strs = tree.to_tree_string();
        int active_row = get_active_row();
        for (int i = 0; i < strs.num_entries(); i++) {
            mvprintw(i, 0, strs.get_entry(i).c_str());
        }
        mvchgat(active_row, 4 * strs.get_indent(active_row), strs.get_entry(active_row).length() - 4 * strs.get_indent(active_row), A_REVERSE, 1, NULL);
        refresh();
    }

    void main_loop() {
        while (true) {
            print();
            file_tree old_root;
            file_tree* entry = get_active_entry();
            file_tree* entry_parent = get_active_entry_parent();
            int c = getch();
            switch (c) {
                case 'd':
                case KEY_RIGHT: 
                    if (entry->expand()) {
                        idx.push_back(0);
                    }
                    break;
                case 'a':
                case KEY_LEFT:
                    if (idx.size() > 0) idx.pop_back();
                    break;
                case 'w':
                case KEY_UP:
                    if (idx.size() > 0) {
                        idx[idx.size() -1]--;
                        if (idx[idx.size() -1] < 0) idx[idx.size() -1] = 0;
                    }
                    break;
                case 's':
                case KEY_DOWN:
                    if (idx.size() > 0) {
                        idx[idx.size() -1]++;
                        if (idx[idx.size() -1] >= entry_parent->num_children()) idx[idx.size() -1] = entry_parent->num_children() -1;
                    }
                    break;
                case 'r':
                    old_root = tree;
                    tree = file_tree(tree.get_path().parent_path());
                    tree.expand();
                    for (int i = 0; i < tree.num_children(); i++) {
                        file_tree* child = tree.get(i);
                        if (child->get_path() == old_root.get_path()) {
                            child->update(old_root);
                            idx.insert(idx.begin(),i);
                        }
                    }
                    break;
                case 'f':
                    if (idx.size() > 0) {
                        old_root = tree;
                        tree = file_tree(old_root.get(idx[0])->get_path());
                        tree.update(*old_root.get(idx[0]));
                        idx.erase(idx.begin());
                    }
                    break;
                case 'e':
                    entry->toggle();
                    break;
                case 'v':
                    tree.update();
                    break;
                case 'q':
                    return;
                case 'c':
                    if (entry->is_file()) {
                        endwin();
                        cout << "Press any key to return to the file explorer";
                        cout.flush();
                        system(("less -f '" + entry->get_path().string() + "'").c_str());
                        initscr();
                        cbreak();
                        noecho();
                        curs_set(0);
                        keypad(stdscr, TRUE);
                    }
            };
        }
    }

    int get_active_row() {
        return get_row(idx);
    }

    int get_row(vector<int> idx) {
        int row = 0;
        file_tree* cursor = &tree;
        for (int i = 0; i < idx.size(); i++) {
            row++;
            for (int j = 0; j < idx[i]; j++) {
                row += cursor->get(j)->num_entries();
            }
            cursor = cursor->get(idx[i]);
        }
        return row;
    }

    file_tree* get_active_entry() {
        file_tree* cursor = &tree;
        for (int i = 0; i < idx.size(); i++) {
            cursor = cursor->get(idx[i]);
        }
        return cursor;
    }

    file_tree* get_active_entry_parent() {
        file_tree* cursor = &tree;
        if (idx.size() > 1) {
            for (int i = 0; i < idx.size() -1; i++) {
                cursor = cursor->get(idx[i]);
            }
        }
        return cursor;
    }

    ~file_explorer() {
        endwin();
    }
};

int main() {
    file_explorer fe = file_explorer();
    fe.main_loop();
}