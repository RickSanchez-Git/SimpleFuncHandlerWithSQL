#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <list>

#include "mysql.h" 

using namespace std;
using namespace std::filesystem;

#define SERVER "localhost"
#define USER "admin"
#define PASSWORD "admin"
#define DATABASE "lr5" 

class Container {
protected:
	int index = 0;
	int localIndex = 0;
	int semicolonCounter = 0;
	string body;
	string expression;
	string* str;
public:
	Container(int index, string *str) {
		this->index = this->localIndex = index;
		this->str = str;
	}

	int getSemicolonCounter() {
		return this->semicolonCounter;
	}

	bool validateExpression() {
		while ((*(this->str))[this->localIndex] != '\0') {
			if ((*(this->str))[this->localIndex] == '(') {
				return true;
			}
			if ((*(this->str))[this->localIndex] != ' ' && (*(this->str))[this->localIndex] != '\n' && (*(this->str))[this->localIndex] != '\t') {
				return false;
			}
			this->localIndex++;
		}
	}

	virtual void handleExpression() {
		this->localIndex++; // to skip '('
		int nestingCounter = 0;
		while (true) {
			if ((*(this->str))[this->localIndex] == '(') {
				nestingCounter++;
			}
			if ((*(this->str))[this->localIndex] == ')') {
				if (nestingCounter == 0) {
					return;
				}
				nestingCounter--;
			}
			this->expression += (*(this->str))[this->localIndex];
			this->localIndex++;
		}
	}
	virtual void handleBody() {
		this->localIndex++; // to skip ')'
		while ((*(this->str))[this->localIndex] == ' ' || (*(this->str))[this->localIndex] == '\n' || (*(this->str))[this->localIndex] == '\t') {
			this->localIndex++;
		}
		this->localIndex++; // to skip '{'
		int nestingCounter = 0;
		while (true) {
			if ((*(this->str))[this->localIndex] == ';') {
				this->semicolonCounter++;
			}
			if ((*(this->str))[this->localIndex] == '{') {
				nestingCounter++;
			}
			if ((*(this->str))[this->localIndex] == '}') {
				if (nestingCounter == 0) {
					return;
				}
				nestingCounter--;
			}
			this->body += (*(this->str))[this->localIndex];
			this->localIndex++;
		}
	}
	virtual string print(){
		return "default print";
	};
	virtual string getSqlValues() {
		return "VALUES('" + this->body + "','" + this->expression + "')";
	}
};

class ForContainer : public Container {
private:
	string initBlock;
	string logicalBlock;
	string actionBlock;

	void cutBlock(string *block) {
		this->localIndex++; // to skip '(' and ';'
		int nestingCounter = 0;
		while ((*(this->str))[this->localIndex] != ';' && ((*(this->str))[this->localIndex] != ')' || nestingCounter != 0)) {
			if ((*(this->str))[this->localIndex] != '\n' && (*(this->str))[this->localIndex] != '\t') {
				*block += (*str)[this->localIndex];
			}
			if ((*(this->str))[this->localIndex] == '(') {
				nestingCounter++;
			}
			if ((*(this->str))[this->localIndex] == ')') {
				nestingCounter--;
			}
			this->localIndex++;
		}
	}
	void handleExpression() {
		cutBlock(&initBlock);
		cutBlock(&logicalBlock);
		cutBlock(&actionBlock);
	}
	void handleBody() {
		this->localIndex++; // to skip ')'
		while ((*(this->str))[this->localIndex] == ' ' || (*(this->str))[this->localIndex] == '\n' || (*(this->str))[this->localIndex] == '\t') {
			this->localIndex++;
		}
		switch ((*(this->str))[this->localIndex]) {
		case '{':
			handleDefaultBody();
			break;
		case ';':
			handleEmptyBody();
			break;
		default:
			handleShortBody();
			break;
		}
	}
	void handleDefaultBody() {
		this->localIndex++; // to skip '{'
		int nestingCounter = 0;
		while (true) {
			if ((*(this->str))[this->localIndex] == ';') {
				this->semicolonCounter++;
			}
			if ((*(this->str))[this->localIndex] == '{') {
				nestingCounter++;
			}
			if ((*(this->str))[this->localIndex] == '}') {
				if (nestingCounter == 0) {
					return;
				}
				nestingCounter--;
			}
			this->body += (*(this->str))[this->localIndex];
			this->localIndex++;
		}
	}
	void handleEmptyBody() {
		this->semicolonCounter++;
		this->body += ';';
	}
	void handleShortBody() {
		this->semicolonCounter++;
		while ((*(this->str))[this->localIndex - 1] != ';') {
			this->body += (*(this->str))[this->localIndex];
			this->localIndex++;
		}
	}
public:
	ForContainer(int index, string *str) : Container(index, str) {};
	void handleFor() {
		this->localIndex += 3;
		if (!validateExpression()) {
			return;
		}
		handleExpression();
		handleBody();
	}
	string print() {
		if (this->semicolonCounter == 1) {
			return "for(" + this->initBlock + ";" + this->logicalBlock + ";"
			 + this->actionBlock + ")" + "\n\t" + this->body + '\n';
		}
		else {
			return "for(" + this->initBlock + ";" + this->logicalBlock + ";" 
			+ this->actionBlock + ")" + "{" + this->body + "}" + '\n';
		}
	}
	string getSqlValues() {
		return "VALUES('" + this->body + "','" + this->initBlock + "','" +
			this->logicalBlock + "','" + this->actionBlock + "')";
	}
};

class IfContainer : public Container{
public:
	IfContainer(int index, string* str) : Container(index, str) {};
	void handleIf() {
		this->localIndex += 2;
		if (!validateExpression()) {
			return;
		}
		handleExpression();
		handleBody();
	}
	string print() {
		if (this->semicolonCounter == 1) {
			return "if(" + this->expression + ")" + this->body + '\n';
		}
		else {
			return "if(" + this->expression + ")" + "{" + this->body + "}" + '\n';
		}
	}
};

class SwitchContainer : public Container{
public:
	SwitchContainer(int index, string* str) : Container(index, str) {};
	void handleSwitch() {
		this->localIndex += 6;
		if (!validateExpression()) {
			return;
		}
		handleExpression();
		handleBody();
	}
	string print() {
		return "switch(" + this->expression + ")" + "{" + this->body + "}" + '\n';
	}
};

class FileHandler {
private:
	string inputText;
	string outputText;
	path inputDir;
	path outputDir;
	fstream file;
public:
	FileHandler(path inputDir, path outputDir, char fileNameIter) {
		this->inputDir = inputDir;

		string fileName = "output().txt";
		fileName.insert(7, 1, fileNameIter);

		this->outputDir = outputDir / fileName;
	}
	string addOutput(string text) {
		outputText += text;
		return text;
	}
	string getInput() {
		return inputText;
	}
	string* getInputPtr() {
		return &inputText;
	}
	void readFile() {
		file.open(inputDir, fstream::in);
		string line;
		if (file.is_open()) {
			while (getline(file, line)) {
				inputText += line + '\n';
			}
		}
		else {
			cout << "Error during reading file" << endl;
		}
		file.close();
	}
	void writeFile() {
		file.open(outputDir, fstream::out);
		if (file.is_open()) {
			file << outputText << endl;
		}
		else {
			cout << "Error during writing file" << endl;
		}
		file.close();
	}
	void handleFile() {
		MYSQL* connect = mysql_init(NULL);
		connect = mysql_real_connect(connect, SERVER, USER, PASSWORD, DATABASE, 0, NULL, 0);
		if (!connect) { 
			cout << "SQL-connection failed" << endl;
			return;
		}
		cout << "SQL-connection established" << endl;

		this->readFile();

		list<Container*> container;
		string queryString;

		for (int i = this->getInput().find("for", 0); i != string::npos; i = this->getInput().find("for", i + 1)) {
			ForContainer* forInstance = new ForContainer(i, this->getInputPtr());
			forInstance->handleFor();
			if (forInstance->getSemicolonCounter() == 0) {
				delete forInstance;
			}
			else {
				queryString = "INSERT INTO for_table(body, init_block, logical_block, action_block)" + forInstance->getSqlValues();
				mysql_query(connect, queryString.c_str());
				container.push_back(forInstance);
			}
		}
		for (int i = this->getInput().find("if", 0); i != string::npos; i = this->getInput().find("if", i + 1)) {
			IfContainer* ifInstance = new IfContainer(i, this->getInputPtr());
			ifInstance->handleIf();
			if (ifInstance->getSemicolonCounter() == 0) {
				delete ifInstance;
			}
			else {
				queryString = "INSERT INTO if_table(body, expression)" + ifInstance->getSqlValues();
				mysql_query(connect, queryString.c_str());
				container.push_back(ifInstance);
			}
		}
		for (int i = this->getInput().find("switch", 0); i != string::npos; i = this->getInput().find("switch", i + 1)) {
			SwitchContainer* switchInstance = new SwitchContainer(i, this->getInputPtr());
			switchInstance->handleSwitch();
			if (switchInstance->getSemicolonCounter() == 0) {
				delete switchInstance;
			}
			else {
				queryString = "INSERT INTO switch_table(body, expression)" + switchInstance->getSqlValues();
				mysql_query(connect, queryString.c_str());
				container.push_back(switchInstance);
			}
		}
		cout << "Success!" << endl;
		/*cout << "Size of Container(number of elements): " << container.size() << endl << endl;
		cout << "Elements: " << endl << endl;
		for (Container* i : container) {
			cout << "Semicolons: " << i->getSemicolonCounter() << endl;
			cout << this->addOutput(i->print());
			cout << endl;
		}*/

		//mysql_query(connect,"CREATE TABLE Staff(id INT,name VARCHAR(255) NOT NULL,position VARCHAR(30),birthday Date);");
		//mysql_query(connect, "INSERT INTO for_table(body, init_block, logical_block, action_block) VALUES('mybody', 'myinitblock', 'mylogicalblock', 'myactionblock')");

		mysql_close(connect);
		this->writeFile();
		container.clear();
	}
};