#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <memory>
#include <cstdio>
#include <cassert>
#include <algorithm>
#include <array>
#include <cctype>

using namespace std;

void printShader(const std::string &shader) {
	int lineNr = 1;
	std::string line;
	std::stringstream myfile(shader);
	
	while (std::getline(myfile, line))
		cout << lineNr++ << ": " << line << endl;
}

int nrOfLines(std::string in) {
	int number_of_lines = 0;
	std::string line;
	std::stringstream myfile(in);

	while (std::getline(myfile, line))
		++number_of_lines;
	return number_of_lines;
}

/// Try to find in the Haystack the Needle - ignore case
bool findString(const std::string & inStr, const std::string & pattern)
{
	auto it = std::search(
		inStr.begin(), inStr.end(),
		pattern.begin(), pattern.end(),
		[](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
	);
	return (it != inStr.end());
}
bool printErrorAndWarnings(const std::string strOutput) {
	bool foundError = false;
	std::istringstream vstream(strOutput);
	std::string vline;
	while (std::getline(vstream, vline)) {

		if (findString(vline, "warning") || findString(vline, "error")) {
			if (findString(vline, " is not yet complete")) // version warning
				continue;
			foundError = true;
			std::cout << vline << std::endl;
		}
	}
	return foundError;
}


std::string exec(const char* cmd) {
	char buffer[128];
	std::string result = "";
	std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) assert(false);
	while (!feof(pipe.get())) {
		if (fgets(buffer, 128, pipe.get()) != NULL)
			result += buffer;
	}
	return result;
}

void _parseInludes(std::string &in, std::string &out) {
	string line;
	stringstream stream(in);
	while (getline(stream, line)) {
		if (line.find("#include") != string::npos) {
			stringstream includeStream(line);
			
			string a, file;
			includeStream >> a >> file;
			file = file.substr(1, file.size() - 2);
			ifstream infile(file);
			if (!infile.good()) {
				cout << "could not open file: " << file << endl;
				exit(1);
			}
			string newStr;
			while (getline(infile, line))
				newStr += line + "\n";
			_parseInludes(newStr, out);

		}
		else
			out += line + "\n";
	}
}
bool exists(const std::string& name) {
	ifstream f(name.c_str());
	return f.good();
}
std::string parseInludes(std::string &in) {
	string out;
	_parseInludes(in, out);
	return out;
}

enum class State { common, vertex, fragment};

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cout << "shaderCompiler filename " << std::endl;
		return 1;
	}
	std::string inFile(argv[1]);
	ifstream infile(inFile + ".glsl");
	if (!infile.is_open()) {
		cout << "Could not open file: " << argv[1] << ".glsl" << endl;
		exit(1);
	}
	string line;
	string vertex, fragment, common;
	State state = State::common;
	while (getline(infile, line)) {
		if (line.find("@vert") != string::npos) {
			state = State::vertex;
			continue;
		}
		else if (line.find("@frag") != string::npos) {
			state = State::fragment;
			continue;
		}
		if (state == State::common)
			common += line + "\n";
		else if (state == State::vertex) {
			vertex += line + "\n";;
		}
		else if (state == State::fragment)
			fragment += line + "\n";
	}
	common = parseInludes(common);
	vertex = parseInludes(vertex);
	fragment = parseInludes(fragment);
	string outVertex = common + vertex;
	string outFragment = common + fragment;

	string vf = inFile + ".vert";
	string ff = inFile + ".frag";
	 
	ofstream outVertexFile(vf);
	ofstream outFragmentFile(ff);

	outVertexFile << outVertex;
	outFragmentFile << outFragment;

	outVertexFile.close();
	outFragmentFile.close();
	string vertexSpv = "glslangvalidator -V " + vf + " -o builds/" + vf + ".spv";
	string fragmentSpv = "glslangvalidator -V " + ff + " -o builds/" + ff + ".spv";
	bool foundError = false;

	cout << vf << endl;
	foundError = printErrorAndWarnings(exec(vertexSpv.c_str()));
	if (foundError)
		printShader(outVertex);
	
	cout << ff << endl;
	foundError = printErrorAndWarnings(exec(fragmentSpv.c_str()));
	if (foundError)
		printShader(outFragment);



	remove(vf.c_str());
	remove(ff.c_str());

	return 0;
}