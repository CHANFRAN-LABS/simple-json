#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <windows.h>
using namespace std;

class Attribute {
public:
	int m_id;
	int m_parentId;
	string m_key;
	string m_value;

	Attribute() {
		m_id = 0;
	}

	void printAttribute() {
		cout << "id:" << m_id << ", parentId:" << m_parentId << ", key: " << m_key << ", value:" << m_value << endl;
	}

};

class easyJson {
public:

	int m_currentId = 1;
	int m_currentParent = 0;
	string m_jsonString;
	vector<Attribute> m_jsonAttributes;
	
	easyJson(string input) {
		m_jsonString = input;
		stringToJson(m_jsonString);
	}

	char findNextDelimiter(string input) {
		for (char& character : input) {
			switch (character) {
				case ':':
					return ':';
				case ',':
					return ',';
				case '{':
					return '{';
				case '}':
					return '}';
				default:
					continue;
			}
		}

		return '0';
	}

	Attribute saveAttribute (Attribute attribute) {
		attribute.m_id = m_currentId;
		attribute.m_parentId = m_currentParent;
		m_jsonAttributes.push_back(attribute);

		Attribute newAttribute;
		return newAttribute;
	}

	Attribute getParent (int parentId) {
		Attribute attribute;
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			Attribute currentAttribute = m_jsonAttributes[i];
			if (currentAttribute.m_id == parentId) {
				attribute = currentAttribute;
			}
		}

		return attribute;
	}

	void stringToJson(string input) {
		bool running = true;
		size_t pos;
		char delimiter;

		Attribute attribute;

		while (running == true) {
			cout << "input: " << input << endl;
			delimiter = findNextDelimiter(input);

			switch (delimiter) {
				case ':':
					pos = input.find_first_of(':');
					attribute.m_key = input.substr(0, pos);
					input.erase(0, pos+1);
					break;
				
				case ',':
					pos = input.find_first_of(',');
					attribute.m_value = input.substr(0, pos);
					input.erase(0, pos+1);
					attribute = saveAttribute(attribute);
					m_currentId++;
					break;

				case '{':
					pos = input.find_first_of('{');	//should be zero or at least just whitespace?
					input.erase(0, pos+1);
					attribute = saveAttribute(attribute);
					m_currentParent = m_currentId;
					m_currentId++;
					break;
				
				case '}':
				{
					pos = input.find_first_of('}');
					attribute.m_value = input.substr(0, pos);
					input.erase(0, pos+1);
					attribute = saveAttribute(attribute);
					m_currentId++;
					Attribute parentAttribute = getParent(m_currentParent);
					m_currentParent = parentAttribute.m_parentId;
					break;
				}

				default:
					cout << "default" << endl;
			}

			if (input == "") {
				running = false;
				if (m_currentParent != 0) {
					throw 505;
				}
			}


		}

	}

	void printJsonAttributes() {
		for (int i = 0; i < m_jsonAttributes.size(); i++) {
			Attribute current = m_jsonAttributes[i];
			current.printAttribute();
		}
	}


};


int main(){

string basicJson = "{'name': 'charlie', 'age': '26'}";
string midJson = "{'name': 'charlie', 'skills': {'drawing': 'true', 'writing': 'false'}}";
string testJson = "{ 'name':'charlie', 'age':'24', 'parents': { 'mother': 'julia', 'father': { 'name': 'steve', 'age': '50' }}, 'dob': '123456'}";
easyJson testEasy(testJson);
testEasy.printJsonAttributes();

}