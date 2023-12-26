#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories);

bool FindIncludeFile(const path& source, size_t line, const path& in_file, const path& out_file, const vector<path>& include_directories) {
    bool found = false;
    for (const auto& file : include_directories) {
        if (exists(path(file / source))) {  
            Preprocess(file / source, out_file, include_directories);
            found = true;
            break;
        }
    }
    if (found == false) {
        cout << "unknown include file "s << source.string() << " at file "s << in_file.string() << " at line "s << line << endl;
    }
    return found;
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories){
    static regex regex1(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex regex2(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");
    ifstream fin(in_file);
    ofstream fout(out_file, ios::out | ios::app);
    string str;
    smatch m;
    int line = 0;
    while (getline(fin, str)) {
        ++line;
        if (regex_match(str, m, regex1)) {
            path source = string(m[1]);
            path p = in_file.parent_path() / source;
            ifstream current_file;
            current_file.open(p);
            if (current_file.is_open()) {
                Preprocess(p, out_file, include_directories);
                current_file.close();
            }
            else {
                bool found = FindIncludeFile(source, line, in_file, out_file, include_directories);
                if (found == false){
                return false;
                }
            }
        }
        else if (regex_match(str, m, regex2)) {
            path source = string(m[1]);
            bool found = FindIncludeFile(source, line, in_file, out_file, include_directories);
             if (found == false){
                return false;
                }
        }
        else {
            fout << str << endl;
        }
    }
    return true;
}
    

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}
